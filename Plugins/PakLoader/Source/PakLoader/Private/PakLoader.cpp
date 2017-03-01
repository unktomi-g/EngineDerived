// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "PakLoaderPrivatePCH.h"
#include "IPlatformFilePak.h"
#include "Engine.h"
#include "CallbackDevice.h"
#include "PackageName.h"
#include "StringClassReference.h"

#define LOCTEXT_NAMESPACE "FPakLoaderModule"

DEFINE_LOG_CATEGORY(PakLoader);

void FPakLoaderModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	StreamableManager = new FStreamableManager();
	PakPlatformFile = nullptr;
	UnloadId = 0;
}

void FPakLoaderModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	delete StreamableManager;
	//delete PakPlatformFile; // This should never be deleted as it's in the chain of IPlatformFiles if non-null
}

bool FPakLoaderModule::GetLevelsFromPak(const FString& PakFilePath, TArray<FString>& Levels)
{
	Levels.Reset();
	TSharedPtr<FPakFile> Ptr;
	if (!MountPakFile(PakFilePath, Ptr))
	{
		return false;
	}
	TArray<FStringAssetReference> Refs;
	if (!GetAssetReferencesFromPak(Ptr, FPackageName::GetMapPackageExtension(), Refs))
	{
		return false;
	}
	for (int32 i = 0; i < Refs.Num(); i++)
	{
		FString LevelName = Refs[i].ToString();
		UE_LOG(PakLoader, Log, TEXT("Got Level: %s"), *LevelName);
		Levels.Add(LevelName);
	}
	return true;
}

bool FPakLoaderModule::GetAssetReferencesFromPak(const TSharedPtr<FPakFile>& Ptr, const FString& FileExtension, TArray<FStringAssetReference>& Result)
{
	TSet<FString> PakContent;
	FPakFile& PakFile = *Ptr;
	PakFile.FindFilesAtPath(PakContent, *PakFile.GetMountPoint(), true, false, true);
	FString ContentDir = FPaths::GameContentDir();
	FPaths::MakeStandardFilename(ContentDir);
	for (TSet<FString>::TConstIterator SetIt(PakContent); SetIt; ++SetIt)
	{
		FString AssetName = *SetIt;
		if (FileExtension.Len() == 0 || AssetName.EndsWith(FileExtension))
		{
			FString BasePath = FPaths::GetBaseFilename(AssetName, false);
			FString Dest = TEXT("/Game/") + BasePath.Mid(ContentDir.Len());
			UE_LOG(PakLoader, Log, TEXT("Asset %s => %s; ContentDir: %s, AssetName: %s, MountPoint: %s"), *BasePath, *Dest, *ContentDir, **SetIt, *PakFile.GetMountPoint());
			Result.Add(FStringAssetReference(Dest));
		}
	}
	return true;
}

bool FPakLoaderModule::UnmountPakFile(const FString& PakFilePath)
{
	bool Result = false;
	if (MountedPaks.Contains(PakFilePath))
	{
		TSharedPtr<FPakFile> Ptr;
		if (MountPakFile(PakFilePath, Ptr))
		{
			TArray<FStringAssetReference> Assets;
			GetAssetReferencesFromPak(Ptr, FString(), Assets);
			Ptr.Reset();
			bool AssetsUnloaded = true;
			const uint32 Id = UnloadId++;
			for (int32 i = 0; i < Assets.Num(); i++)
			{
				UObject* InMemory = StaticFindObject(UObject::StaticClass(), NULL, *Assets[i].ToString());
				if (InMemory)
				{
					UE_LOG(PakLoader, Log, TEXT("Asset still in memory: %s, renaming it"), *Assets[i].ToString());
					InMemory->Rename(*(Assets[i].ToString() + "-Unloaded___" + FString::FromInt(Id)), InMemory->GetOuter());
					//@TODO ensure InMemory is gc-ed
				}
			}
			if (AssetsUnloaded)
			{
				Result = PakPlatformFile->Unmount(*PakFilePath);
				if (Result)
				{
					MountedPaks.Remove(PakFilePath);
					UE_LOG(PakLoader, Log, TEXT("Unmounted: %s"), *PakFilePath);
				}
			}
		}
	}
	else
	{
		UE_LOG(PakLoader, Error, TEXT("Pak file isn't mounted: %s"), *PakFilePath);
	}
	if (!Result)
	{
		UE_LOG(PakLoader, Error, TEXT("Couldn't unmount Pak file :( %s"), *PakFilePath);
	}
	return Result;
}


bool FPakLoaderModule::MountPakFile(const FString& PakFilePath, TSharedPtr<FPakFile>& Result)
{
	bSandboxed = false;
	if (PakPlatformFile == nullptr)
	{
		IPlatformFile* File = FPlatformFileManager::Get().FindPlatformFile(FPakPlatformFile::GetTypeName());
		if (File != nullptr)
		{
			UE_LOG(PakLoader, Log, TEXT("Found existing FPakPlatformFile"));
			PakPlatformFile = static_cast<FPakPlatformFile*>(File);
		}
		else
		{
			PakPlatformFile = new FPakPlatformFile();
			IPlatformFile* LowerLevel = &FPlatformFileManager::Get().GetPlatformFile();
			const FString LowerLevelName(LowerLevel->GetName());
			if (LowerLevelName == TEXT("CachedReadFile"))
			{
				// hack: Pak must go below CachedRead
				IPlatformFile* CachedReadFile = LowerLevel;
				IPlatformFile* Physical = LowerLevel->GetLowerLevel();
				PakPlatformFile->Initialize(Physical, TEXT(""));
				CachedReadFile->Initialize(PakPlatformFile, TEXT(""));
			}
			else
			{
				PakPlatformFile->Initialize(LowerLevel, TEXT(""));
				FPlatformFileManager::Get().SetPlatformFile(*PakPlatformFile);
			}
			UE_LOG(PakLoader, Log, TEXT("Created new FPakPlatformFile above %s"), LowerLevel->GetName());
			
		}
		IPlatformFile* Top = &FPlatformFileManager::Get().GetPlatformFile();
		static FString SandBoxFile("SandBoxFile");
		while (Top != nullptr)
		{
			UE_LOG(PakLoader, Log, TEXT("%s"), Top->GetName());
			if (SandBoxFile == Top->GetName())
			{
				bSandboxed = true;
			}
			Top = Top->GetLowerLevel();
		}
	}
	FString GameContentDir(FPaths::GameContentDir());
	FPaths::MakeStandardFilename(GameContentDir);
	FString Absolute = FPaths::ConvertRelativePathToFull(GameContentDir);
	UE_LOG(PakLoader, Log, TEXT("GameContentDir: %s, full: %s"), *GameContentDir, *Absolute);
	IFileManager* const FileManager = &IFileManager::Get();
	if (!FileManager->FileExists(*PakFilePath))
	{
		UE_LOG(PakLoader, Error, TEXT("Pak file doesn't exist :( %s"), *PakFilePath);
		return false;
	}

	if (MountedPaks.Contains(PakFilePath) || PakPlatformFile->Mount(*PakFilePath, 5, *GameContentDir))
	{
		MountedPaks.Add(PakFilePath);
		TSharedPtr<FPakFile> PakFile(new FPakFile(&FPlatformFileManager::Get().GetPlatformFile(), *PakFilePath, false));
		PakFile->SetMountPoint(*GameContentDir);
		UE_LOG(PakLoader, Log, TEXT("Mounted Pak File: %s"), *PakFilePath);
		UE_LOG(PakLoader, Log, TEXT("MountPoint: %s"), *PakFile->GetMountPoint());
		Result = PakFile;
		return true;
	}
	return false;
}

bool FPakLoaderModule::GetAssetsFromPak(const FString& PakFilePath,
	TFunction<void(TSharedPtr<TArray<FStringAssetReference>>)> AssetsLoadedCallback)
{
	TSharedPtr<TArray<FStringAssetReference>> AssetsPtr(new TArray<FStringAssetReference>());
	TSharedPtr<FPakFile> Ptr;
	if (!MountPakFile(PakFilePath, Ptr))
	{
		return false;
	}
	FPakFile& PakFile = *Ptr;
	TArray<FStringAssetReference> &TargetAssets = *AssetsPtr;
	GetAssetReferencesFromPak(Ptr, FPackageName::GetAssetPackageExtension(), TargetAssets);
	const bool bAsync = false;
	if (bAsync)
	{
		StreamableManager->RequestAsyncLoad(TargetAssets,
			[=]
		{
			AssetsLoadedCallback(AssetsPtr);

		});
	}
	else // for debugging:
	{
		for (int32 i = 0; i < TargetAssets.Num(); i++)
		{
			UObject *Result = TargetAssets[i].TryLoad();
			if (Result)
			{
				UE_LOG(PakLoader, Log, TEXT("Loaded :) %s"), *TargetAssets[i].ToString());
			}
			else
			{
				FStringAssetReference C(TargetAssets[i].ToString() + "_C");
				Result = C.TryLoad();
				if (!Result)
				{
					UE_LOG(PakLoader, Log, TEXT("Not Loaded :( %s"), *TargetAssets[i].ToString());
				}
				else
				{
					TargetAssets[i] = C;
				}
			}
		}
		AssetsLoadedCallback(AssetsPtr);
	}
	return true;
}

void FPakLoaderModule::EndPlay()
{
#if WITH_EDITOR
	TSet<FString> ToRemove(MountedPaks);
	for (TSet<FString>::TConstIterator It(ToRemove); It; ++It)
	{
		UnmountPakFile(*It);
	}
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPakLoaderModule, PakLoader)
