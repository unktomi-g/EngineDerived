// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Engine.h"
#include "IHttpRequest.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include "AsyncTaskDownloadPak.generated.h"

/**
* Note: adapted from UAsyncTaskDownloadImage
*/

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDownloadPakDelegate, const FString&, LocalFilename);

UCLASS()
class PAKLOADER_API UAsyncTaskDownloadPak : public UBlueprintAsyncActionBase
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
		static UAsyncTaskDownloadPak* DownloadPak(const FString& URL, bool CheckForUpdateOnly);

public:

	UPROPERTY(BlueprintAssignable)
		FDownloadPakDelegate OnSuccess;

	UPROPERTY(BlueprintAssignable)
		FDownloadPakDelegate OnFail;

	UPROPERTY(BlueprintAssignable)
		FDownloadPakDelegate OnUpdated;

public:

	void Start(FString URL);

	static void GetDownloadFilename(const FString& Url, FString& PakFilename)
	{
		FString Ignored;
		GetDownloadFilenames(Url, PakFilename, Ignored);
	}
private:
	bool bCheckForUpdateOnly;
	static void GetDownloadFilenames(const FString& Url, FString& PakFilename, FString& ETagFilename);
	/** Handles Pak requests coming from the web */
	void HandlePakRequest(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);
};
