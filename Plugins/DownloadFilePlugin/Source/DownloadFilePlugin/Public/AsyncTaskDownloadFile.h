// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Engine.h"
#include "IHttpRequest.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "AsyncTaskDownloadFile.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(FileLoader, Log, All)

/**
* Note: adapted from UAsyncTaskDownloadImage
*/

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDownloadFileDelegate, const FString&, LocalFilename);

UCLASS()
class DOWNLOADFILEPLUGIN_API UAsyncTaskDownloadFile : public UBlueprintAsyncActionBase
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
		static UAsyncTaskDownloadFile* DownloadFile(const FString& URL, bool CheckForUpdateOnly);

public:

	UPROPERTY(BlueprintAssignable)
		FDownloadFileDelegate OnSuccess;

	UPROPERTY(BlueprintAssignable)
		FDownloadFileDelegate OnFail;

	UPROPERTY(BlueprintAssignable)
		FDownloadFileDelegate OnUpdated;

public:

	void Start(FString URL);

	static void GetDownloadFilename(const FString& Url, FString& FileFilename)
	{
		FString Ignored;
		GetDownloadFilenames(Url, FileFilename, Ignored);
	}
private:
	bool bCheckForUpdateOnly;
	static void GetDownloadFilenames(const FString& Url, FString& FileFilename, FString& ETagFilename);
	/** Handles File requests coming from the web */
	void HandleFileRequest(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);
};
