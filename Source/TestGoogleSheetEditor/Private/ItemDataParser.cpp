// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemDataParser.h"

#include "PathDataLoadHelper.h"
#include "TestGoogleSheet/Data/ItemDataStructure.h"

namespace
{
	bool IsUnsetConfigValue(const FString& Value)
	{
		const FString TrimmedValue = Value.TrimStartAndEnd();
		return TrimmedValue.IsEmpty()
			|| TrimmedValue.Equals(TEXT("None"), ESearchCase::IgnoreCase);
	}
}

UItemDataParser::UItemDataParser()
{
	SpriteFolderPath.Path = TEXT("/Game/Icons");
	AssetFolderPath.Path = TEXT("/Game/Items/DataAssets");
}

void UItemDataParser::OnParseComplete()
{
	UE_LOG(LogTemp, Log, TEXT("get row: %d"), GetRowCount());

	if (!IsValid(TargetTable))
	{
		UE_LOG(LogTemp, Error, TEXT("TargetTable is empty. Parsed rows will not be applied."));
		return;
	}
	
	for (int32 i = 0; i < GetRowCount(); ++i)
	{
		TMap<FString, FString> RowData;
		if (GetRowAt(i, RowData))
		{
			const FString ID = RowData.FindRef(TEXT("ID")).TrimStartAndEnd();
			if (IsUnsetConfigValue(ID))
			{
				UE_LOG(LogTemp, Warning, TEXT("Skip row %d because ID is empty."), i);
				continue;
			}

			if (IsUnsetConfigValue(AssetFolderPath.Path) || IsUnsetConfigValue(AssetNameFormat))
			{
				UE_LOG(LogTemp, Warning, TEXT("Skip row %d because asset path settings are empty."), i);
				continue;
			}

			const FString DataAssetFileName = FString::Format(*AssetNameFormat, {ID}); 
			UItemDataAsset* DataAsset = UPathDataLoadHelper::GetOrCreateAsset
				<UItemDataAsset>(AssetFolderPath.Path, DataAssetFileName);
			
			FItemDataStructure NewRow;
			NewRow.ItemID = FName(ID);
			NewRow.DisplayName = RowData.FindRef(TEXT("DisplayName"));
			NewRow.Description  =  RowData.FindRef(TEXT("Description"));
			//NewRow.MaxStack  = //파싱  RowData.FindRef(TEXT("MaxStack"));
			NewRow.ItemDataAsset = SetupItemAsset(DataAsset, ID);
            
			UE_LOG(LogTemp, Log, TEXT("이름: %s 설명: %s")
				, *NewRow.DisplayName, *NewRow.Description);

			TargetTable->AddRow(NewRow.ItemID, NewRow);
		}
	}
}

UItemDataAsset* UItemDataParser::SetupItemAsset(UItemDataAsset* ItemAsset, FString ID)
{
	if (ItemAsset == nullptr) return nullptr;

	if (IsUnsetConfigValue(ID) || IsUnsetConfigValue(SpriteFolderPath.Path) || IsUnsetConfigValue(SpriteFileFormat))
	{
		UE_LOG(LogTemp, Warning, TEXT("Skip icon setup because sprite settings are empty."));
		return ItemAsset;
	}
	
	auto iconFileName = FString::Format(*SpriteFileFormat, { ID.TrimStartAndEnd() });
	auto iconPath = UPathDataLoadHelper::MakeAssetReferencePath(SpriteFolderPath.Path, iconFileName);
	
	ItemAsset->Icon = UPathDataLoadHelper::LoadResource<UPaperSprite>(iconPath);

	return ItemAsset;
}
