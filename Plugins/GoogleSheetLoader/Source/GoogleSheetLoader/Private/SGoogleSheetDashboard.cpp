#include "SGoogleSheetDashboard.h"
#include "GoogleSheetConfig.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "EditorStyleSet.h"

void SGoogleSheetDashboard::Construct(const FArguments& InArgs)
{
	RefreshList();

	ChildSlot
	[
		SNew(SVerticalBox)
		
		// 상단 컨트롤 바
		+ SVerticalBox::Slot().AutoHeight().Padding(5)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton)
				.OnClicked(this, &SGoogleSheetDashboard::OnFetchAllClicked)
				.ContentPadding(FMargin(10, 5))
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("🔄 Update All Sheets")))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(5, 0)
			[
				SNew(SButton)
				.OnClicked_Lambda([this]() { RefreshList(); return FReply::Handled(); })
				.ContentPadding(FMargin(10, 5))
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("🔍 Refresh List")))
				]
			]
		]

		// 중앙 리스트 뷰
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(5)
		[
			SAssignNew(ListView, SListView<UGoogleSheetConfig*>)
			.ItemHeight(32.f)
			.ListItemsSource(&ConfigList)
			.OnGenerateRow(this, &SGoogleSheetDashboard::OnGenerateRow)
		]
	];
}

void SGoogleSheetDashboard::RefreshList()
{
	ConfigList.Empty();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetDataList;
	
	// 프로젝트 내 모든 GoogleSheetConfig 클래스(및 자식) 에셋 검색
	AssetRegistryModule.Get().GetAssetsByClass(UGoogleSheetConfig::StaticClass()->GetClassPathName(), AssetDataList);

	for (const FAssetData& AssetData : AssetDataList)
	{
		if (UGoogleSheetConfig* Config = Cast<UGoogleSheetConfig>(AssetData.GetAsset()))
		{
			ConfigList.Add(Config);
		}
	}

	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
}

TSharedRef<ITableRow> SGoogleSheetDashboard::OnGenerateRow(UGoogleSheetConfig* Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<UGoogleSheetConfig*>, OwnerTable)
	[
		SNew(SHorizontalBox)
		
		// 에셋 이름 및 아이콘
		+ SHorizontalBox::Slot().FillWidth(0.3f).VAlign(VAlign_Center).Padding(5, 0)
		[
			SNew(STextBlock).Text(FText::FromString(Item->GetName())).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
		]

		// 상태 표시
		+ SHorizontalBox::Slot().FillWidth(0.1f).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text_Lambda([Item]() {
				switch (Item->FetchStatus) {
					case EFetchStatus::Success: return FText::FromString(TEXT("✅"));
					case EFetchStatus::Failed:  return FText::FromString(TEXT("❌"));
					case EFetchStatus::Loading: return FText::FromString(TEXT("⏳"));
					default:                    return FText::FromString(TEXT("⬜"));
				}
			})
		]

		// 마지막 업데이트 메시지
		+ SHorizontalBox::Slot().FillWidth(0.4f).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text_Lambda([Item]() { return FText::FromString(Item->LastMessage); })
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		]

		// 개별 조작 버튼
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(5, 0)
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("Update")))
			.OnClicked_Lambda([Item]() {
				if (Item) Item->Fetch();
				return FReply::Handled();
			})
		]
		
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("Find")))
			.OnClicked_Lambda([Item]() {
				// 타입을 명시하여 모호성 해결
				if (Item) GEditor->SyncBrowserToObjects(TArray<UObject*>{ Item });
				return FReply::Handled();
			})
		]
	];
}

FReply SGoogleSheetDashboard::OnFetchAllClicked()
{
	for (UGoogleSheetConfig* Config : ConfigList)
	{
		if (Config) Config->Fetch();
	}
	return FReply::Handled();
}