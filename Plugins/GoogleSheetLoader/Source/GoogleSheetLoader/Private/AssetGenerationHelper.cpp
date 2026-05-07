#include "AssetGenerationHelper.h"

#include "AssetToolsModule.h"
#include "IAssetTools.h"

/**
 * [일괄 생성 및 로드]
 * 외부(Fetcher)에서는 이 함수만 호출하면 됩니다.
 */
TMap<int32, UPrimaryDataAsset*> AssetGenerationHelper::GenerateAssetMap(
    const TArray<TArray<FString>>& ParsedRows,
    const FString& FolderPath,
    const FString& NameFormat,
    UClass* AssetClass)
{
    TMap<int32, UPrimaryDataAsset*> ResultMap;

    if (ParsedRows.Num() == 0 || !AssetClass) return ResultMap;

    // --- [최적화 포인트 1] 루프 진입 전 인터페이스 캐싱 ---
    // 루프 안에서 매번 LoadModuleChecked를 하지 않도록 여기서 한 번만 가져옵니다.
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

    for (const TArray<FString>& Row : ParsedRows)
    {
        if (Row.Num() < 1) continue;
        
        int32 ID = FCString::Atoi(*Row[0]);
        if (ID <= 0) continue;

        // 경로 생성 (예: /Game/Items/DataAssets/DA_Item_101)
        FString PackagePath = FPaths::Combine(*FolderPath, *FString::Format(*NameFormat, { ID }));
        
        // --- [최적화 포인트 2] 내부 전용 함수에 캐싱된 도구 전달 ---
        UPrimaryDataAsset* Asset = GetOrCreateAssetInternal(PackagePath, AssetClass, AssetTools, PlatformFile);
        
        if (Asset)
        {
            ResultMap.Add(ID, Asset);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[AssetHelper] 총 %d개의 에셋을 준비했습니다."), ResultMap.Num());
    return ResultMap;
}

/**
 * [단일 에셋 생성/로드 - 내부용]
 * 매개변수로 도구들을 직접 받아서 성능 저하를 방지합니다.
 */
UPrimaryDataAsset* AssetGenerationHelper::GetOrCreateAssetInternal(
    const FString& PackagePath,
    UClass* AssetClass,
    IAssetTools& AssetTools,
    IPlatformFile& PlatformFile)
{
    // 1. 이미 존재하는지 먼저 확인 (메모리에 있거나 로드 가능하면 가져옴)
    UPrimaryDataAsset* TargetAsset = Cast<UPrimaryDataAsset>(StaticLoadObject(AssetClass, nullptr, *PackagePath));

    // 2. 에디터 환경에서 에셋이 없는 경우 새로 생성
    if (!TargetAsset)
    {
        // 폴더 경로 추출 및 물리 경로 확인 (RightChop(6)는 "/Game/" 제거용)
        FString LongPackagePath = FPackageName::GetLongPackagePath(PackagePath);
        FString PhysPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + LongPackagePath.RightChop(6));

        if (!PlatformFile.DirectoryExists(*PhysPath))
        {
            PlatformFile.CreateDirectoryTree(*PhysPath);
        }

        // 에셋 생성
        
        FString AssetName = FPaths::GetBaseFilename(PackagePath);
        UObject* NewObj = AssetTools.CreateAsset(AssetName, LongPackagePath, AssetClass, nullptr);
        TargetAsset = Cast<UPrimaryDataAsset>(NewObj);

        if (TargetAsset)
        {
            // 에셋 생성 성공 시 'Dirty' 표시 (사용자가 저장하거나 마지막에 한꺼번에 저장하도록 유도)
            TargetAsset->MarkPackageDirty();

            // 만약 무조건 즉시 물리 파일로 저장해야 한다면 아래 주석 해제
            /*
            UPackage* Package = TargetAsset->GetOutermost();
            FSavePackageArgs SaveArgs;
            SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
            FString FileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
            UPackage::SavePackage(Package, TargetAsset, *FileName, SaveArgs);
            */
        }
    }

    return TargetAsset;
}

