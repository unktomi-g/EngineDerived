#include "PakLoaderPrivatePCH.h"
#include "AssetLoadingActor.h"
#include "Engine/LevelStreamingKismet.h"
#include "AsyncTaskDownloadPak.h"
#include "Runtime/Launch/Resources/Version.h"

#define ENGINE_COOKED_VERSION_STRING \
VERSION_STRINGIFY(ENGINE_MAJOR_VERSION) \
VERSION_TEXT(".") \
VERSION_STRINGIFY(ENGINE_MINOR_VERSION) \
VERSION_TEXT(".") \
VERSION_STRINGIFY(ENGINE_PATCH_VERSION)


FString AAssetLoadingActor::GetCookedPlatformName(UObject* WorldContextObject)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject))
	{
		if (World->WorldType == EWorldType::PIE || !World->StreamingLevelsPrefix.IsEmpty())
		{
			return FString("Editor");
		}
		// @TODO Unfortunately this doesn't give the cook_flavor, e.g. gives Android instead of Android_ASTC
		FString Result = FPlatformProperties::PlatformName();
		if (Result == "Windows" || Result == "Mac" || Result == "Linux")
		{
			Result = Result + "NoEditor";
		}
#if ENGINE_MINOR_VERSION > 13
		if (Result == "Android")
		{
			TArray<FString> Platforms;
			FPlatformMisc::GetValidTargetPlatforms(Platforms);
			//if (Platforms.Num() > 0) Result = Platforms[0]; // @TODO: find the best one
			for (int32 i = 0; i < Platforms.Num(); i++)
			{
				UE_LOG(PakLoader, Log, TEXT("Android target: %s"), *Platforms[i]);
			}
		}
#endif
		FString EngineVersion(ENGINE_COOKED_VERSION_STRING);
		UE_LOG(PakLoader, Log, TEXT("Engine version: %s"), *EngineVersion);
		return Result + "_" + EngineVersion;
	}
	return FString("Unknown");
}

void AAssetLoadingActor::GetCookedPlatformNames(UObject* WorldContextObject, FString& EngineVersion, TArray<FString>& CookedPlatforms)
{
#if ENGINE_MINOR_VERSION > 13
	FPlatformMisc::GetValidTargetPlatforms(CookedPlatforms);
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
	if (World != nullptr)
	{
		if (World->IsPlayInEditor())
		{

			EngineVersion = ""; // uncooked works on all versions
			CookedPlatforms.Reset();
			CookedPlatforms.Add(TEXT("Editor"));
			return;
		}
	}
	EngineVersion = ENGINE_COOKED_VERSION_STRING;
	UE_LOG(PakLoader, Log, TEXT("Cooked Engine Version: %s"), *EngineVersion);
	for (int32 i = 0; i < CookedPlatforms.Num(); i++)
	{
		UE_LOG(PakLoader, Log, TEXT("Cooked target: %s"), *CookedPlatforms[i]);
	}
	return;
#else
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject))
	{
		if (World->WorldType == EWorldType::PIE || !World->StreamingLevelsPrefix.IsEmpty())
		{
			EngineVersion = ""; // uncooked works on all versions
			CookedPlatforms.Add(TEXT("Editor"));
			return;
		}
		EngineVersion = ENGINE_COOKED_VERSION_STRING;
		// @TODO Unfortunately this doesn't give the cook_flavor, e.g. gives Android instead of Android_ASTC
		FString Result = FPlatformProperties::PlatformName();
		if (Result == "Windows" || Result == "Mac" || Result == "Linux")
		{
			Result = Result + "NoEditor";
		}

		CookedPlatforms.Add(Result);
	}
#endif
}

UObject* AAssetLoadingActor::LoadRef(const FStringAssetReference& Ref)
{
	UObject* Result = Ref.TryLoad();
	if (Result == nullptr)
	{
		// Compiled blueprint classes have a _C extension
		FStringAssetReference C(Ref.ToString() + "_C");
		Result = C.TryLoad();
	}
	return Result;
}

// Note: this must be match the code in LevelStreaming.cpp
static FName MakeSafeLevelName(const FName& InLevelName, UWorld* InWorld)
{
#if WITH_EDITOR
	// Special case for PIE, the PackageName gets mangled.
	if (!InWorld->StreamingLevelsPrefix.IsEmpty())
	{
		FString PackageName = InWorld->StreamingLevelsPrefix + FPackageName::GetShortName(InLevelName);
		if (!FPackageName::IsShortPackageName(InLevelName))
		{
			PackageName = FPackageName::GetLongPackagePath(InLevelName.ToString()) + TEXT("/") + PackageName;
		}
		return FName(*PackageName);
	}
#endif
	return InLevelName;
}

FString AAssetLoadingActor::GetLevelInstanceName(const ULevelStreaming* Level)
{
	if (Level != nullptr)
	{
		return Level->GetWorldAsset().ToString();
	}
	return FString();
}

FString AAssetLoadingActor::GetLevelToLoadName(const ULevelStreaming* Level)
{
	if (Level != nullptr)
	{
		const FString QualifiedName = Level->PackageNameToLoad.ToString();
		int32 Dot = QualifiedName.Find(".");
		return QualifiedName.Left(Dot);
	}
	return FString();
}

bool AAssetLoadingActor::SetLevelTransform(const FString& MapName, const FTransform& Transform)
{
	FName PackageNameToLoad = FName(*MapName);
	UWorld* InWorld = GetWorld();
	FName InstanceUniquePackageName = MakeSafeLevelName(PackageNameToLoad, InWorld);
	int32 Index = InWorld->StreamingLevels.IndexOfByPredicate(ULevelStreaming::FPackageNameMatcher(InstanceUniquePackageName));
	if (Index != INDEX_NONE)
	{
		InWorld->StreamingLevels[Index]->LevelTransform = Transform;
	}
	else
	{
		DeferredLevelTransforms.Add(MapName, Transform);
	}
	return Index != INDEX_NONE;
}

/*
void AAssetLoadingActor::CreateLevelInstance(ULevelStreaming* Level, const FString& InstanceId, const FTransform& Transform, ULevelStreaming*& LevelInstance)
{
	ULevelStreaming* Result = Level->CreateInstance(InstanceId);
	LevelInstance = Result;
	if (Result)
	{
		Result->LevelTransform = Transform;
		Result->bShouldBeLoaded = false;
		Result->bShouldBeVisible = false;
	}
}
*/


bool AAssetLoadingActor::GetLevelsFromPak(UObject* WorldContextObject, const FString& PakFile, TArray<ULevelStreamingKismet*> &Levels)
{
	bool Result = false;
	if (UWorld* InWorld = GEngine->GetWorldFromContextObject(WorldContextObject))
	{
		FPakLoaderModule& Loader =
			FModuleManager::LoadModuleChecked<FPakLoaderModule>(FName(TEXT("PakLoader")));
		TArray<FString> MapNames;
		Result = Loader.GetLevelsFromPak(PakFile, MapNames);
		if (Result)
		{
			for (int32 i = 0; i < MapNames.Num(); i++)
			{
				const FString& MapName = FPackageName::ObjectPathToPackageName(MapNames[i]);
				FString ShortName = FPackageName::GetShortName(*MapName);
				FName PackageNameToLoad = FName(*(MapName + TEXT(".") + ShortName));
				FString PackageNameToLoadStr = PackageNameToLoad.ToString();
				FName InstanceUniquePackageName = PackageNameToLoad;
				if (GEngine->NetworkRemapPath(InWorld->GetNetDriver(), PackageNameToLoadStr, true))
				{
					InstanceUniquePackageName = FName(*PackageNameToLoadStr);
				}
				// check if instance name is unique among existing streaming level objects
				int32 Index =
					InWorld->StreamingLevels.IndexOfByPredicate(ULevelStreaming::FPackageNameMatcher(InstanceUniquePackageName));
				ULevelStreamingKismet* StreamingLevelInstance;
				if (Index == INDEX_NONE)
				{
					StreamingLevelInstance = NewObject<ULevelStreamingKismet>(InWorld, ULevelStreamingKismet::StaticClass(),
						NAME_None,
						RF_Transient,
						NULL);
					// new level streaming instance will load the same map package as this object
					StreamingLevelInstance->PackageNameToLoad = PackageNameToLoad;
					TAssetPtr<UWorld> WorldAsset;
					WorldAsset = InstanceUniquePackageName.ToString();
					StreamingLevelInstance->SetWorldAsset(WorldAsset);
					StreamingLevelInstance->bShouldBeLoaded = false;
					StreamingLevelInstance->bShouldBeVisible = false;
					// add a new instance to streaming level list
					InWorld->StreamingLevels.Add(StreamingLevelInstance);
					UE_LOG(PakLoader, Log, TEXT("Added Streaming Level :) %s"), *WorldAsset.ToString());
					Levels.Add(StreamingLevelInstance);
				}
				else
				{
					StreamingLevelInstance = Cast<ULevelStreamingKismet>(InWorld->StreamingLevels[Index]);
					if (StreamingLevelInstance != nullptr) 
					{
						Levels.Add(StreamingLevelInstance);
					}
				}
			}
		}
	}
	return Result;
}

bool AAssetLoadingActor::LoadPak()
{
	FPakLoaderModule& Loader =
		FModuleManager::LoadModuleChecked<FPakLoaderModule>(FName(TEXT("PakLoader")));
	bool Result = Loader.GetAssetsFromPak(PakFile,
		[this, &Loader](TSharedPtr<TArray<FStringAssetReference>> AssetsPtr)
	{
		TArray<UClass*> Classes;
		TArray<UObject*> Objects;
		const TArray<FStringAssetReference>& Assets = *AssetsPtr;
		for (int32 i = 0; i < Assets.Num(); i++)
		{
			UObject* Obj = LoadRef(Assets[i]);
			UClass* Class;
#if WITH_EDITOR
			UBlueprint* BP = Cast<UBlueprint>(Obj);
			if (BP != nullptr)
			{
				Class = BP->GeneratedClass;
			}
			else
#endif
			{
				Class = Cast<UClass>(Obj);
			}
			if (Class != nullptr)
			{
				Classes.Add(Class);
			}
			else if (Obj != nullptr)
			{
				Objects.Add(Obj);
			}
			else
			{
				UE_LOG(PakLoader, Log, TEXT("Couldn't load asset :( %s from Pak %s"), *Assets[i].ToString(), *PakFile);
			}
		}
		OnAssetsLoaded(Classes, Objects);
	});
	return Result;
}

void AAssetLoadingActor::ReinitPakLoader()
{
	FPakLoaderModule& Loader =
		FModuleManager::LoadModuleChecked<FPakLoaderModule>(FName(TEXT("PakLoader")));
	Loader.EndPlay();
}

bool AAssetLoadingActor::UnmountPak(const FString& PakFileName)
{
	FPakLoaderModule& Loader =
		FModuleManager::LoadModuleChecked<FPakLoaderModule>(FName(TEXT("PakLoader")));
	return Loader.UnmountPakFile(PakFileName);
}

void AAssetLoadingActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
}

void AAssetLoadingActor::GetLevelInstanceActors(ULevelStreaming* StreamingLevel, TArray<AActor*>& Result)
{
	if (StreamingLevel != nullptr)
	{
		ULevel* LoadedLevel = StreamingLevel->GetLoadedLevel();
		if (LoadedLevel != nullptr)
		{
			Result.Append(LoadedLevel->Actors);
		}
	}
}

void AAssetLoadingActor::RemoveStreamingLevel(ULevelStreaming* Level)
{
	if (Level != nullptr)
	{
		Level->bIsRequestingUnloadAndRemoval = true;
		if (Level->bShouldBeLoaded)
		{
			ActivateLevelInstance(Level, false);
		}
	}
}


void AAssetLoadingActor::ActivateLevelInstance(ULevelStreaming* LevelStreamingObject, bool bLoading)
{
	
	if (LevelStreamingObject != NULL)
	{
		// Loading.
		if (bLoading)
		{
			UE_LOG(PakLoader, Log, TEXT("Streaming in level %s (%s)..."), *LevelStreamingObject->GetName(), *LevelStreamingObject->GetWorldAssetPackageName());
			LevelStreamingObject->bShouldBeLoaded = bLoading;
			LevelStreamingObject->bShouldBeVisible = true;
			LevelStreamingObject->bShouldBlockOnLoad = false;
		}
		// Unloading.
		else
		{
			UE_LOG(PakLoader, Log, TEXT("Streaming out level %s (%s)..."), *LevelStreamingObject->GetName(), *LevelStreamingObject->GetWorldAssetPackageName());
			LevelStreamingObject->bShouldBeLoaded = false;
			LevelStreamingObject->bShouldBeVisible = false;
		}

		UWorld* LevelWorld = CastChecked<UWorld>(LevelStreamingObject->GetOuter());
		// If we have a valid world
		if (LevelWorld)
		{
			// Notify players of the change
			for (FConstPlayerControllerIterator Iterator = LevelWorld->GetPlayerControllerIterator(); Iterator; ++Iterator)
			{
				APlayerController* PlayerController = *Iterator;

				UE_LOG(PakLoader, Log, TEXT("ActivateLevel %s %i %i %i"),
					*LevelStreamingObject->GetWorldAssetPackageName(),
					LevelStreamingObject->bShouldBeLoaded,
					LevelStreamingObject->bShouldBeVisible,
					LevelStreamingObject->bShouldBlockOnLoad);



				PlayerController->LevelStreamingStatusChanged(
					LevelStreamingObject,
					LevelStreamingObject->bShouldBeLoaded,
					LevelStreamingObject->bShouldBeVisible,
					LevelStreamingObject->bShouldBlockOnLoad,
					INDEX_NONE);

			}
		}

	}
}

void AAssetLoadingActor::GetLevelActors(const FString& LevelName, UObject* WorldContextObject, TArray<AActor*>& Result)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject))
	{
		const FString ShortName = FPackageName::GetShortName(*LevelName);
		const FName LongName = FName(*(LevelName + TEXT(".") + ShortName));
		for (int32 i = 0; i < World->StreamingLevels.Num(); i++)
		{
			ULevelStreaming* StreamingLevel = World->StreamingLevels[i];
			if (StreamingLevel->PackageNameToLoad == LongName)
			{
				ULevel* LoadedLevel = StreamingLevel->GetLoadedLevel();
				if (LoadedLevel != nullptr)
				{
					Result.Append(LoadedLevel->Actors);
				}
				return;
			}
		}
	}
}

ULevelStreamingKismet* AAssetLoadingActor::CreateLevelInstance(UObject* WorldContextObject, const FString& InLevelName, const FString& LevelUID, const FVector& Location, const FRotator& Rotation, bool& bOutSuccess)
{
	bOutSuccess = false;
	UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject, false);
	if (!World)
	{
		return nullptr;
	}
	const FString LevelName = FPackageName::ObjectPathToPackageName(InLevelName);
	// Check whether requested map exists, this could be very slow if LevelName is a short package name
	FString LongPackageName;
	bOutSuccess = FPackageName::SearchForPackageOnDisk(LevelName, &LongPackageName);
	if (!bOutSuccess)
	{
		return nullptr;
	}
	
	// Create Unique Name for sub-level package
	const FString ShortPackageName = FPackageName::GetShortName(LongPackageName);
	const FString PackagePath = FPackageName::GetLongPackagePath(LongPackageName);
	FString UniqueLevelPackageName = PackagePath + TEXT("/") + World->StreamingLevelsPrefix + ShortPackageName;
	UniqueLevelPackageName += TEXT("_LevelInstance_") + LevelUID;
	FName UniqueLevelPackageFName(*UniqueLevelPackageName);
	int32 Index =
		World->StreamingLevels.IndexOfByPredicate(ULevelStreaming::FPackageNameMatcher(UniqueLevelPackageFName));
	if (Index != INDEX_NONE)
	{
		return (ULevelStreamingKismet*)World->StreamingLevels[Index];
	}
	// Setup streaming level object that will load specified map
	ULevelStreamingKismet* StreamingLevel = NewObject<ULevelStreamingKismet>(World, ULevelStreamingKismet::StaticClass(), NAME_None, RF_Transient, NULL);
	StreamingLevel->SetWorldAssetByPackageName(UniqueLevelPackageFName);
	StreamingLevel->LevelColor = FColor::MakeRandomColor();
	StreamingLevel->bShouldBeLoaded = false;
	StreamingLevel->bShouldBeVisible = false;
	StreamingLevel->bShouldBlockOnLoad = false;
	StreamingLevel->bInitiallyLoaded = false;
	StreamingLevel->bInitiallyVisible = false;
	// Transform
	StreamingLevel->LevelTransform = FTransform(Rotation, Location);
	// Map to Load
	StreamingLevel->PackageNameToLoad = FName(*LongPackageName);

	// Add the new level to world.
	World->StreamingLevels.Add(StreamingLevel);

	bOutSuccess = true;
	return StreamingLevel;
}
