#include "PathDataLoadHelper.h"

FString UPathDataLoadHelper::MakeAssetReferencePath(const FString& FolderPath, const FString& AssetName)
{
	if (IsUnsetPathValue(FolderPath) || IsUnsetPathValue(AssetName))
	{
		return FString();
	}

	FString PackagePath = FPaths::Combine(*FolderPath, *AssetName);
    
	return FString::Printf(TEXT("%s.%s"), *PackagePath, *AssetName);
}

FString UPathDataLoadHelper::MakePackagePath(const FString& FolderPath, const FString& AssetName)
{
	if (IsUnsetPathValue(FolderPath) || IsUnsetPathValue(AssetName))
	{
		return FString();
	}

	return FPaths::Combine(*FolderPath, *AssetName);
}

TArray<FAssetData> UPathDataLoadHelper::GetAssetsByPathFilter(const FString& FolderPath, const FString& NameFilter)
{
	TArray<FAssetData> AssetDatas;

	if (IsUnsetPathValue(FolderPath) || IsUnsetPathValue(NameFilter))
	{
		return AssetDatas;
	}
        
	// Asset Registry 모듈 로드
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        
	// 필터 설정
	FARFilter Filter;
	Filter.PackagePaths.Add(*FolderPath);
	Filter.bRecursivePaths = false; // 해당 폴더만 (필요시 true)

	TArray<FAssetData> TempList;
	AssetRegistryModule.Get().GetAssets(Filter, TempList);

	// 이름 패턴 매칭 (Wildcard 지원)
	for (const FAssetData& Asset : TempList)
	{
		if (Asset.AssetName.ToString().MatchesWildcard(NameFilter))
		{
			AssetDatas.Add(Asset);
		}
	}

	return AssetDatas;
}

