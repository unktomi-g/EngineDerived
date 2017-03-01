// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "DeployToPakEditorPrivatePCH.h"

#include "SlateBasics.h"
#include "SlateExtras.h"

#include "DeployToPakEditorStyle.h"
#include "DeployToPakEditorCommands.h"

#include "LevelEditor.h"
#include "CookContentMenu.h"

static const FName DeployToPakEditorTabName("DeployToPakEditor");

#define LOCTEXT_NAMESPACE "FDeployToPakEditorModule"

void FDeployToPakEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FDeployToPakEditorStyle::Initialize();
	FDeployToPakEditorStyle::ReloadTextures();

	FDeployToPakEditorCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FDeployToPakEditorCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FDeployToPakEditorModule::PluginButtonClicked),
		FCanExecuteAction());

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("FileProject", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FDeployToPakEditorModule::AddMenuExtension));

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}
}

void FDeployToPakEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FDeployToPakEditorStyle::Shutdown();

	FDeployToPakEditorCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(DeployToPakEditorTabName);
}

TSharedRef<SDockTab> FDeployToPakEditorModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	FText WidgetText = FText::Format(
		LOCTEXT("WindowWidgetText", "Add code to {0} in {1} to override this window's contents"),
		FText::FromString(TEXT("FDeployToPakEditorModule::OnSpawnPluginTab")),
		FText::FromString(TEXT("DeployToPakEditor.cpp"))
		);

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			// Put your tab content here!
			SNew(SBox)
			.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(WidgetText)
		]
		];
}

void FDeployToPakEditorModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->InvokeTab(DeployToPakEditorTabName);
}

void FDeployToPakEditorModule::AddMenuExtension(FMenuBuilder& MenuBuilder)
{
	//Builder.AddMenuEntry(FDeployToPakEditorCommands::Get().OpenPluginWindow);
	MenuBuilder.AddSubMenu(
		LOCTEXT("CookProjectSubMenuLabel", "Package Content Only"),
		LOCTEXT("CookProjectSubMenuToolTip", "Deploy your content to a Pak file for a platform - cooking if necessary"),
		FNewMenuDelegate::CreateStatic(&FCookContentMenu::MakeMenu), false, FSlateIcon()
		);
}

void FDeployToPakEditorModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FDeployToPakEditorCommands::Get().OpenPluginWindow);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDeployToPakEditorModule, DeployToPakEditor)