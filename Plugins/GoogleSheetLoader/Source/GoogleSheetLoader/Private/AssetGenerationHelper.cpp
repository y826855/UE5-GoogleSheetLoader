#include "AssetGenerationHelper.h"

#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Misc/PackageName.h"
#include "Misc/ScopedSlowTask.h"
#include "UObject/SoftObjectPath.h"

namespace
{
    bool NormalizeGameFolderPath(const FString& InFolderPath, FString& OutFolderPath)
    {
        OutFolderPath = InFolderPath.TrimStartAndEnd();
        OutFolderPath.ReplaceInline(TEXT("\\"), TEXT("/"));

        while (OutFolderPath.Len() > 5 && OutFolderPath.EndsWith(TEXT("/")))
        {
            OutFolderPath.LeftChopInline(1);
        }

        if (OutFolderPath != TEXT("/Game") && !OutFolderPath.StartsWith(TEXT("/Game/")))
        {
            UE_LOG(LogTemp, Error, TEXT("[AssetHelper] FolderPath must be '/Game' or start with '/Game/': %s"), *InFolderPath);
            return false;
        }

        FString UnusedFilename;
        if (!FPackageName::TryConvertLongPackageNameToFilename(OutFolderPath, UnusedFilename))
        {
            UE_LOG(LogTemp, Error, TEXT("[AssetHelper] Invalid game folder path: %s"), *OutFolderPath);
            return false;
        }

        return true;
    }

    bool NormalizeAndValidatePackagePath(const FString& InPackagePath, FString& OutPackagePath)
    {
        OutPackagePath = InPackagePath.TrimStartAndEnd();
        OutPackagePath.ReplaceInline(TEXT("\\"), TEXT("/"));

        if (OutPackagePath.IsEmpty())
        {
            UE_LOG(LogTemp, Error, TEXT("[AssetHelper] PackagePath is empty."));
            return false;
        }

        if (!OutPackagePath.StartsWith(TEXT("/Game/")))
        {
            UE_LOG(LogTemp, Error, TEXT("[AssetHelper] PackagePath must start with '/Game/': %s"), *OutPackagePath);
            return false;
        }

        FText Reason;
        if (!FPackageName::IsValidLongPackageName(OutPackagePath, false, &Reason))
        {
            UE_LOG(LogTemp, Error, TEXT("[AssetHelper] Invalid package path '%s': %s"), *OutPackagePath, *Reason.ToString());
            return false;
        }

        const FString AssetName = FPackageName::GetLongPackageAssetName(OutPackagePath);
        const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *OutPackagePath, *AssetName);
        if (!FPackageName::IsValidObjectPath(ObjectPath, &Reason))
        {
            UE_LOG(LogTemp, Error, TEXT("[AssetHelper] Invalid asset object path '%s': %s"), *ObjectPath, *Reason.ToString());
            return false;
        }

        return true;
    }

    FString MakeObjectPathFromPackagePath(const FString& PackagePath)
    {
        const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
        return FString::Printf(TEXT("%s.%s"), *PackagePath, *AssetName);
    }
}

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

    FString NormalizedFolderPath;
    if (!NormalizeGameFolderPath(FolderPath, NormalizedFolderPath))
    {
        return ResultMap;
    }

    // --- [최적화 포인트 1] 루프 진입 전 인터페이스 캐싱 ---
    // 루프 안에서 매번 LoadModuleChecked를 하지 않도록 여기서 한 번만 가져옵니다.
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    TSet<FString> CreatedPaths;

    FScopedSlowTask Progress(static_cast<float>(ParsedRows.Num()), FText::FromString(TEXT("Generating Data Assets...")));
    Progress.MakeDialog(true); // 취소 가능하도록 설정

    for (const TArray<FString>& Row : ParsedRows)
    {
        if (Progress.ShouldCancel())
        {
            break;
        }

        Progress.EnterProgressFrame(1.f);

        if (Row.Num() < 1) continue;

        int32 ID = FCString::Atoi(*Row[0]);
        if (ID <= 0) continue;

        const FString AssetName = FString::Format(*NameFormat, { ID }).TrimStartAndEnd();
        if (AssetName.IsEmpty())
        {
            UE_LOG(LogTemp, Error, TEXT("[AssetHelper] AssetName is empty. ID: %d"), ID);
            continue;
        }

        // 경로 생성 (예: /Game/Items/DataAssets/DA_Item_101)
        FString PackagePath = FPaths::Combine(*NormalizedFolderPath, *AssetName);

        // --- [최적화 포인트 2] 내부 전용 함수에 캐싱된 도구 전달 ---
        UPrimaryDataAsset* Asset = GetOrCreateAssetInternal(PackagePath, AssetClass, AssetTools, PlatformFile, CreatedPaths);

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
    IPlatformFile& PlatformFile,
    TSet<FString>& OutCreatedPaths)
{
    FString NormalizedPackagePath;
    if (!AssetClass || !NormalizeAndValidatePackagePath(PackagePath, NormalizedPackagePath))
    {
        return nullptr;
    }

    const FString ObjectPath = MakeObjectPathFromPackagePath(NormalizedPackagePath);

    // 1. 이미 존재하는지 먼저 확인 (메모리에 있거나 로드 가능하면 가져옴)
    UObject* ExistingObject = FSoftObjectPath(ObjectPath).TryLoad();
    UPrimaryDataAsset* TargetAsset = Cast<UPrimaryDataAsset>(ExistingObject);
    if (ExistingObject && !TargetAsset)
    {
        UE_LOG(LogTemp, Error, TEXT("[AssetHelper] Existing asset is not a UPrimaryDataAsset: %s"), *ObjectPath);
        return nullptr;
    }

    // 2. 에디터 환경에서 에셋이 없는 경우 새로 생성
    if (!TargetAsset)
    {
        FString LongPackagePath = FPackageName::GetLongPackagePath(NormalizedPackagePath);
        FString PhysPath;
        if (!FPackageName::TryConvertLongPackageNameToFilename(LongPackagePath, PhysPath))
        {
            UE_LOG(LogTemp, Error, TEXT("[AssetHelper] Failed to convert package path to filename: %s"), *LongPackagePath);
            return nullptr;
        }

        if (!OutCreatedPaths.Contains(PhysPath) && !PlatformFile.DirectoryExists(*PhysPath))
        {
            PlatformFile.CreateDirectoryTree(*PhysPath);
            OutCreatedPaths.Add(PhysPath);
        }

        // 에셋 생성
        FString AssetName = FPackageName::GetLongPackageAssetName(NormalizedPackagePath);
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
            FString FileName = FPackageName::LongPackageNameToFilename(NormalizedPackagePath, FPackageName::GetAssetPackageExtension());
            UPackage::SavePackage(Package, TargetAsset, *FileName, SaveArgs);
            */
        }
    }

    return TargetAsset;
}
