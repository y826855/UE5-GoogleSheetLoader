#include "GoogleSheetConfigCustomization.h"
#include "GoogleSheetConfig.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"

TSharedRef<IDetailCustomization> FGoogleSheetConfigCustomization::MakeInstance()
{
    return MakeShareable(new FGoogleSheetConfigCustomization);
}

void FGoogleSheetConfigCustomization::CustomizeDetails(
    IDetailLayoutBuilder& DetailBuilder)
{
    TArray<TWeakObjectPtr<UObject>> Objects;
    DetailBuilder.GetObjectsBeingCustomized(Objects);
    if (Objects.IsEmpty()) return;

    Config = Cast<UGoogleSheetConfig>(Objects[0].Get());
    if (!Config.IsValid()) return;

    LayoutBuilder = &DetailBuilder;

    IDetailCategoryBuilder& Cat =
        DetailBuilder.EditCategory("Actions",
            FText::FromString("Actions"),
            ECategoryPriority::Important);

    // ── 요약 정보 표시 ──────────────────────────
    Cat.AddCustomRow(FText::FromString("Summary"))
    [
        SNew(SBox).Padding(FMargin(4.f, 6.f))
        [
            SNew(SVerticalBox)

            // Spreadsheet ID 한줄 요약
            + SVerticalBox::Slot().AutoHeight().Padding(0, 2)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().Padding(0,0,6,0)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString("Sheet ID"))
                    .ColorAndOpacity(FSlateColor::UseSubduedForeground())
                ]
                + SHorizontalBox::Slot()
                [
                    SNew(STextBlock)
                    .Text_Lambda([this]()
                    {
                        if (!Config.IsValid()) return FText::GetEmpty();
                        FString ID = Config->GetSpreadsheetID();
                        // 너무 길면 앞 8자만
                        return FText::FromString(
                            ID.Len() > 8 ? ID.Left(8) + TEXT("...") : ID);
                    })
                ]
            ]

            // URL에서 자동으로 찾은 시트 탭 ID
            + SVerticalBox::Slot().AutoHeight().Padding(0, 2)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().Padding(0,0,6,0)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString("Sheet GID"))
                    .ColorAndOpacity(FSlateColor::UseSubduedForeground())
                ]
                + SHorizontalBox::Slot()
                [
                    SNew(STextBlock)
                    .Text_Lambda([this]()
                    {
                        return Config.IsValid()
                            ? FText::FromString(Config->GetSheetGid())
                            : FText::GetEmpty();
                    })
                ]
            ]

            // 범위 요약
            + SVerticalBox::Slot().AutoHeight().Padding(0, 2)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().Padding(0,0,6,0)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString("Range"))
                    .ColorAndOpacity(FSlateColor::UseSubduedForeground())
                ]
                + SHorizontalBox::Slot()
                [
                    SNew(STextBlock)
                    .Text_Lambda([this]()
                    {
                        if (!Config.IsValid()) return FText::GetEmpty();
                        return FText::FromString(Config->GetRangeString());
                    })
                ]
            ]
        ]
    ];

    // ── 상태 표시 위젯 ──────────────────────────
    Cat.AddCustomRow(FText::FromString("Status"))
    [
        SNew(SBox).Padding(FMargin(4.f, 4.f))
        [
            SNew(SHorizontalBox)

            // 상태 아이콘
            + SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0).VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text_Lambda([this]() -> FText
                {
                    if (!Config.IsValid()) return FText::GetEmpty();
                    switch (Config->FetchStatus)
                    {
                    case EFetchStatus::Success: return FText::FromString(TEXT("✅"));
                    case EFetchStatus::Failed:  return FText::FromString(TEXT("❌"));
                    case EFetchStatus::Loading: return FText::FromString(TEXT("⏳"));
                    default:                    return FText::FromString(TEXT("⬜"));
                    }
                })
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
            ]

            // 상태 메시지
            + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text_Lambda([this]() -> FText
                {
                    if (!Config.IsValid()) return FText::GetEmpty();
                    FString Msg = Config->LastMessage;
                    if (!Config->LastFetchTime.IsEmpty())
                        Msg += TEXT("  (") + Config->LastFetchTime + TEXT(")");
                    return FText::FromString(Msg);
                })
                .ColorAndOpacity_Lambda([this]() -> FSlateColor
                {
                    if (!Config.IsValid()) return FSlateColor::UseForeground();
                    switch (Config->FetchStatus)
                    {
                    case EFetchStatus::Success:
                        return FSlateColor(FLinearColor(0.2f, 0.8f, 0.2f));
                    case EFetchStatus::Failed:
                        return FSlateColor(FLinearColor(0.9f, 0.2f, 0.2f));
                    case EFetchStatus::Loading:
                        return FSlateColor(FLinearColor(0.9f, 0.7f, 0.1f));
                    default:
                        return FSlateColor::UseSubduedForeground();
                    }
                })
                .AutoWrapText(true)
            ]
        ]
    ];

    // ── 페치 버튼 ───────────────────────────────
    Cat.AddCustomRow(FText::FromString("Fetch"))
    [
        SNew(SBox).Padding(FMargin(4.f, 4.f))
        [
            SNew(SButton)
            .HAlign(HAlign_Center)
            .IsEnabled_Lambda([this]()
            {
                // 로딩 중엔 버튼 비활성화
                return Config.IsValid() &&
                    Config->FetchStatus != EFetchStatus::Loading;
            })
            .Text_Lambda([this]() -> FText
            {
                if (Config.IsValid() &&
                    Config->FetchStatus == EFetchStatus::Loading)
                    return FText::FromString(TEXT("Loading..."));
                return FText::FromString(TEXT("📥  Load Google Sheet Data"));
            })
            .OnClicked(this,
                &FGoogleSheetConfigCustomization::OnFetchClicked)
        ]
    ];
}

FReply FGoogleSheetConfigCustomization::OnFetchClicked()
{
    if (!Config.IsValid()) return FReply::Handled();

    Config->Fetch();

    // 상태가 바뀌었으니 Detail 패널 리프레시
    if (LayoutBuilder)
        LayoutBuilder->ForceRefreshDetails();

    return FReply::Handled();
}
