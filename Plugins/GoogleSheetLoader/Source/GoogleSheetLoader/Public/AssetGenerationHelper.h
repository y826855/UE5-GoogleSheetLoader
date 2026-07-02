#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR
#include "IAssetTools.h"
#endif


class GOOGLESHEETLOADER_API AssetGenerationHelper
{
public:
	
	/** * [단일 생성] 특정 경로에 에셋을 로드하거나 생성함 
	 */
#if WITH_EDITOR
	static UPrimaryDataAsset* GetOrCreateAssetInternal(
		const FString& PackagePath,
		UClass* AssetClass,
		IAssetTools& AssetTools,
		IPlatformFile& PlatformFile,
		TSet<FString>& OutCreatedPaths);
	
	/** * [일괄 생성] 파싱된 행 데이터를 받아 TMap으로 묶어서 반환함 
	 * @param ParsedRows: 부모로부터 받은 2차원 배열 데이터
	 * @param FolderPath: 에셋이 저장될 폴더
	 * @param NameFormat: 에셋 이름 규칙 (예: DA_Item_{0})
	 * @param AssetClass: 생성할 에셋의 타입
	 */
	static TMap<int32, UPrimaryDataAsset*> GenerateAssetMap(
		const TArray<TArray<FString>>& ParsedRows, 
		const FString& FolderPath, 
		const FString& NameFormat, 
		UClass* AssetClass);
#endif
	
};

