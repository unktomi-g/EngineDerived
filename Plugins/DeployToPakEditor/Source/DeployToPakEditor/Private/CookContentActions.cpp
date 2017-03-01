// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.


#include "DeployToPakEditorPrivatePCH.h"
#include "CookContentActions.h"
#include "MessageLog.h"
#include "SDockTab.h"
#include "SNotificationList.h"
#include "NotificationManager.h"
#include "GenericCommands.h"
#include "EngineBuildSettings.h"
#include "SourceCodeNavigation.h"
#include "SOutputLogDialog.h"
#include "CookContentMenu.h"
#include "CookContentActions.h"
#include "CoreMisc.h"
#include "EditorStyleSet.h"
#include "Settings/EditorSettings.h"
#include "UnrealEd.h"
#include "UnrealEdMisc.h"
#include "Engine/ObjectLibrary.h"
#include "AnalyticsEventAttribute.h"
#include "TickableEditorObject.h"
#define LOCTEXT_NAMESPACE "CookContentActions"
#include "Runtime/Launch/Resources/Version.h"

#define ENGINE_COOKED_VERSION_STRING \
VERSION_STRINGIFY(ENGINE_MAJOR_VERSION) \
VERSION_TEXT(".") \
VERSION_STRINGIFY(ENGINE_MINOR_VERSION) \
VERSION_TEXT(".") \
VERSION_STRINGIFY(ENGINE_PATCH_VERSION)

DEFINE_LOG_CATEGORY_STATIC(CookContentActions, Log, All);



/**
 * Gets compilation flags for UAT for this system.
 */
const TCHAR* GetUATCompilationFlags()
{
	// We never want to compile editor targets when invoking UAT in this context.
	// If we are installed or don't have a compiler, we must assume we have a precompiled UAT.
	return (FApp::GetEngineIsPromotedBuild() || FApp::IsEngineInstalled())
		? TEXT("-nocompile -nocompileeditor")
		: TEXT("-nocompileeditor");
}

FString GetCookingOptionalParams()
{
	FString OptionalParams;
	const UProjectPackagingSettings* const PackagingSettings = GetDefault<UProjectPackagingSettings>();
	if (PackagingSettings->bCookAll)
	{
		OptionalParams += TEXT(" -CookAll");
		// maps only flag only affects cook all
		if (PackagingSettings->bCookMapsOnly)
		{
			OptionalParams += TEXT(" -CookMapsOnly");
		}
	}

	if (PackagingSettings->bSkipEditorContent)
	{
		OptionalParams += TEXT(" -SKIPEDITORCONTENT");
	}
	return OptionalParams;
}

static TArray<FString> GetAllMapNames()
{
	TArray<FString> Names = TArray<FString>();
	const TArray<FDeployToPakAsset>& Assets = FDeployToPakEditorModule::Get().GetAssets();
	TSet<FString> Maps;
	for (int32 i = 0; i < Assets.Num(); i++) {
		for (int32 j = 0; j < Assets[i].Maps.Num(); j++) {
			Maps.Add(FPackageName::ObjectPathToPackageName(Assets[i].Maps[j].ToString()));
		}
	}
	for (TSet<FString>::TConstIterator It(Maps);  It; ++It)
	{
		Names.Add(*It);
	}
	return Names;
}

static TArray<FString> GetAllBlueprintNames()
{
	TArray<FString> Names = TArray<FString>();
	const TArray<FDeployToPakAsset>& Assets = FDeployToPakEditorModule::Get().GetAssets();
	TSet<FString> Maps;
	for (int32 i = 0; i < Assets.Num(); i++) {
		for (int32 j = 0; j < Assets[i].Blueprints.Num(); j++) {
			Maps.Add(Assets[i].Blueprints[j].ToString());
		}
	}
	for (TSet<FString>::TConstIterator It(Maps); It; ++It)
	{
		Names.Add(*It);
	}
	return Names;;
}


void FCookContentActionCallbacks::CookContent(const FName InPlatformInfoName)
{
	const PlatformInfo::FPlatformInfo* const PlatformInfo = PlatformInfo::FindPlatformInfo(InPlatformInfoName);
	if (!PlatformInfo)
	{
		// Editor mode	
		// hack: create a file at the top level to ensure unrealpak doesn't try to change the mount point
		FString ContentFolder = FPaths::GameContentDir();
		FString PlaceHolder = ContentFolder + TEXT("unrealpakPlaceholder");
		if (!FFileHelper::SaveStringToFile(TEXT("force unrealpak to include the folder that contains this"),
			*PlaceHolder))
		{
			UE_LOG(CookContentActions, Error, TEXT("failed to create placeholder :( %s"), *PlaceHolder);
		}
		CreatePakTask(InPlatformInfoName.ToString(), LOCTEXT("PackingContentTaskName", "Editor"), LOCTEXT("PakingContentTaskName", "Packaging content"), LOCTEXT("PakingTaskName", "Packing"), FEditorStyle::GetBrush(TEXT("MainFrame.CookContent")));
		return;
	}

	if (FInstalledPlatformInfo::Get().IsPlatformMissingRequiredFile(PlatformInfo->BinaryFolderName))
	{
		if (!FInstalledPlatformInfo::OpenInstallerOptions())
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("MissingPlatformFilesCook", "Missing required files to cook for this platform."));
		}
		return;
	}

	FString OptionalParams;

	if (!FModuleManager::LoadModuleChecked<IProjectTargetPlatformEditorModule>("ProjectTargetPlatformEditor").ShowUnsupportedTargetWarning(PlatformInfo->VanillaPlatformName))
	{
		return;
	}

	if (PlatformInfo->SDKStatus == PlatformInfo::EPlatformSDKStatus::NotInstalled)
	{
		//IMainFrameModule& Module = FModuleManager::GetModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
		//Module.BroadcastCookContentSDKNotInstalled(PlatformInfo->TargetPlatformName.ToString(), PlatformInfo->SDKTutorial);
		return;
	}

	// Append any extra UAT flags specified for this platform flavor
	if (!PlatformInfo->UATCommandLine.IsEmpty())
	{
		OptionalParams += TEXT(" ");
		OptionalParams += PlatformInfo->UATCommandLine;
	}
	else
	{
		OptionalParams += TEXT(" -targetplatform=");
		OptionalParams += *PlatformInfo->TargetPlatformName.ToString();
	}

	OptionalParams += GetCookingOptionalParams();

	const bool bRunningDebug = FParse::Param(FCommandLine::Get(), TEXT("debug"));

	if (bRunningDebug)
	{
		OptionalParams += TEXT(" -UseDebugParamForEditorExe");
	}
	TArray<FString> MapFiles = GetAllMapNames();
	//IFileManager::Get().FindFilesRecursive(MapFiles, FPaths::GameContentDir(), TEXT(".umap"), true, false);
	FString MapsToCook;
	FString Sep = "";
	for (int32 i = 0; i < MapFiles.Num(); i++)
	{
		MapsToCook += Sep;
		Sep = "+";
		MapsToCook +=MapFiles[i];
	}
	TArray<FString> BlueprintFiles = GetAllBlueprintNames();
	FString DirsToCook;
	Sep = "";
	TSet<FString> Dirs;
	for (int32 i = 0; i < BlueprintFiles.Num(); i++)
	{
		FString Dir = FPaths::GetPath(BlueprintFiles[i]);
		if (!Dir.StartsWith(TEXT("/Game/")))
		{
			UE_LOG(CookContentActions, Error, TEXT("Error: asset %s is not under the Content folder - it will not be included."), *BlueprintFiles[i]);
			continue;
		}
		Dir = Dir.RightChop(6); // Remove leading "/Game/"
		bool bExists = false;
		Dirs.Add(Dir, &bExists);
		if (!bExists)
		{
			DirsToCook += Sep;
			Sep = "+";
			DirsToCook += Dir;
		}
	}

	FString TargetPlatformInfoName = PlatformInfo->TargetPlatformName.ToString();
	FString ProjectPath = FPaths::IsProjectFilePathSet() ? FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()) : FPaths::RootDir() / FApp::GetGameName() / FApp::GetGameName() + TEXT(".uproject");
	
	if (true)
	{
		// \Engine\Binaries\Win64\UE4Editor-Cmd.exe E:\Tango2\TestPak\TestPak.uproject -run=Cook  -TargetPlatform=WindowsNoEditor -fileopenlog -unversioned -iterate -cookall -compress -skipeditorcontent
		FString CommandLine = FString::Printf(TEXT("\"%s\" -run=Cook -targetplatform=%s -maps=\"%s\" -cookdir=\"%s\" -compressed -iterate -stdout -FORCELOGFLUSH -skipeditorcontent"),
			*ProjectPath, *TargetPlatformInfoName, *MapsToCook, *DirsToCook);
		UE_LOG(CookContentActions, Log, TEXT("Editor cook command line: %s"), *CommandLine);
		CreateEditorTask(CommandLine, TargetPlatformInfoName,
			PlatformInfo->DisplayName,
			LOCTEXT("CookingContentTaskName", "Cooking content"),
			LOCTEXT("CookingTaskName", "Cooking"),
			FEditorStyle::GetBrush(TEXT("MainFrame.CookContent")));
	}
	else
	{
		FString CommandLine = FString::Printf(TEXT("BuildCookRun %s%s -nop4 -project=\"%s\" -NoCompile -maps=\"%s\" -cook -SkipEditorContent -CookAll -ue4exe=%s %s"),
			GetUATCompilationFlags(),
			FApp::IsEngineInstalled() ? TEXT(" -installed") : TEXT(""),
			*ProjectPath,
			*MapsToCook,
			*FUnrealEdMisc::Get().GetExecutableForCommandlets(),
			*OptionalParams
			);

		CreateUatTask(
			CommandLine, TargetPlatformInfoName, PlatformInfo->DisplayName, 
			LOCTEXT("CookingContentTaskName", "Cooking content"), LOCTEXT("CookingTaskName", "Cooking"), 
			FEditorStyle::GetBrush(TEXT("MainFrame.CookContent"))
		);
	}
}

bool FCookContentActionCallbacks::CookContentCanExecute(const FName PlatformInfoName)
{
	return true;
}

/* FCookContentActionCallbacks implementation
 *****************************************************************************/

void FCookContentActionCallbacks::CreatePakTask(const FString& TargetPlatform, const FText& PlatformDisplayName, const FText& TaskName, const FText &TaskShortName, const FSlateBrush* TaskIcon)
{
#if PLATFORM_WINDOWS
	FString CmdExe = TEXT("cmd.exe");
#elif PLATFORM_LINUX
	FString CmdExe = TEXT("/bin/bash");
#else
	FString CmdExe = TEXT("/bin/sh");
#endif
	FString CurrentPlatform; // hack
#if PLATFORM_WINDOWS
	CurrentPlatform = "Win64";
#elif PLATFORM_LINUX
	CurrentPlatform = "Linux";
#else
	CurrentPlatform = "Mac";
#endif
	FString U4PakPath = FPaths::ConvertRelativePathToFull(FPaths::EngineDir() / "Binaries" / CurrentPlatform / "UnrealPak");
	FGameProjectGenerationModule& GameProjectModule = FModuleManager::LoadModuleChecked<FGameProjectGenerationModule>(TEXT("GameProjectGeneration"));
	bool bHavCode = GameProjectModule.Get().ProjectHasCodeFiles();
	FString GameName = FApp::GetGameName();
	FString ContentFolder;
	FString OutputFile;
	FString VersionTag;
	if (TargetPlatform == TEXT("Editor"))
	{
		ContentFolder = FPaths::ConvertRelativePathToFull(FPaths::GameContentDir());
	}
	else
	{
		ContentFolder = FPaths::ConvertRelativePathToFull(FPaths::GameSavedDir() / "Cooked" / TargetPlatform  / GameName / "Content/");
		//FString ResponseFile = FPaths::ConvertRelativePathToFull(FPaths::GameSavedDir() / "Content-Pak.txt");
		VersionTag = FString("_") + ENGINE_COOKED_VERSION_STRING;
	}
	OutputFile = FPaths::GameSavedDir() / "Cooked" / GameName + "-" + TargetPlatform + VersionTag + "-Content.pak";

	FString CommandLine = FString::Printf(TEXT("\"%s\" -create=\"%s\" -compress"), *OutputFile, *ContentFolder);
#if PLATFORM_WINDOWS
	FString FullCommandLine = FString::Printf(TEXT("/c \"\"%s\" %s\""), *U4PakPath, *CommandLine);
#else
	FString FullCommandLine = FString::Printf(TEXT("\"%s\" %s"), *U4PakPath, *CommandLine);
#endif	


	UE_LOG(CookContentActions, Log, TEXT("Pak command: %s"), *FullCommandLine);
	TSharedPtr<FMonitoredProcess> PakProcess(new FMonitoredProcess(CmdExe, FullCommandLine, true));

	// create notification item
	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("Platform"), PlatformDisplayName);
	Arguments.Add(TEXT("TaskName"), TaskName);
	FNotificationInfo Info(FText::Format(LOCTEXT("UatTaskInProgressNotification", "{TaskName} for {Platform}..."), Arguments));

	Info.Image = TaskIcon;
	Info.bFireAndForget = false;
	Info.ExpireDuration = 3.0f;
	Info.Hyperlink = FSimpleDelegate::CreateStatic(&FCookContentActionCallbacks::HandleUatHyperlinkNavigate);
	Info.HyperlinkText = LOCTEXT("ShowOutputLogHyperlink", "Show Output Log");
	Info.ButtonDetails.Add(
		FNotificationButtonInfo(
			LOCTEXT("UatTaskCancel", "Cancel"),
			LOCTEXT("UatTaskCancelToolTip", "Cancels execution of this task."),
			FSimpleDelegate::CreateStatic(&FCookContentActionCallbacks::HandleUatCancelButtonClicked, PakProcess)
		)
	);


	TSharedPtr<SNotificationItem> NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);

	if (!NotificationItem.IsValid())
	{
		return;
	}

	FString EventName = TEXT("Editor.Package");
	FEditorAnalytics::ReportEvent(EventName + TEXT(".Start"), PlatformDisplayName.ToString(), false);

	NotificationItem->SetCompletionState(SNotificationItem::CS_Pending);
	TWeakPtr<SNotificationItem> NotificationItemPtr(NotificationItem);

	EventData Data;
	Data.StartTime = FPlatformTime::Seconds();
	Data.EventName = TEXT("Editor.Package");
	Data.bProjectHasCode = false;
	PakProcess->OnCanceled().BindStatic(&FCookContentActionCallbacks::HandleUatProcessCanceled, NotificationItemPtr, PlatformDisplayName, TaskShortName, Data);
	PakProcess->OnCompleted().BindStatic(&FCookContentActionCallbacks::HandleUatProcessCompleted, false, NotificationItemPtr, TargetPlatform, PlatformDisplayName, TaskShortName, Data);
	PakProcess->OnOutput().BindStatic(&FCookContentActionCallbacks::HandleUatProcessOutput, NotificationItemPtr, PlatformDisplayName, TaskShortName);

	TWeakPtr<FMonitoredProcess> PakProcessPtr(PakProcess); 
	FEditorDelegates::OnShutdownPostPackagesSaved.Add(FSimpleDelegate::CreateStatic(&FCookContentActionCallbacks::HandleUatCancelButtonClicked, PakProcessPtr)); 
	if (PakProcess->Launch())
	{
		GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileStart_Cue.CompileStart_Cue"));
	}
	else
	{
		GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileFailed_Cue.CompileFailed_Cue"));

		NotificationItem->SetText(LOCTEXT("FailedToLaunchUnrealPakNotification", "Failed to launch UnrealPak!"));
		NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);
		NotificationItem->ExpireAndFadeout();

		TArray<FAnalyticsEventAttribute> ParamArray;
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("Time"), 0.0));
		FEditorAnalytics::ReportEvent(EventName + TEXT(".Failed"), PlatformDisplayName.ToString(), false, EAnalyticsErrorCodes::UATLaunchFailure, ParamArray);
	}
}

void FCookContentActionCallbacks::CreateEditorTask(const FString& CommandLine, const FString& InPlatformName, const FText& PlatformDisplayName, const FText& TaskName, const FText &TaskShortName, const FSlateBrush* TaskIcon)
{

	FString CmdExe = FUnrealEdMisc::Get().GetExecutableForCommandlets();
	FGameProjectGenerationModule& GameProjectModule = FModuleManager::LoadModuleChecked<FGameProjectGenerationModule>(TEXT("GameProjectGeneration"));
	bool bHasCode = GameProjectModule.Get().ProjectHasCodeFiles();

	FString FullCommandLine = CommandLine;

	TSharedPtr<FMonitoredProcess> UatProcess = MakeShareable(new FMonitoredProcess(CmdExe, FullCommandLine, true));

	// create notification item
	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("Platform"), PlatformDisplayName);
	Arguments.Add(TEXT("TaskName"), TaskName);
	FNotificationInfo Info(FText::Format(LOCTEXT("UatTaskInProgressNotification", "{TaskName} for {Platform}..."), Arguments));

	Info.Image = TaskIcon;
	Info.bFireAndForget = false;
	Info.ExpireDuration = 3.0f;
	Info.Hyperlink = FSimpleDelegate::CreateStatic(&FCookContentActionCallbacks::HandleUatHyperlinkNavigate);
	Info.HyperlinkText = LOCTEXT("ShowOutputLogHyperlink", "Show Output Log");
	Info.ButtonDetails.Add(
		FNotificationButtonInfo(
			LOCTEXT("UatTaskCancel", "Cancel"),
			LOCTEXT("UatTaskCancelToolTip", "Cancels execution of this task."),
			FSimpleDelegate::CreateStatic(&FCookContentActionCallbacks::HandleUatCancelButtonClicked, UatProcess)
			)
		);

	TSharedPtr<SNotificationItem> NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);

	if (!NotificationItem.IsValid())
	{
		return;
	}

	FString EventName = (CommandLine.Contains(TEXT("-package")) ? TEXT("Editor.Package") : TEXT("Editor.Cook"));
	FEditorAnalytics::ReportEvent(EventName + TEXT(".Start"), PlatformDisplayName.ToString(), bHasCode);

	NotificationItem->SetCompletionState(SNotificationItem::CS_Pending);

	// launch the packager
	TWeakPtr<SNotificationItem> NotificationItemPtr(NotificationItem);

	EventData Data;
	Data.StartTime = FPlatformTime::Seconds();
	Data.EventName = EventName;
	Data.bProjectHasCode = false;
	UatProcess->OnCanceled().BindStatic(&FCookContentActionCallbacks::HandleUatProcessCanceled, NotificationItemPtr, PlatformDisplayName, TaskShortName, Data);
	UatProcess->OnCompleted().BindStatic(&FCookContentActionCallbacks::HandleUatProcessCompleted, true, NotificationItemPtr, InPlatformName, PlatformDisplayName, TaskShortName, Data);
	UatProcess->OnOutput().BindStatic(&FCookContentActionCallbacks::HandleUatProcessOutput, NotificationItemPtr, PlatformDisplayName, TaskShortName);

	TWeakPtr<FMonitoredProcess> UatProcessPtr(UatProcess);
	FEditorDelegates::OnShutdownPostPackagesSaved.Add(FSimpleDelegate::CreateStatic(&FCookContentActionCallbacks::HandleUatCancelButtonClicked, UatProcessPtr));

	if (UatProcess->Launch())
	{
		GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileStart_Cue.CompileStart_Cue"));
	}
	else
	{
		GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileFailed_Cue.CompileFailed_Cue"));

		NotificationItem->SetText(LOCTEXT("UatLaunchFailedNotification", "Failed to launch Unreal Automation Tool (UAT)!"));
		NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);
		NotificationItem->ExpireAndFadeout();

		TArray<FAnalyticsEventAttribute> ParamArray;
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("Time"), 0.0));
		FEditorAnalytics::ReportEvent(EventName + TEXT(".Failed"), PlatformDisplayName.ToString(), bHasCode, EAnalyticsErrorCodes::UATLaunchFailure, ParamArray);
	}
}


void FCookContentActionCallbacks::CreateUatTask(const FString& CommandLine, const FString& InPlatformName, const FText& PlatformDisplayName, const FText& TaskName, const FText &TaskShortName, const FSlateBrush* TaskIcon)
{
	// make sure that the UAT batch file is in place
#if PLATFORM_WINDOWS
	FString RunUATScriptName = TEXT("RunUAT.bat");
	FString CmdExe = TEXT("cmd.exe");
#elif PLATFORM_LINUX
	FString RunUATScriptName = TEXT("RunUAT.sh");
	FString CmdExe = TEXT("/bin/bash");
#else
	FString RunUATScriptName = TEXT("RunUAT.command");
	FString CmdExe = TEXT("/bin/sh");
#endif

	FString UatPath = FPaths::ConvertRelativePathToFull(FPaths::EngineDir() / TEXT("Build/BatchFiles") / RunUATScriptName);
	FGameProjectGenerationModule& GameProjectModule = FModuleManager::LoadModuleChecked<FGameProjectGenerationModule>(TEXT("GameProjectGeneration"));
	bool bHasCode = GameProjectModule.Get().ProjectHasCodeFiles();

	if (!FPaths::FileExists(UatPath))
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("File"), FText::FromString(UatPath));
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("RequiredFileNotFoundMessage", "A required file could not be found:\n{File}"), Arguments));

		TArray<FAnalyticsEventAttribute> ParamArray;
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("Time"), 0.0));
		FString EventName = (CommandLine.Contains(TEXT("-package")) ? TEXT("Editor.Package") : TEXT("Editor.Cook"));
		FEditorAnalytics::ReportEvent(EventName + TEXT(".Failed"), PlatformDisplayName.ToString(), bHasCode, EAnalyticsErrorCodes::UATNotFound, ParamArray);

		return;
	}

#if PLATFORM_WINDOWS
	FString FullCommandLine = FString::Printf(TEXT("/c \"\"%s\" %s\""), *UatPath, *CommandLine);
#else
	FString FullCommandLine = FString::Printf(TEXT("\"%s\" %s"), *UatPath, *CommandLine);
#endif

	TSharedPtr<FMonitoredProcess> UatProcess = MakeShareable(new FMonitoredProcess(CmdExe, FullCommandLine, true));

	// create notification item
	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("Platform"), PlatformDisplayName);
	Arguments.Add(TEXT("TaskName"), TaskName);
	FNotificationInfo Info(FText::Format(LOCTEXT("UatTaskInProgressNotification", "{TaskName} for {Platform}..."), Arguments));

	Info.Image = TaskIcon;
	Info.bFireAndForget = false;
	Info.ExpireDuration = 3.0f;
	Info.Hyperlink = FSimpleDelegate::CreateStatic(&FCookContentActionCallbacks::HandleUatHyperlinkNavigate);
	Info.HyperlinkText = LOCTEXT("ShowOutputLogHyperlink", "Show Output Log");
	Info.ButtonDetails.Add(
		FNotificationButtonInfo(
			LOCTEXT("UatTaskCancel", "Cancel"),
			LOCTEXT("UatTaskCancelToolTip", "Cancels execution of this task."),
			FSimpleDelegate::CreateStatic(&FCookContentActionCallbacks::HandleUatCancelButtonClicked, UatProcess)
			)
		);

	TSharedPtr<SNotificationItem> NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);

	if (!NotificationItem.IsValid())
	{
		return;
	}

	FString EventName = (CommandLine.Contains(TEXT("-package")) ? TEXT("Editor.Package") : TEXT("Editor.Cook"));
	FEditorAnalytics::ReportEvent(EventName + TEXT(".Start"), PlatformDisplayName.ToString(), bHasCode);

	NotificationItem->SetCompletionState(SNotificationItem::CS_Pending);

	// launch the packager
	TWeakPtr<SNotificationItem> NotificationItemPtr(NotificationItem);

	EventData Data;
	Data.StartTime = FPlatformTime::Seconds();
	Data.EventName = EventName;
	Data.bProjectHasCode = false;
	UatProcess->OnCanceled().BindStatic(&FCookContentActionCallbacks::HandleUatProcessCanceled, NotificationItemPtr, PlatformDisplayName, TaskShortName, Data);
	UatProcess->OnCompleted().BindStatic(&FCookContentActionCallbacks::HandleUatProcessCompleted, true, NotificationItemPtr, InPlatformName, PlatformDisplayName, TaskShortName, Data);
	UatProcess->OnOutput().BindStatic(&FCookContentActionCallbacks::HandleUatProcessOutput, NotificationItemPtr, PlatformDisplayName, TaskShortName);
	
	TWeakPtr<FMonitoredProcess> UatProcessPtr(UatProcess);
	FEditorDelegates::OnShutdownPostPackagesSaved.Add(FSimpleDelegate::CreateStatic(&FCookContentActionCallbacks::HandleUatCancelButtonClicked, UatProcessPtr));

	if (UatProcess->Launch())
	{
		GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileStart_Cue.CompileStart_Cue"));
	}
	else
	{
		GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileFailed_Cue.CompileFailed_Cue"));

		NotificationItem->SetText(LOCTEXT("UatLaunchFailedNotification", "Failed to launch Unreal Automation Tool (UAT)!"));
		NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);
		NotificationItem->ExpireAndFadeout();

		TArray<FAnalyticsEventAttribute> ParamArray;
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("Time"), 0.0));
		FEditorAnalytics::ReportEvent(EventName + TEXT(".Failed"), PlatformDisplayName.ToString(), bHasCode, EAnalyticsErrorCodes::UATLaunchFailure, ParamArray);
	}
}

/* FCookContentActionCallbacks callbacks
 *****************************************************************************/

class FCookContentActionsNotificationTask
{
public:

	FCookContentActionsNotificationTask(TWeakPtr<SNotificationItem> InNotificationItemPtr, SNotificationItem::ECompletionState InCompletionState, const FText& InText)
		: CompletionState(InCompletionState)
		, NotificationItemPtr(InNotificationItemPtr)
		, Text(InText)
	{ }

	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		if (NotificationItemPtr.IsValid())
		{
			if (CompletionState == SNotificationItem::CS_Fail)
			{
				GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileFailed_Cue.CompileFailed_Cue"));
			}
			else
			{
				GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileSuccess_Cue.CompileSuccess_Cue"));
			}

			TSharedPtr<SNotificationItem> NotificationItem = NotificationItemPtr.Pin();
			NotificationItem->SetText(Text);
			NotificationItem->SetCompletionState(CompletionState);
			NotificationItem->ExpireAndFadeout();
		}
	}

	static ESubsequentsMode::Type GetSubsequentsMode() { return ESubsequentsMode::TrackSubsequents; }
	ENamedThreads::Type GetDesiredThread() { return ENamedThreads::GameThread; }
	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FCookContentActionsNotificationTask, STATGROUP_TaskGraphTasks);
	}

private:

	SNotificationItem::ECompletionState CompletionState;
	TWeakPtr<SNotificationItem> NotificationItemPtr;
	FText Text;
};


void FCookContentActionCallbacks::HandleUatHyperlinkNavigate()
{
	FGlobalTabmanager::Get()->InvokeTab(FName("OutputLog"));
}


void FCookContentActionCallbacks::HandleUatCancelButtonClicked(TSharedPtr<FMonitoredProcess> PackagerProcess)
{
	if (PackagerProcess.IsValid())
	{
		PackagerProcess->Cancel(true);
	}
}

void FCookContentActionCallbacks::HandleUatCancelButtonClicked(TWeakPtr<FMonitoredProcess> PackagerProcessPtr)
{
	TSharedPtr<FMonitoredProcess> PackagerProcess = PackagerProcessPtr.Pin();
	if (PackagerProcess.IsValid())
	{
		PackagerProcess->Cancel(true);
	}
}

void FCookContentActionCallbacks::HandleUatProcessCanceled(TWeakPtr<SNotificationItem> NotificationItemPtr, FText PlatformDisplayName, FText TaskName, EventData Event)
{
	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("Platform"), PlatformDisplayName);
	Arguments.Add(TEXT("TaskName"), TaskName);

	TGraphTask<FCookContentActionsNotificationTask>::CreateTask().ConstructAndDispatchWhenReady(
		NotificationItemPtr,
		SNotificationItem::CS_Fail,
		FText::Format(LOCTEXT("UatProcessFailedNotification", "{TaskName} canceled!"), Arguments)
		);

	TArray<FAnalyticsEventAttribute> ParamArray;
	ParamArray.Add(FAnalyticsEventAttribute(TEXT("Time"), FPlatformTime::Seconds() - Event.StartTime));
	FEditorAnalytics::ReportEvent(Event.EventName + TEXT(".Canceled"), PlatformDisplayName.ToString(), Event.bProjectHasCode, ParamArray);
	//	FMessageLog("PackagingResults").Warning(FText::Format(LOCTEXT("UatProcessCanceledMessageLog", "{TaskName} for {Platform} canceled by user"), Arguments));
}


/**
 * Helper class to deal with packaging issues encountered in UAT.
 **/
class FPackagingErrorHandler
{
private:

	/**
	 * Create a message to send to the Message Log.
	 *
	 * @Param MessageString - The error we wish to send to the Message Log.
	 * @Param MessageType - The severity of the message, i.e. error, warning etc.
	 **/
	static void AddMessageToMessageLog(FString MessageString, EMessageSeverity::Type MessageType)
	{
		FText MsgText = FText::FromString(MessageString);

		TSharedRef<FTokenizedMessage> Message = FTokenizedMessage::Create(MessageType);
		Message->AddToken(FTextToken::Create(MsgText));

		FMessageLog MessageLog("PackagingResults");
		MessageLog.AddMessage(Message);
	}

	/**
	 * Send Error to the Message Log.
	 *
	 * @Param MessageString - The error we wish to send to the Message Log.
	 * @Param MessageType - The severity of the message, i.e. error, warning etc.
	 **/
	static void SyncMessageWithMessageLog(FString MessageString, EMessageSeverity::Type MessageType)
	{
		DECLARE_CYCLE_STAT(TEXT("FSimpleDelegateGraphTask.SendPackageErrorToMessageLog"),
		STAT_FSimpleDelegateGraphTask_SendPackageErrorToMessageLog,
			STATGROUP_TaskGraphTasks);

		// Remove any new line terminators
		MessageString.ReplaceInline(TEXT("\r"), TEXT(""));
		MessageString.ReplaceInline(TEXT("\n"), TEXT(""));

		/**
		 * Dispatch the error from packaging to the message log.
		 **/
		FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
			FSimpleDelegateGraphTask::FDelegate::CreateStatic(&FPackagingErrorHandler::AddMessageToMessageLog, MessageString, MessageType),
			GET_STATID(STAT_FSimpleDelegateGraphTask_SendPackageErrorToMessageLog),
			nullptr, ENamedThreads::GameThread
			);
	}

public:
	/**
	 * Determine if the output is an error we wish to send to the Message Log.
	 *
	 * @Param UATOutput - The current line of output from the UAT package process.
	 **/
	static void ProcessAndHandleCookErrorOutput(FString UATOutput)
	{
		FString LhsUATOutputMsg, ParsedCookIssue;

		// note: CookResults:Warning: actually outputs some unhandled errors.
		if (UATOutput.Split(TEXT("CookResults:Warning: "), &LhsUATOutputMsg, &ParsedCookIssue))
		{
			SyncMessageWithMessageLog(ParsedCookIssue, EMessageSeverity::Warning);
		}

		if (UATOutput.Split(TEXT("CookResults:Error: "), &LhsUATOutputMsg, &ParsedCookIssue))
		{
			SyncMessageWithMessageLog(ParsedCookIssue, EMessageSeverity::Error);
		}
	}

	/**
	 * Send the UAT Packaging error message to the Message Log.
	 *
	 * @Param ErrorCode - The UAT return code we received and wish to display the error message for.
	 **/
	static void SendPackagingErrorToMessageLog(int32 ErrorCode)
	{
		SyncMessageWithMessageLog(FEditorAnalytics::TranslateErrorCode(ErrorCode), EMessageSeverity::Error);
	}
};

class RunOnMainThread : public FTickableEditorObject 
{
public:
	virtual void Tick(float DeltaSeconds) override
	{
		Run();
		delete this;
	}
	virtual bool IsTickable() const override 
	{
		return true;
	}

	/** return the stat id to use for this tickable **/
	virtual TStatId GetStatId() const override 
	{
		return StatId;
	}
	RunOnMainThread(const TFunction<void()> InRun) : Run(InRun) {}
private:
	TFunction<void()> Run;
	static TStatId StatId;
	
};

TStatId RunOnMainThread::StatId;


class FFileUploader
{
	TSharedPtr<SNotificationItem> NotificationItem;
public:
	FFileUploader(const TSharedPtr<SNotificationItem>& InNotificationItem) : NotificationItem(InNotificationItem) {}
	void HandleHttpUploadFileComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
	{		
		if (!bSucceeded || (HttpResponse.IsValid() && HttpResponse->GetResponseCode() != 200))
		{
			UE_LOG(CookContentActions, Error, TEXT("Upload to %s failed: %d"), *HttpRequest->GetURL(), HttpResponse.IsValid() ? HttpResponse->GetResponseCode() : -1);
			GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileFailed_Cue.CompileFailed_Cue"));
			NotificationItem->SetText(LOCTEXT("HttpUploadPakFailedNotification", "Failed to upload Pak file!"));
			NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);	
			FPlatformProcess::ExploreFolder(*FPaths::ConvertRelativePathToFull(FPaths::GameSavedDir() / "Cooked"));
		}
		else
		{		
			UE_LOG(CookContentActions, Log, TEXT("Completed upload of %s"), *HttpRequest->GetURL());
			GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileSuccess_Cue.CompileSuccess_Cue"));
			NotificationItem->SetText(LOCTEXT("HttpUploadPakSucceededNotification", "Completed uploading Pak file!"));
			NotificationItem->SetCompletionState(SNotificationItem::CS_Success);
		}
		NotificationItem->ExpireAndFadeout();
		delete this;
	}
};

TSharedPtr<SNotificationItem> FCookContentActionCallbacks::CreateNotificationItem(FText PlatformDisplayName, FText TaskName)
{
	// create notification item
	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("Platform"), PlatformDisplayName);
	Arguments.Add(TEXT("TaskName"), TaskName);
	FNotificationInfo Info(FText::Format(LOCTEXT("UatTaskInProgressNotification", "{TaskName} for {Platform}..."), Arguments));

	Info.Image = FEditorStyle::GetBrush(TEXT("MainFrame.CookContent"));
	Info.bFireAndForget = false;
	Info.ExpireDuration = 3.0f;
	Info.Hyperlink = FSimpleDelegate::CreateStatic(&FCookContentActionCallbacks::HandleUatHyperlinkNavigate);
	Info.HyperlinkText = LOCTEXT("ShowOutputLogHyperlink", "Show Output Log");
	/*
	Info.ButtonDetails.Add(
		FNotificationButtonInfo(
			LOCTEXT("UatTaskCancel", "Cancel"),
			LOCTEXT("UatTaskCancelToolTip", "Cancels execution of this task."),
			FSimpleDelegate::CreateStatic(&FCookContentActionCallbacks::HandleUatCancelButtonClicked, UatProcess)
		)
	);
	*/

	TSharedPtr<SNotificationItem> NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);

	if (!NotificationItem.IsValid())
	{
		return NotificationItem;
	}

	NotificationItem->SetCompletionState(SNotificationItem::CS_Pending);

	return NotificationItem;
}



DECLARE_CYCLE_STAT(TEXT("Requesting FCookContentActionCallbacks::HandleUatProcessCompleted message dialog to present the error message"), STAT_FCookContentActionCallbacks_HandleUatProcessCompleted_DialogMessage, STATGROUP_TaskGraphTasks);
void FCookContentActionCallbacks::HandleUatProcessCompleted(int32 ReturnCode, bool LaunchPakTask, TWeakPtr<SNotificationItem> NotificationItemPtr, FString TargetPlatform, FText PlatformDisplayName, FText TaskName, EventData Event)
{
	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("Platform"), PlatformDisplayName);
	Arguments.Add(TEXT("TaskName"), TaskName);

	if (ReturnCode == 0)
	{
		bool Failed = false;
		if (LaunchPakTask)
		{			
			// hack: create a file at the top level to ensure unrealpak doesn't try to change the mount point
			FString ContentFolder = FPaths::ConvertRelativePathToFull(FPaths::GameSavedDir() / "Cooked" / TargetPlatform / FApp::GetGameName() / "Content/");
			FString PlaceHolder = ContentFolder + TEXT("unrealpakPlaceholder");
			if (!FFileHelper::SaveStringToFile(TEXT("force unrealpak to include the folder that contains this"),
				*PlaceHolder))
			{
				Failed = true;
				UE_LOG(CookContentActions, Log, TEXT("failed to create placeholder :( %s"), *PlaceHolder);
			}
			else
			{
				RunOnMainThread* DoPak = new RunOnMainThread([=]() -> void {
					CreatePakTask(TargetPlatform, PlatformDisplayName, LOCTEXT("PakingContentTaskName", "Packaging content"), LOCTEXT("PakingTaskName", "Packing"), FEditorStyle::GetBrush(TEXT("MainFrame.CookContent")));
				});
				
			}
		}
		else
		{
			FString UploadURL = FDeployToPakEditorModule::Get().GetPakFileUploadURL();
			FString Author = FDeployToPakEditorModule::Get().GetAuthor();
			const UGeneralProjectSettings& ProjectSettings = *GetDefault<UGeneralProjectSettings>();
			const FString& CompanyName = ProjectSettings.CompanyName;
			if (UploadURL.Len() > 0)
			{
				RunOnMainThread* DoUpload = new RunOnMainThread([=]() -> void 
				{
					FString EngineVersionTag;
					if (TargetPlatform != "Editor")
					{
						EngineVersionTag = FString("_")+FString(ENGINE_COOKED_VERSION_STRING);
					}
					FString Basefilename = FString(FApp::GetGameName()) + TEXT("-") + TargetPlatform + EngineVersionTag + TEXT("-Content.pak");
					FString Filename = FPaths::ConvertRelativePathToFull(FPaths::GameSavedDir() / "Cooked" / Basefilename);
					TArray<uint8> Bytes;
					FFileHelper::LoadFileToArray(Bytes, *Filename);
					TSharedRef<IHttpRequest> Request = (&FHttpModule::Get())->CreateRequest();
					Request->OnProcessRequestComplete().BindRaw(new FFileUploader(CreateNotificationItem(PlatformDisplayName, LOCTEXT("PakingContentTaskName", "Upload Pak file"))), &FFileUploader::HandleHttpUploadFileComplete);
					const TArray<FDeployToPakAsset>& Assets = FDeployToPakEditorModule::Get().GetAssets();
					FString AssetsToUpload = "{\"Assets\":[";
					FString Sep = "";
					for (int32 i = 0; i < Assets.Num(); i++)
					{
						AssetsToUpload += Sep;
                                                Sep = ",";
						FString Sep2;
                                                AssetsToUpload += "{\"Maps\":[";
						Sep2 = "";
						for (int32 j = 0; j < Assets[i].Maps.Num(); j++)
						{
							AssetsToUpload += Sep2;
                                                        AssetsToUpload += "\"";
							AssetsToUpload += FPackageName::ObjectPathToPackageName(Assets[i].Maps[j].ToString());
                                                        AssetsToUpload += "\"";

							Sep2 = ",";
						}
						AssetsToUpload += "],";
                                                AssetsToUpload += "\"Blueprints\":[";
						Sep2 = "";
						for (int32 j = 0; j < Assets[i].Blueprints.Num(); j++)
						{
							AssetsToUpload += Sep2;
                                                        AssetsToUpload += "\"";
							AssetsToUpload += Assets[i].Blueprints[j].ToString();
                                                        AssetsToUpload += "\"";
							Sep2 = ",";
						}
                                                AssetsToUpload += "]}";
					}
                                        AssetsToUpload += "]}";
					Request->SetURL(TEXT("http://") + UploadURL / Basefilename + "?assets="+ FGenericPlatformHttp::UrlEncode(AssetsToUpload)+"&author="+ FGenericPlatformHttp::UrlEncode(CompanyName));
					Request->SetVerb("POST");
					Request->SetHeader("Content-Type", "application/octet-stream");
					Request->SetContent(Bytes);
					Request->ProcessRequest();
				});
			}
			else
			{
				FPlatformProcess::ExploreFolder(*FPaths::ConvertRelativePathToFull(FPaths::GameSavedDir() / "Cooked"));
			}
		}
		if (!Failed)
		{
			TGraphTask<FCookContentActionsNotificationTask>::CreateTask().ConstructAndDispatchWhenReady(
				NotificationItemPtr,
				SNotificationItem::CS_Success,
				FText::Format(LOCTEXT("UatProcessSucceededNotification", "{TaskName} complete!"), Arguments)
			);

			TArray<FAnalyticsEventAttribute> ParamArray;
			ParamArray.Add(FAnalyticsEventAttribute(TEXT("Time"), FPlatformTime::Seconds() - Event.StartTime));
			FEditorAnalytics::ReportEvent(Event.EventName + TEXT(".Completed"), PlatformDisplayName.ToString(), Event.bProjectHasCode, ParamArray);
			return;
		}
		//		FMessageLog("PackagingResults").Info(FText::Format(LOCTEXT("UatProcessSuccessMessageLog", "{TaskName} for {Platform} completed successfully"), Arguments));
	}
	TGraphTask<FCookContentActionsNotificationTask>::CreateTask().ConstructAndDispatchWhenReady(
		NotificationItemPtr,
		SNotificationItem::CS_Fail,
		FText::Format(LOCTEXT("PackagerFailedNotification", "{TaskName} failed!"), Arguments)
	);

	TArray<FAnalyticsEventAttribute> ParamArray;
	ParamArray.Add(FAnalyticsEventAttribute(TEXT("Time"), FPlatformTime::Seconds() - Event.StartTime));
	FEditorAnalytics::ReportEvent(Event.EventName + TEXT(".Failed"), PlatformDisplayName.ToString(), Event.bProjectHasCode, ReturnCode, ParamArray);

	if (TaskName.EqualTo(LOCTEXT("PackagingTaskName", "Packaging")))
	{
		FPackagingErrorHandler::SendPackagingErrorToMessageLog(ReturnCode);
	}

	// Present a message dialog if we want the error message to be prominent.
	if (FEditorAnalytics::ShouldElevateMessageThroughDialog(ReturnCode))
	{
		FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
			FSimpleDelegateGraphTask::FDelegate::CreateLambda([=]() {
			FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FEditorAnalytics::TranslateErrorCode(ReturnCode)));
		}),
			GET_STATID(STAT_FCookContentActionCallbacks_HandleUatProcessCompleted_DialogMessage),
			nullptr,
			ENamedThreads::GameThread
			);

		//		FMessageLog("PackagingResults").Info(FText::Format(LOCTEXT("UatProcessFailedMessageLog", "{TaskName} for {Platform} failed"), Arguments));
	}
}


void FCookContentActionCallbacks::HandleUatProcessOutput(FString Output, TWeakPtr<SNotificationItem> NotificationItemPtr, FText PlatformDisplayName, FText TaskName)
{
	if (!Output.IsEmpty() && !Output.Equals("\r"))
	{
		UE_LOG(CookContentActions, Log, TEXT("%s (%s): %s"), *TaskName.ToString(), *PlatformDisplayName.ToString(), *Output);

		if (TaskName.EqualTo(LOCTEXT("PackagingTaskName", "Packaging")))
		{
			// Deal with any cook errors that may have been encountered.
			FPackagingErrorHandler::ProcessAndHandleCookErrorOutput(Output);
		}
	}
}


#undef LOCTEXT_NAMESPACE
