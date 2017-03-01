// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "Engine.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "OffscreenWidgetComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTextureAvailable, UTexture*, Texture);

UCLASS(Blueprintable, ClassGroup = "UserInterface", hidecategories = (Object, Activation, "Components|Activation", Sockets, Base, Lighting, LOD, Mesh), editinlinenew, meta = (BlueprintSpawnableComponent))
class OFFSCREENWIDGETPLUGIN_API UOffscreenWidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UOffscreenWidgetComponent();

	// Called when the game starts
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintAssignable)
		FOnTextureAvailable OnTextureAvailable;

	UFUNCTION(BlueprintCallable, Category = UserInterface)
		UTexture* GetTexture()
	{
		return GetRenderTarget();
	}
	
	// Called every frame
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;
	virtual bool ShouldDrawWidget() const override 
	{
		return true;
	}	
private:
	bool bTextureIsAvailable;
	
};
