// Fill out your copyright notice in the Description page of Project Settings.
#include "OffscreenWidgetPluginPrivatePCH.h"
#include "OffscreenWidgetPlugin.h"
#include "OffscreenWidgetComponent.h"


// Sets default values for this component's properties
UOffscreenWidgetComponent::UOffscreenWidgetComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	// ...
	bVisible = false;
	bTextureIsAvailable = false;
	SetCollisionProfileName(FName("NoCollision"));
}


// Called when the game starts
void UOffscreenWidgetComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UOffscreenWidgetComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );
	if (!bTextureIsAvailable)
	{
		UTexture* Texture = GetTexture();
		if (Texture != nullptr)
		{
			bTextureIsAvailable = true;
			OnTextureAvailable.Broadcast(Texture);
		}
	}
	// ...
}

