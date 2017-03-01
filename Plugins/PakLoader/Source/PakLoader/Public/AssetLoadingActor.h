#pragma once
#include "Engine.h"
#include "PakLoader.h"
#include "AssetLoadingActor.generated.h"

/**
* Helper class to load assets from Pak files
*/

UCLASS(BlueprintType)
class AAssetLoadingActor : public AActor
{
	GENERATED_BODY()

	UObject* LoadRef(const FStringAssetReference& Ref);
	TMap<FString, FTransform> DeferredLevelTransforms;
public:

	/**
	* Returns the Platform name, but in PIE returns "Editor" (used to encode pak file names)
	*/

	UFUNCTION(BlueprintCallable, Category = "Pak", BlueprintPure, meta = (WorldContext = "WorldContextObject"))
		static FString GetCookedPlatformName(UObject *WorldContextObject);
	
	UFUNCTION(BlueprintCallable, Category = "Pak", BlueprintPure, meta = (WorldContext = "WorldContextObject"))
		static void GetCookedPlatformNames(UObject *WorldContextObject, FString& EngineVersion, TArray<FString>& CookedPlatforms);
	/**
	* Unmounts all pak files
	*/
	UFUNCTION(BlueprintCallable, Category = "Pak")
		static void ReinitPakLoader();

	/**
	* Get Level Actors
	*/
	UFUNCTION(BlueprintCallable, Category = "Level", BlueprintPure, meta = (WorldContext = "WorldContextObject"))
		static void GetLevelActors(const FString& LevelName, UObject* WorldContextObject, TArray<AActor*>& Result);

	/**
	* Unmounts the specified Pak file
	*/
	UFUNCTION(BlueprintCallable, Category = "Pak")
		static bool UnmountPak(const FString& PakFileName);

	/**
	* Loads assets from PakFile asynchronously. When done triggers OnAssetsLoaded event.
	*/
	UFUNCTION(BlueprintCallable, Category = "Pak")
		bool LoadPak();
	/**
	* Returns a list of classes and a list of objects found in PakFile
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = "Pak")
		void OnAssetsLoaded(const TArray<UClass*>& Classes, const TArray<UObject*>& Objects);

	// Level handling

	/**
	* Converts any maps found in PakFile into StreamingLevels (to be loaded later)
	*/
	UFUNCTION(BlueprintCallable, Category = "Pak", meta = (WorldContext="WorldContextObject"))
		static bool GetLevelsFromPak(UObject* WorldContextObject, const FString& PakFile, TArray<ULevelStreamingKismet*>& Levels);

	/**
	* Sets the transform of the streaming level associated with MapName (must be called before the level is loaded).
	*/
	UFUNCTION(BlueprintCallable, Category = "Pak")
		bool SetLevelTransform(const FString& MapName, const FTransform& Transform);

	/**
	* Get the long-form map name from a Streaming Level
	*/
	UFUNCTION(BlueprintCallable, Category = "Pak", BlueprintPure)
		static FString GetLevelToLoadName(const ULevelStreaming* Level);
	/**
	* Get the unique level instance name
	*/
	UFUNCTION(BlueprintCallable, Category = "Pak", BlueprintPure)
		static FString GetLevelInstanceName(const ULevelStreaming* Level);

	//UFUNCTION(BlueprintCallable, Category = "Pak", BlueprintPure)
	//	static void CreateLevelInstance(ULevelStreaming* Level, const FString& InstanceId, const FTransform& Transform, ULevelStreaming*& LevelInstance);

	UFUNCTION(BlueprintCallable, Category = "Level", BlueprintPure)
		static void GetLevelInstanceActors(ULevelStreaming* LevelInstance, TArray<AActor*>& Result);

	UFUNCTION(BlueprintCallable, Category = "Level")
		static void RemoveStreamingLevel(ULevelStreaming* Level);

	UFUNCTION(BlueprintCallable, Category = "Level")
		static void ActivateLevelInstance(ULevelStreaming* LevelInstance, bool bLoading);
	UFUNCTION(BlueprintCallable, Category = LevelStreaming, meta = (WorldContext = "WorldContextObject"))
		static ULevelStreamingKismet* CreateLevelInstance(UObject* WorldContextObject, const FString& LevelName, const FString& LevelUID, const FVector& Location, const FRotator& Rotation, bool& bOutSuccess);
	/**
	* The PakFile from which to load assets
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pak")
		FString PakFile;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

};
