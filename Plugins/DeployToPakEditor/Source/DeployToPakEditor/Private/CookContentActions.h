// Note: adapted from code in engine MainFrame module

// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.


#ifndef __CookContentActions_h__
#define __CookContentActions_h__

#pragma once

#include "Engine.h"
#include "ISourceControlProvider.h"
#include "Settings/ProjectPackagingSettings.h"
#include "Http.h"

/**
 * Implementation of cook content action callback
 */
class FCookContentActionCallbacks
{

public:

	/** Cooks the project's content for the specified platform. */
	static void CookContent(const FName InPlatformInfoName);

	/** Checks whether a menu action for cooking the project's content can execute. */
	static bool CookContentCanExecute(const FName PlatformInfoName);

protected:


	/**
	 * Creates an asynchronous UAT task.
	 *
	 * @param CommandLine - The command line for the UAT task.
	 * @param PlatformDisplayName - The display name of the platform that the task is running for.
	 * @param TaskName - The human readable name of the task.
	 * @param TaskIcon - The icon for the task (to show in the notification item).
	 */
	static void CreateUatTask(const FString& CommandLine, const FString& InPlatformName, const FText& PlatformDisplayName, const FText& TaskName, const FText &TaskShortName, const FSlateBrush* TaskIcon);

	/**
	* Creates an asynchronous UE4Editor-Cmd task.
	*
	* @param CommandLine - The command line for the Editor task.
	* @param PlatformDisplayName - The display name of the platform that the task is running for.
	* @param TaskName - The human readable name of the task.
	* @param TaskIcon - The icon for the task (to show in the notification item).
	*/

	static void CreateEditorTask(const FString& CommandLine, const FString& InPlatformName, const FText& PlatformDisplayName, const FText& TaskName, const FText &TaskShortName, const FSlateBrush* TaskIcon);

	/**
	* Creates an asynchronous UnrealPak task.
	*
	* @param CommandLine - The command line for the UnrealPak task.
	* @param PlatformDisplayName - The display name of the platform that the task is running for.
	* @param TaskName - The human readable name of the task.
	* @param TaskIcon - The icon for the task (to show in the notification item).
	*/
	static void
		CreatePakTask(const FString &OutputFile, const FText& PlatformDisplayName, const FText &TaskName, const FText &ShortTaskname, const FSlateBrush* TaskIcon);


private:

	struct EventData
	{
		FString EventName;
		bool bProjectHasCode;
		double StartTime;
	};

	// Handles clicking the packager notification item's Cancel button.
	static void HandleUatCancelButtonClicked(TSharedPtr<FMonitoredProcess> PackagerProcess);

	static void HandleUatCancelButtonClicked(TWeakPtr<FMonitoredProcess> PackagerProcess);

	// Handles clicking the hyper link on a packager notification item.
	static void HandleUatHyperlinkNavigate();

	// Handles canceled packager processes.
	static void HandleUatProcessCanceled(TWeakPtr<class SNotificationItem> NotificationItemPtr, FText PlatformDisplayName, FText TaskName, EventData Event);

	// Handles the completion of a packager process.
	static void HandleUatProcessCompleted(int32 ReturnCode, bool LaunchPakTask, TWeakPtr<class SNotificationItem> NotificationItemPtr, FString TargetPlatform, FText PlatformDisplayName, FText TaskName, EventData Event);

	// Handles packager process output.
	static void HandleUatProcessOutput(FString Output, TWeakPtr<class SNotificationItem> NotificationItemPtr, FText PlatformDisplayName, FText TaskName);

private:
	static TSharedPtr<class SNotificationItem> CreateNotificationItem(FText PlatformDisplayName,  FText TaskName);

};


#endif		// __CookContentActions_h__
