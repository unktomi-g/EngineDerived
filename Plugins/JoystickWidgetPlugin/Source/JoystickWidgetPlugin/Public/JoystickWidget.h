#pragma once
#include "Engine.h"
#include "SVirtualJoystickWidget.h"
#include "joystickwidget.generated.h"

USTRUCT()
struct FJoystickInputControl
{
	GENERATED_USTRUCT_BODY()
public:
		// basically mirroring SVirtualJoystick::FControlInfo but as an editable class
	UPROPERTY(EditAnywhere, Category = "Control", meta = (ToolTip = "For sticks, this is the Thumb"))
		UTexture2D* Image1;
	UPROPERTY(EditAnywhere, Category = "Control", meta = (ToolTip = "For sticks, this is the Background"))
		UTexture2D* Image2;
	UPROPERTY(EditAnywhere, Category = "Control", meta = (ToolTip = "The center point of the control (if <= 1.0, it's relative to screen, > 1.0 is absolute)"))
		FVector2D Center;
	UPROPERTY(EditAnywhere, Category = "Control", meta = (ToolTip = "The size of the control (if <= 1.0, it's relative to screen, > 1.0 is absolute)"))
		FVector2D VisualSize;
	UPROPERTY(EditAnywhere, Category = "Control", meta = (ToolTip = "For sticks, the size of the thumb (if <= 1.0, it's relative to screen, > 1.0 is absolute)"))
		FVector2D ThumbSize;
	UPROPERTY(EditAnywhere, Category = "Control", meta = (ToolTip = "The interactive size of the control (if <= 1.0, it's relative to screen, > 1.0 is absolute)"))
		FVector2D InteractionSize;
	UPROPERTY(EditAnywhere, Category = "Control", meta = (ToolTip = "The scale for control input"))
		FVector2D InputScale;
	UPROPERTY(EditAnywhere, Category = "Control", meta = (ToolTip = "The main input to send from this control (for sticks, this is the horizontal axis)"))
		FKey MainInputKey;
	UPROPERTY(EditAnywhere, Category = "Control", meta = (ToolTip = "The alternate input to send from this control (for sticks, this is the vertical axis)"))
		FKey AltInputKey;

	FJoystickInputControl()
		: InputScale(1.f, 1.f), Image1(nullptr), Image2(nullptr), Center(.855f, .75f), VisualSize(180, 180), ThumbSize(96, 96), InteractionSize(180, 180)
	{
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnControllerAnalog, float, x, float, y);

UCLASS()
class JOYSTICKWIDGETPLUGIN_API UJoystickWidget : public UWidget
{
	GENERATED_UCLASS_BODY()
protected:
	TSharedPtr<SVirtualJoystickWidget> Joystick;
public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTouchEvent, FGeometry, MyGeometry, const FPointerEvent&, PointerEvent);
#if WITH_EDITOR
	// UWidget interface
	virtual const FSlateBrush* GetEditorIcon() override;
	virtual const FText GetPaletteCategory() override;
	// End UWidget interface
#endif

	UPROPERTY(EditAnywhere, Category = "Joystick")
		TArray<FJoystickInputControl> Controls;

	UPROPERTY(BlueprintAssignable, Category = "Joystick")
		FOnControllerAnalog OnControllerAnalog;

	UPROPERTY(EditAnywhere, Category = "Joystick", meta = (ToolTip = "Opacity (0.0 - 1.0) of all controls while any control is active"))
		float ActiveOpacity;

	UPROPERTY(EditAnywhere, Category = "Joystick", meta = (ToolTip = "Opacity (0.0 - 1.0) of all controls while no controls are active"))
		float InactiveOpacity;

	UPROPERTY(EditAnywhere, Category = "Joystick", meta = (ToolTip = "How long after user interaction will all controls fade out to Inactive Opacity"))
		float TimeUntilDeactive;

	UPROPERTY(EditAnywhere, Category = "Joystick", meta = (ToolTip = "How long after going inactive will controls reset/recenter themselves (0.0 will disable this feature)"))
		float TimeUntilReset;

	UPROPERTY(EditAnywhere, Category = "Joystick", meta = (ToolTip = "How long after joystick enabled for touch (0.0 will disable this feature)"))
		float ActivationDelay;

	UPROPERTY(EditAnywhere, Category = "Joystick", meta = (ToolTip = "Whether to prevent joystick re-center"))
		bool bPreventRecenter;

	UPROPERTY(EditAnywhere, Category = "Joystick", meta = (ToolTip = "Delay at startup before virtual joystick is drawn"))
		float StartupDelay;
	UPROPERTY()
		UTexture2D* DefaultThumb;
	UPROPERTY()
		UTexture2D* DefaultBackground;
	
	

	/**
	* Called when a touchpad touch is started (finger down)
	*
	* @param MyGeometry    The geometry of the widget receiving the event.
	* @param InTouchEvent	The touch event generated
	*/
	UPROPERTY(BlueprintAssignable, Category = "Events")
		FOnTouchEvent OnTouchStarted;


	/**
	* Called when a touchpad touch is moved  (finger moved)
	*
	* @param MyGeometry    The geometry of the widget receiving the event.
	* @param InTouchEvent	The touch event generated
	*/
	UPROPERTY(BlueprintAssignable, Category = "Events")
		FOnTouchEvent OnTouchMoved;



	/**
	* Called when a touchpad touch is ended (finger lifted)
	*
	* @param MyGeometry    The geometry of the widget receiving the event.
	* @param InTouchEvent	The touch event generated
	*/
	UPROPERTY(BlueprintAssignable, Category = "Events")
		FOnTouchEvent OnTouchEnded;

	void HandleTouchStarted(FGeometry MyGeometry, const FPointerEvent& PointerEvent)
	{
		OnTouchStarted.Broadcast(MyGeometry, PointerEvent);
	}

	void HandleTouchMoved(FGeometry MyGeometry, const FPointerEvent& PointerEvent)
	{
		OnTouchMoved.Broadcast(MyGeometry, PointerEvent);
	}

	void HandleTouchEnded(FGeometry MyGeometry, const FPointerEvent& PointerEvent)
	{
		OnTouchEnded.Broadcast(MyGeometry, PointerEvent);
	}

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void Activate();
	virtual void HandleJoystick(float x, float y);
	virtual void ReleaseSlateResources(bool bReleaseChildResources) override;

	
};
