// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemDataParser.h"

#include "Parser/SheetDataTableUtils.h"
#include "Parser/SheetParserUtils.h"
#include "Parser/SheetValidation.h"
#include "TestGoogleSheet/Data/ItemDataStructure.h"

using namespace SheetDataTableUtils;
using namespace SheetParserUtils;
using namespace SheetValidation;

static constexpr const TCHAR* ParserName = TEXT("ItemData");

// 시트 컬럼 정의
namespace SheetColumns
{
	const FString ID = TEXT("ID");
	const FString DisplayName = TEXT("DisplayName");
	const FString Description = TEXT("Description");
	const FString MaxStack = TEXT("MaxStack");

	const TArray<FString> RequiredHeaders =
	{
		ID,
		DisplayName,
		Description,
		MaxStack
	};
}

UItemDataParser::UItemDataParser()
{
	SpriteFolderPath.Path = TEXT("/Game/Icons");
	AssetFolderPath.Path = TEXT("/Game/Items/DataAssets");
}

bool UItemDataParser::OnParseComplete(FString& OutError)
{
	FParseReport Report;
	OutError.Reset();
	LogHeaders(ParserName, GetHeaders());

	// 파서 설정 검증
	if (!ValidateParserSetup(Report, OutError))
	{
		return false;
	}

	// DataTable 변경 Scope
	FScopedDataTableEditNotification TableEdit(TargetTable);

	for (int32 Index = 0; Index < GetRowCount(); ++Index)
	{
		TMap<FString, FString> RowData;
		if (!GetRowAt(Index, RowData))
		{
			continue;
		}

		// 행 데이터 변환
		FSheetRowReader Row(RowData, Index, Report);
		FItemDataStructure NewRow;
		NewRow.ItemID = Row.GetRequiredName(SheetColumns::ID);
		NewRow.DisplayName = Row.Get(SheetColumns::DisplayName);
		NewRow.Description = Row.Get(SheetColumns::Description);
		NewRow.MaxStack = Row.GetRequiredInt(SheetColumns::MaxStack);

		if (!Row.IsValid())
		{
			continue;
		}

		// DataAsset 생성
		UItemDataAsset* DataAsset = GetOrCreateDataAsset<UItemDataAsset>(
			AssetFolderPath.Path,
			AssetNameFormat,
			NewRow.ItemID);
		NewRow.ItemDataAsset = SetupItemAsset(
			DataAsset,
			Row);

		TargetTable->AddRow(NewRow.ItemID, NewRow);
		Report.AddSuccess();
	}

	return FinalizeParseReport(ParserName, Report, OutError);
}

bool UItemDataParser::ValidateParserSetup(
	FParseReport& Report,
	FString& OutError) const
{
	// DataTable 검증
	if (!ValidateTargetTable(
		ParserName,
		TargetTable,
		FItemDataStructure::StaticStruct(),
		OutError))
	{
		return false;
	}

	// 필수 헤더 검증
	if (!ValidateRequiredHeaders(
		GetHeaders(),
		SheetColumns::RequiredHeaders,
		&Report))
	{
		return FinalizeParseReport(ParserName, Report, OutError);
	}

	// 필수 경로 검증
	return ValidateRequiredSettings(
		ParserName,
		{
			{ TEXT("AssetFolderPath"), AssetFolderPath.Path },
			{ TEXT("AssetNameFormat"), AssetNameFormat }
		},
		OutError);
}

UItemDataAsset* UItemDataParser::SetupItemAsset(
	UItemDataAsset* ItemAsset,
	FSheetRowReader& Row)
{
	// DataAsset 검증
	if (!ItemAsset)
	{
		return nullptr;
	}

	// 아이콘 설정
	Row.AssignOptionalAsset(
		ItemAsset->Icon,
		SpriteFolderPath.Path,
		SpriteFileFormat,
		TEXT("Icon"));

	return ItemAsset;
}
