// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"
#include "IPlatformFilePak.h"
#include "Set.h"
struct FStreamableManager;
DECLARE_LOG_CATEGORY_EXTERN(PakLoader, Log, All);

class FPakLoaderModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/**
	*  Loads assets found in the Pak file at PakFilePath asynchronously and then calls the supplied callback.
	*  Note: mounts the Pak file as a side-effect.
	*/
	virtual bool GetAssetsFromPak(const FString& PakFilePath,
		TFunction<void(TSharedPtr<TArray<FStringAssetReference>>)> AssetsLoadedCallback);
	/**
	* Returns a list of assets contained in the Pak file Ptr points at having the given file extension.
	*/
	virtual bool GetAssetReferencesFromPak(const TSharedPtr<FPakFile>& Ptr, const FString& FileExtension, TArray<FStringAssetReference>& Result);
	/**
	* Returns a list of (long-form) level names found in the Pak file at PakFilePath (Note: mounts the Pak file as a side-effect).
	*/
	virtual bool GetLevelsFromPak(const FString& PakFilePath, TArray<FString>& Levels);
	/**
	* Mounts the given Pak file on the Game content folder and then returns a pointer to the FPakFile object.
	*/
	virtual bool MountPakFile(const FString& PakFilePath, TSharedPtr<FPakFile>& Result);

	/**
	* Unmounts the given (previously mounted) Pak file
	*/
	virtual bool UnmountPakFile(const FString &PakFilePath);

	virtual void EndPlay();

	void ConvertToSandBoxPath(const FString& InPath, FString* Result)
	{
		if (bSandboxed)
		{
			IFileManager& FileManager = IFileManager::Get();
			FString DiskFilename = FileManager.GetFilenameOnDisk(*InPath);
			*Result = DiskFilename;
		}
		else
		{
			*Result = InPath;
		}
	}
private:
	FStreamableManager* StreamableManager;
	FPakPlatformFile* PakPlatformFile;
	bool bSandboxed;
	TSet<FString> MountedPaks;
	uint32 UnloadId;
};