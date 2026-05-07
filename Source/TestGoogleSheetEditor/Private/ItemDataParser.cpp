// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemDataParser.h"

#include "PathDataLoadHelper.h"
#include "TestGoogleSheet/Data/ItemDataStructure.h"

void UItemDataParser::OnParseComplete()
{
	UE_LOG(LogTemp, Log, TEXT("get row: %d"), GetRowCount());
	
	for (int32 i = 0; i < GetRowCount(); ++i)
	{
		TMap<FString, FString> RowData;
		if (GetRowAt(i, RowData))
		{
			auto ID = RowData.FindRef(TEXT("ID"));
			auto DataAssetFileName = FString::Format(*AssetNameFormat, {ID}); 
			auto DataAsset = UPathDataLoadHelper::GetOrCreateAsset
				<UItemDataAsset>(AssetFolderPath, DataAssetFileName);
			
			FItemDataStructure NewRow;
			NewRow.ItemID = FName(ID);
			NewRow.DisplayName = RowData.FindRef(TEXT("DisplayName"));
			NewRow.Description  =  RowData.FindRef(TEXT("Description"));
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
	
	auto iconFileName = FString::Format(*SpriteFileFormat, { ID.TrimStartAndEnd() });
	auto iconPath = UPathDataLoadHelper::MakeAssetReferencePath(SpriteFolderPath, iconFileName);

	
	ItemAsset->Icon = UPathDataLoadHelper::LoadResource<UPaperSprite>(iconPath);

	return ItemAsset;
}

