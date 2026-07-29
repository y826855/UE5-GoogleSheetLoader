#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class UGoogleSheetConfig;

/** 대시보드 리스트에 표시될 항목 데이터 */
using FGoogleSheetConfigWeakPtr = TWeakObjectPtr<UGoogleSheetConfig>;

class SGoogleSheetDashboard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGoogleSheetDashboard) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** 프로젝트 내 모든 Config 에셋을 검색하여 리스트 갱신 */
	void RefreshList();

private:
	TSharedRef<ITableRow> OnGenerateRow(FGoogleSheetConfigWeakPtr Item, const TSharedRef<STableViewBase>& OwnerTable);
	FReply OnFetchAllClicked();

	TArray<FGoogleSheetConfigWeakPtr> ConfigList;
	TSharedPtr<SListView<FGoogleSheetConfigWeakPtr>> ListView;
};
