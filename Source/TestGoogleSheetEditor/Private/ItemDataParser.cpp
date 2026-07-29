// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemDataParser.h"

#include "PathDataLoadHelper.h"
#include "SheetDataTableUtils.h"
#include "SheetParserUtils.h"
#include "SheetValidation.h"
#include "TestGoogleSheet/Data/ItemDataStructure.h"

using namespace SheetDataTableUtils;
using namespace SheetParserUtils;
using namespace SheetValidation;

UItemDataParser::UItemDataParser()
{
	SpriteFolderPath.Path = TEXT("/Game/Icons");
	AssetFolderPath.Path = TEXT("/Game/Items/DataAssets");
}

bool UItemDataParser::OnParseComplete(FString& OutError)
{
	constexpr const TCHAR* ParserName = TEXT("ItemData");
	FParseReport Report;
	OutError.Reset();
	LogHeaders(ParserName, GetHeaders());

	if (!ValidateTargetTable(TargetTable, FItemDataStructure::StaticStruct()))
	{
		LogTargetTableError(
			ParserName,
			TargetTable,
			FItemDataStructure::StaticStruct());
		OutError = TEXT("대상 DataTable 또는 RowStruct가 올바르지 않습니다.");
		return false;
	}

	if (!ValidateRequiredHeaders(
		GetHeaders(),
		{
			TEXT("ID"),
			TEXT("DisplayName"),
			TEXT("Description"),
			TEXT("MaxStack"),
			TEXT("테스트")
		},
		&Report))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[Sheet][%s] Required headers are missing."),
			ParserName);
		OutError = TEXT("필수 헤더가 없습니다.");
		return false;
	}

	if (IsUnsetValue(AssetFolderPath.Path)
		|| IsUnsetValue(AssetNameFormat))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[Sheet][%s] Asset path settings are empty."),
			ParserName);
		OutError = TEXT("에셋 경로 설정이 비어 있습니다.");
		return false;
	}

	FScopedDataTableEditNotification TableEdit(TargetTable);
	if (!TableEdit.IsActive())
	{
		OutError = TEXT("DataTable 편집을 시작하지 못했습니다.");
		return false;
	}

	for (int32 i = 0; i < GetRowCount(); ++i)
	{
		TMap<FString, FString> RowData;
		if (!GetRowAt(i, RowData))
		{
			continue;
		}

		FString ID;
		if (!GetRequiredCell(
			RowData,
			TEXT("ID"),
			ID,
			&Report,
			i))
		{
			continue;
		}

		const auto ParseRequiredIntCell =
			[&RowData, &Report, i](
				const FString& ColumnName,
				const FName RowName,
				int32& OutValue)
			{
				FString SourceValue;
				if (!GetRequiredCell(
					RowData,
					ColumnName,
					SourceValue,
					&Report,
					i,
					RowName))
				{
					return false;
				}

				if (LexTryParseString(OutValue, *SourceValue))
				{
					return true;
				}

				Report.AddIssue(
					EParseIssueSeverity::Error,
					TEXT("정수로 변환할 수 없습니다."),
					i,
					RowName,
					ColumnName,
					SourceValue);
				return false;
			};

		const FName RowName = ParseNameValue(ID);
		if (RowName.IsNone())
		{
			Report.AddIssue(
				EParseIssueSeverity::Error,
				TEXT("ID could not be converted to a row name."),
				i,
				NAME_None,
				TEXT("ID"),
				ID);
			continue;
		}

		int32 MaxStack = 0;
		int32 TestValue = 0;
		if (!ParseRequiredIntCell(TEXT("MaxStack"), RowName, MaxStack)
			|| !ParseRequiredIntCell(TEXT("테스트"), RowName, TestValue))
		{
			continue;
		}

		UItemDataAsset* DataAsset = GetOrCreateDataAsset<UItemDataAsset>(
			AssetFolderPath.Path,
			AssetNameFormat,
			RowName);

		FItemDataStructure NewRow;
		NewRow.ItemID = RowName;
		NewRow.DisplayName = GetCellOrDefault(RowData, TEXT("DisplayName"));
		NewRow.Description = GetCellOrDefault(RowData, TEXT("Description"));
		NewRow.MaxStack = MaxStack;
		NewRow.ItemDataAsset = SetupItemAsset(DataAsset, ID);

		UE_LOG(
			LogTemp,
			Verbose,
			TEXT("[Sheet][%s] 테스트 값: %d"),
			ParserName,
			TestValue);

		TargetTable->AddRow(NewRow.ItemID, NewRow);
		++Report.SuccessCount;
	}

	for (const FParseIssue& Issue : Report.Issues)
	{
		if (Issue.Severity == EParseIssueSeverity::Error)
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("[Sheet][%s] Row=%d Column=%s Value='%s' %s"),
				ParserName,
				Issue.RowIndex,
				*Issue.ColumnName,
				*Issue.SourceValue,
				*Issue.Message);
		}
		else
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[Sheet][%s] Row=%d Column=%s Value='%s' %s"),
				ParserName,
				Issue.RowIndex,
				*Issue.ColumnName,
				*Issue.SourceValue,
				*Issue.Message);
		}
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[Sheet][%s] Success=%d Warning=%d Error=%d"),
		ParserName,
		Report.SuccessCount,
		Report.WarningCount,
		Report.ErrorCount);

	if (Report.HasErrors())
	{
		OutError = FString::Printf(
			TEXT("%d개 행 처리 오류가 발생했습니다."),
			Report.ErrorCount);
		return false;
	}

	return true;
}

UItemDataAsset* UItemDataParser::SetupItemAsset(UItemDataAsset* ItemAsset, FString ID)
{
	if (ItemAsset == nullptr) return nullptr;

	if (IsUnsetValue(ID)
		|| IsUnsetValue(SpriteFolderPath.Path)
		|| IsUnsetValue(SpriteFileFormat))
	{
		UE_LOG(LogTemp, Warning, TEXT("Skip icon setup because sprite settings are empty."));
		return ItemAsset;
	}
	
	auto iconFileName = FString::Format(*SpriteFileFormat, { ID.TrimStartAndEnd() });
	auto iconPath = UPathDataLoadHelper::MakeAssetReferencePath(SpriteFolderPath.Path, iconFileName);
	
	ItemAsset->Icon = UPathDataLoadHelper::LoadResource<UPaperSprite>(iconPath);

	return ItemAsset;
}
