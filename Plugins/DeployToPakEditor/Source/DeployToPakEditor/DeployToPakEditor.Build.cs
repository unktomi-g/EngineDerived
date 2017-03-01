// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DeployToPakEditor : ModuleRules
{
    public DeployToPakEditor(TargetInfo Target)
    {

        PublicIncludePaths.AddRange(
            new string[] {
                "DeployToPakEditor/Public"
				// ... add public include paths required here ...
			}
            );


        PrivateIncludePaths.AddRange(
            new string[] {
                "DeployToPakEditor/Private",
				// ... add other private include paths required here ...
			}
            );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "DesktopPlatform",
                "ProjectTargetPlatformEditor",
                "TargetPlatform",
                "GameProjectGeneration",
                "EngineSettings",
                "EditorStyle",
                "UnrealEd",
                "Analytics"
				// ... add other public dependencies that you statically link with here ...
			}
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Projects",
                "InputCore",
                "UnrealEd",
                "LevelEditor",
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "Http"
				// ... add private dependencies that you statically link with here ...	
			}
            );


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
				// ... add any modules that your module loads dynamically here ...
			}
            );
    }
}
