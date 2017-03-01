// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"
#include "DeployToPakEditorSettings.h"

class FToolBarBuilder;
class FMenuBuilder;

class FDeployToPakEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();

private:

	void AddToolbarExtension(FToolBarBuilder& Builder);
	void AddMenuExtension(FMenuBuilder& Builder);

	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

public:
	

	FString GetPakFileUploadURL()
	{
		return EditorSettings.Get() != nullptr ? EditorSettings.Get()->PakFileUploadFolderURL : FString();
	}

	FString GetAuthor()
	{
		return Author;
	}

	const TArray<FAssetChannel>& GetAssets()
	{
		return EditorSettings.Get() != nullptr ? EditorSettings.Get()->Assets : EmptyAssets;
	}

	static FDeployToPakEditorModule& Get()
	{
		return FModuleManager::LoadModuleChecked< FDeployToPakEditorModule >("DeployToPakEditor");
	}

	void SetEditorSettings(UDeployToPakEditorSettings *InEditorSettings)
	{
		EditorSettings = InEditorSettings;
	}

private:
	TSharedPtr<class FUICommandList> PluginCommands;
	TWeakObjectPtr<UDeployToPakEditorSettings> EditorSettings;
	TArray<FAssetChannel> EmptyAssets;
	FString Author;
};
