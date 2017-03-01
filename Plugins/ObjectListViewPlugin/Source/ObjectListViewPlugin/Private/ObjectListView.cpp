#include "ObjectListViewPluginPrivatePCH.h"
#include "ObjectListView.h"
#define LOCTEXT_NAMESPACE "OBJECTTREEVIEW"


#if WITH_EDITOR



const FSlateBrush* UObjectListView::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.Image");
}

const FText UObjectListView::GetPaletteCategory()
{
	return LOCTEXT("Common", "Common");
}

#endif

UObjectListView::UObjectListView(const FObjectInitializer& Init) : Super(Init), ItemHeight(24)
{
}

void UObjectListView::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	ListView.Reset();
}

void UObjectListView::OnBindingChanged(const FName& Property)
{
	Super::OnBindingChanged(Property);
}

void UObjectListView::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	UpdateItemsSource();
}

TSharedRef<SWidget> UObjectListView::HandleGenerateWidget(UObject* Item) const
{
	// Call the user's delegate to see if they want to generate a custom widget bound to the data source.
	if (OnGenerateRowWidget.IsBound())
	{
		UWidget* Widget = OnGenerateRowWidget.Execute(Item);
		if (Widget != NULL)
		{
			return Widget->TakeWidget();
		}
	}
	// If a row wasn't generated just create the default one, a simple text block of the item's name.
	return SNew(STextBlock)
		.Text(FText::FromString(Item->GetName()))
		.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 24));
}

TSharedRef<ITableRow> UObjectListView::GenerateRow(UObject* Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	auto Widget = HandleGenerateWidget(Item);
	return SNew(STableRow< UObject* >, OwnerTable)
		[
			Widget
		];
}

void UObjectListView::UpdateItemsSource()
{
}

bool UObjectListView::IsItemSelected(UObject *Item)
{
	return ListView.IsValid() ? ListView->IsItemSelected(Item) : false;
}

UObject* UObjectListView::GetSelectedItem()
{
	UObject* Result = nullptr;
	if (ListView.IsValid())
	{
		TArray<UObject*> SelectedItems;
		int32 Count = ListView->GetSelectedItems(SelectedItems);
		if (Count == 1)
		{
			Result = SelectedItems[0];
		}
	}
	return Result;
}

void UObjectListView::ClearSelection()
{
	if (ListView.IsValid()) ListView->ClearSelection();
}

void UObjectListView::HandleItemSelected(UObject *Item, ESelectInfo::Type Type)
{
	OnItemSelected.Broadcast(Item, Type);
}

class SObjectListView : public SListView<UObject*>
{
	TArray<UObject*> CurrentListItemsSource;
public:
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
	{
		if (CurrentListItemsSource != *ItemsSource)
		{
			CurrentListItemsSource = *ItemsSource;
			RequestListRefresh();
		}
		SListView<UObject*>::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	}
};

TSharedRef<SWidget> UObjectListView::RebuildWidget()
{
	ListView.Reset();
	UpdateItemsSource();
	ListView =
		SNew(SObjectListView)
		.ItemHeight(this->ItemHeight)
		.ListItemsSource(&ItemsSource)
		.OnGenerateRow(SListView<UObject*>::FOnGenerateRow::CreateUObject(this, &UObjectListView::GenerateRow))
		.OnSelectionChanged(SListView<UObject*>::FOnSelectionChanged::CreateUObject(this, &UObjectListView::HandleItemSelected))
		//.OnMouseButtonDoubleClick(this, &ObjectListView::OnItemDoubleClicked)
		//.OnContextMenuOpening(&ObjectListView::OnContextMenuOpening)
		.SelectionMode(ESelectionMode::Single)
		//.OnItemScrolledIntoView(this, &ObjectListView::OnItemScrolledIntoView)
		//.OnSetExpansionRecursive(this, &ObjectListView::OnSetExpansionRecursive)
		;
	return ListView.ToSharedRef();
}



#undef LOCTEXT_NAMESPACE
