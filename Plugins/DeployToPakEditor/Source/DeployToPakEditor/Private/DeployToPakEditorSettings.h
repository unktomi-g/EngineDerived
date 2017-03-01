#pragma once
#include "Engine/DeveloperSettings.h"
#include "DeployToPakEditor.h"
#include "DeployToPakEditorSettings.generated.h"



USTRUCT()
struct FDeployToPakAsset
{
	GENERATED_USTRUCT_BODY()
public:
	/** Include these Maps */
	UPROPERTY(EditAnywhere, Category = "Deploy Content to Pak", meta = (AllowedClasses = "World"))
		TArray<FStringAssetReference> Maps;
	/** Include these Actor Blueprints */
	UPROPERTY(EditAnywhere, Category = "Deploy Content to Pak", meta = (AllowedChildrenOfClasses = "Actor"))
		TArray<FStringAssetReference> Blueprints;
	
};


/**
 *
 */
UCLASS(config = Editor, defaultconfig)
class UDeployToPakEditorSettings : public UDeveloperSettings
{
	GENERATED_UCLASS_BODY()

public:

	/**
	 * Set the pak upload folder url (without the leading http://)
	 */
	UPROPERTY(config, EditAnywhere, Category = "Deploy Content to Pak")
		FString PakFileUploadFolderURL;
	
	UPROPERTY(config, EditAnywhere, Category = "Deploy Content to Pak")
		TArray<FDeployToPakAsset> Assets;

	UPROPERTY()
		FString Author;

public:
	virtual void PostInitProperties() override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
};
