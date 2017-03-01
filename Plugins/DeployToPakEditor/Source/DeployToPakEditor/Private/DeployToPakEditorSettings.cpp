#include "DeployToPakEditorPrivatePCH.h"
#include "DeployToPakEditor.h"
#include "DeployToPakEditorSettings.h"

UDeployToPakEditorSettings::UDeployToPakEditorSettings(class FObjectInitializer const &Init) :
	UDeveloperSettings(Init), PakFileUploadFolderURL("")
{
}

void UDeployToPakEditorSettings::PostInitProperties()
{
	Super::PostInitProperties();
	FDeployToPakEditorModule::Get().SetEditorSettings(this);
}

void UDeployToPakEditorSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName Name = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (Name == GET_MEMBER_NAME_CHECKED(UDeployToPakEditorSettings, PakFileUploadFolderURL))
	{
		
	}
	else if (Name == GET_MEMBER_NAME_CHECKED(UDeployToPakEditorSettings, Author))
	{
		
	}
	else if (Name == GET_MEMBER_NAME_CHECKED(UDeployToPakEditorSettings, Assets))
	{
		
	}
}
