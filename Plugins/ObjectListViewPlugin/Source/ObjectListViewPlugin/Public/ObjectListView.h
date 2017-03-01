#pragma once
#include "UMG.h"
#include "UMGStyle.h"
#include "Widgets/Views/SListView.h"
#include "Binding/PropertyBinding.h"
#include "Delegates/Delegate.h"
#include "ObjectListView.generated.h"



UCLASS(BlueprintType)
class OBJECTLISTVIEWPLUGIN_API UObjectListView : public UWidget
{

	GENERATED_UCLASS_BODY()
protected:
	TSharedPtr<SListView<UObject*>> ListView;
public:
	DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(UWidget*, FGenerateWidget, UObject*, Item);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemSelected, UObject*, Item, ESelectInfo::Type, Type);
	/** Source of row items for this list */
	UPROPERTY(EditAnywhere, Category="Content", BlueprintReadWrite)
		TArray<UObject*> ItemsSource;
	/** Called when the widget is needed for a row item. */
	UPROPERTY(EditAnywhere, Category = Events, meta = (IsBindableEvent = "True"))
		FGenerateWidget OnGenerateRowWidget;
	/** Row height */
	UPROPERTY(EditAnywhere, Category = "Content", BlueprintReadWrite)
		int32 ItemHeight;

	UFUNCTION(BlueprintCallable, Category = "ObjectListView")
		UObject* GetSelectedItem();
	UFUNCTION(BlueprintCallable, Category = "ObjectListView")
		bool IsItemSelected(UObject *Item);
	UFUNCTION(BlueprintCallable, Category = "ObjectListView")
		void ClearSelection();

	UPROPERTY(BlueprintAssignable, Category = "ObjectListView")
		FOnItemSelected OnItemSelected;

	// UWidget interface
#if WITH_EDITOR	
	virtual const FSlateBrush* GetEditorIcon() override;
	virtual const FText GetPaletteCategory() override;

#endif
	virtual void SynchronizeProperties() override;
	virtual void OnBindingChanged(const FName& Property) override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:

	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface
	void UpdateItemsSource();
	TSharedRef<ITableRow> GenerateRow(UObject* Item, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<SWidget> HandleGenerateWidget(UObject* Item) const;
	virtual void HandleItemSelected(UObject* Field, ESelectInfo::Type Type);
	
};
