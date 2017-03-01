// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "PakLoaderPrivatePCH.h"
#include "PakLoader.h"
#include "Http.h"
#include "AsyncTaskDownloadPak.h"
#include "TimerManager.h"
#include "CoreMisc.h"
#include "Base64.h"

//----------------------------------------------------------------------//
// UAsyncTaskDownloadPak
//----------------------------------------------------------------------//


UAsyncTaskDownloadPak::UAsyncTaskDownloadPak(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		AddToRoot();
	}
}

void UAsyncTaskDownloadPak::GetDownloadFilenames(const FString& URL,
	FString& DownloadedFilename, FString& ETagFilename)
{
	FString DownloadDir = FPaths::GameSavedDir() + TEXT("DownloadedPaks/");
	FPaths::NormalizeDirectoryName(DownloadDir);
	IFileManager::Get().MakeDirectory(*DownloadDir, true);
	const FString DownloadedFilenameBase = DownloadDir / FBase64::Encode(URL);
	ETagFilename = DownloadedFilenameBase + ".etag";
	DownloadedFilename = DownloadedFilenameBase + ".pak";
}

UAsyncTaskDownloadPak* UAsyncTaskDownloadPak::DownloadPak(const FString& URL, bool CheckForUpdateOnly)
{
	UAsyncTaskDownloadPak* DownloadTask = NewObject<UAsyncTaskDownloadPak>();
	DownloadTask->bCheckForUpdateOnly = CheckForUpdateOnly;
	DownloadTask->Start(URL);
	return DownloadTask;
}

void UAsyncTaskDownloadPak::Start(FString URL)
{
	UE_LOG(PakLoader, Log, TEXT("Download request for: %s"), *URL);
	// Create the Http request and add to pending request list	
	TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->OnProcessRequestComplete().BindUObject(this, &UAsyncTaskDownloadPak::HandlePakRequest);
	HttpRequest->SetURL(URL);
	HttpRequest->SetVerb(TEXT("GET"));
	FString ETagFilename;
	FString DownloadedFilename;
	GetDownloadFilenames(URL, DownloadedFilename, ETagFilename);
	FString ETag;
	//IPlatformFile& PlatformFile = IPlatformFile::GetPlatformPhysical();
	IFileManager* const FileManager = &IFileManager::Get();
	if (FileManager->FileExists(*DownloadedFilename) && FFileHelper::LoadFileToString(ETag, *ETagFilename))
	{
		UE_LOG(PakLoader, Log, TEXT("Setting If-None-Match: %s"), *ETag);
	}
	HttpRequest->SetHeader(TEXT("If-None-Match"), ETag);
	HttpRequest->ProcessRequest();
}

void UAsyncTaskDownloadPak::HandlePakRequest(FHttpRequestPtr HttpRequest,
	FHttpResponsePtr HttpResponse, bool bSucceeded)
{
	RemoveFromRoot();
	const FString Url = HttpRequest->GetURL();
	const bool _304 = HttpResponse.IsValid() && HttpResponse->GetResponseCode() == 304;	
	if (HttpResponse.IsValid())
	{
		if (!_304)
		{
			UE_LOG(PakLoader, Log, TEXT("Content changed on server: %s"), *Url);
			OnUpdated.Broadcast(Url);
		}
		if (bCheckForUpdateOnly)
		{
			return;
		}
	}
	if (_304 || (bSucceeded && HttpResponse.IsValid() && HttpResponse->GetResponseCode() == 200 && HttpResponse->GetContentLength() > 0))
	{	
		FString DownloadedFilename;
		FString ETagFilename;
		GetDownloadFilenames(Url, DownloadedFilename, ETagFilename);
		if (!_304)
		{
			const FString ETag = HttpResponse->GetHeader("ETag");
			UE_LOG(PakLoader, Log, TEXT("Attempting to cache %s as %s"), *Url, *DownloadedFilename);
			IFileManager* const FileManager = &IFileManager::Get();
			//IPlatformFile& PlatformFile = IPlatformFile::GetPlatformPhysical();
			//FString TmpFilename(DownloadedFilename + ".tmp");
			//TmpFilename = FPaths::ConvertRelativePathToFull(TmpFilename);
			FString TmpFilename = FPaths::CreateTempFilename(*FPaths::GameSavedDir());
			FArchive* const Ar = FileManager->CreateFileWriter(*TmpFilename, 0);
			bool bIOError = (Ar == nullptr);
			if (bIOError)
			{
				UE_LOG(PakLoader, Error, TEXT("Couldn't create tmp file %s for %s"), *Url, *TmpFilename);
			}
			else
			{
				Ar->Serialize(const_cast<uint8*>(HttpResponse->GetContent().GetData()), HttpResponse->GetContentLength());
				Ar->Close();
				delete Ar;
				int32 Size = FileManager->FileSize(*TmpFilename);
				bIOError = (Size != HttpResponse->GetContentLength());
				if (bIOError)
				{
					UE_LOG(PakLoader, Error, TEXT("Could only write %d of %d bytes to %s for %s"), Size, HttpResponse->GetContentLength(), *TmpFilename, *Url);
				}
				else
				{
					bIOError = !FileManager->Move(*DownloadedFilename, *TmpFilename);
				}
				if (bIOError)
				{
					UE_LOG(PakLoader, Error, TEXT("Couldn't rename tmp file %s to %s for %s"), *TmpFilename, *DownloadedFilename, *Url);
				}
				else
				{
					if (ETag.Len() > 0)
					{
						bIOError = !FFileHelper::SaveStringToFile(ETag, *ETagFilename, FFileHelper::EEncodingOptions::ForceUTF8, FileManager);
						if (bIOError)
						{
							UE_LOG(PakLoader, Error, TEXT("Couldn't create etag file %s for %s"), *ETagFilename, *Url);
						}
					}
					else
					{
						UE_LOG(PakLoader, Log, TEXT("No ETag header for %s"), *Url);
					}
				}
			}
			if (bIOError)
			{
				UE_LOG(PakLoader, Error, TEXT("Couldn't save %s as %s"), *Url, *DownloadedFilename);
				OnFail.Broadcast(TEXT("Couldn't save downloaded file"));
				return;
			}
		}
		else
		{
			UE_LOG(PakLoader, Log, TEXT("Using cached file for %s"), *Url);
		}
		OnSuccess.Broadcast(DownloadedFilename);
		return;
	}
	UE_LOG(PakLoader, Error, TEXT("Error downloading %s: %d"), *Url, HttpResponse.IsValid() ? HttpResponse->GetResponseCode() : -1);
	OnFail.Broadcast(TEXT("Couldn't download file"));
}
