// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "DeployToPakEditorPrivatePCH.h"
#include "DeployToPakEditorCommands.h"

#define LOCTEXT_NAMESPACE "FDeployToPakEditorModule"

void FDeployToPakEditorCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "DeployToPakEditor", "Bring up DeployToPakEditor window", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE
