#include "JoystickWidgetPluginPrivatePCH.h"
#include "JoystickWidget.h"
#if WITH_EDITOR
#define LOCTEXT_NAMESPACE "UMG"


const FSlateBrush* UJoystickWidget::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.Image");
}

const FText UJoystickWidget::GetPaletteCategory()
{
	return LOCTEXT("Common", "Common");
}

#endif


UJoystickWidget::UJoystickWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// defaults
	ActiveOpacity = 1.0f;
	InactiveOpacity = 0.1f;
	TimeUntilDeactive = 0.5f;
	TimeUntilReset = 2.0f;
	ActivationDelay = 0.f;
	StartupDelay = 0.f;
	static ConstructorHelpers::FObjectFinder<UTexture2D> JoystickBg(TEXT("/JoystickWidgetPlugin/JoystickWidgetAssets/VirtualJoystick_Background"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> JoystickThumb(TEXT("/JoystickWidgetPlugin/JoystickWidgetAssets/VirtualJoystick_Thumb"));
	DefaultThumb = JoystickThumb.Object;
	DefaultBackground = JoystickBg.Object;
	bPreventRecenter = false;
}

void UJoystickWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Joystick.Reset();
}

void UJoystickWidget::HandleJoystick(float x, float y)
{
	OnControllerAnalog.Broadcast(x, y);
}

void UJoystickWidget::Activate()
{
	// convert from the UStructs to the slate structs
	TArray<SVirtualJoystickWidget::FControlInfo> SlateControls;
	if (Controls.Num() == 0) 
	{
		FJoystickInputControl Control;
		Control.Image1 = DefaultThumb;
		Control.Image2 = DefaultBackground;
		Controls.Add(Control);
	}
	for (int32 ControlIndex = 0; ControlIndex < Controls.Num(); ControlIndex++)
	{
		FJoystickInputControl Control = Controls[ControlIndex];
		SVirtualJoystickWidget::FControlInfo* SlateControl = new(SlateControls)SVirtualJoystickWidget::FControlInfo;
		SlateControl->Image1 = (Control.Image1 ? FCoreStyle::GetDynamicImageBrush("Engine.Joystick.Image1", Control.Image1, Control.Image1->GetFName()) : nullptr);
		SlateControl->Image2 = (Control.Image2 ? FCoreStyle::GetDynamicImageBrush("Engine.Joystick.Image2", Control.Image2, Control.Image2->GetFName()) : nullptr);
		SlateControl->Center = Control.Center;
		SlateControl->VisualSize = Control.VisualSize;
		SlateControl->ThumbSize = Control.ThumbSize;
		if (Control.InputScale.SizeSquared() > FMath::Square(DELTA))
		{
			SlateControl->InputScale = Control.InputScale;
		}
		SlateControl->InteractionSize = Control.InteractionSize;
		SlateControl->MainInputKey = Control.MainInputKey;
		SlateControl->AltInputKey = Control.AltInputKey;
	}
	Joystick->SetGlobalParameters(
		ActiveOpacity,InactiveOpacity,
		TimeUntilDeactive, TimeUntilReset, 
		ActivationDelay, bPreventRecenter, 
		StartupDelay);
	Joystick->OnJoystick.AddUObject(this, &UJoystickWidget::HandleJoystick);
	Joystick->OnTouchStartedEvent.AddUObject(this, &UJoystickWidget::HandleTouchStarted);
	Joystick->OnTouchMovedEvent.AddUObject(this, &UJoystickWidget::HandleTouchMoved);
	Joystick->OnTouchEndedEvent.AddUObject(this, &UJoystickWidget::HandleTouchEnded);
	// set them as active!
	Joystick->SetControls(SlateControls);

}

TSharedRef<SWidget> UJoystickWidget::RebuildWidget() 
{
	Joystick = SNew(SVirtualJoystickWidget);
	Activate();
	return Joystick.ToSharedRef();
}