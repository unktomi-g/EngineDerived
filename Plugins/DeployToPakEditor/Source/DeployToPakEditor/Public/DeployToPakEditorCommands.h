// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateBasics.h"
#include "DeployToPakEditorStyle.h"

class FDeployToPakEditorCommands : public TCommands<FDeployToPakEditorCommands>
{
public:

	FDeployToPakEditorCommands()
		: TCommands<FDeployToPakEditorCommands>(TEXT("DeployToPakEditor"), NSLOCTEXT("Contexts", "DeployToPakEditor", "DeployToPakEditor Plugin"), NAME_None, FDeployToPakEditorStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};