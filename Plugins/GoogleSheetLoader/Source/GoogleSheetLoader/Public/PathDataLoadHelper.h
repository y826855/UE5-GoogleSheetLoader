#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "PathDataLoadHelper.generated.h"

UCLASS()
class GOOGLESHEETLOADER_API UPathDataLoadHelper : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    static bool IsUnsetPathValue(const FString& Value)
    {
        const FString TrimmedValue = Value.TrimStartAndEnd();
        return TrimmedValue.IsEmpty()
            || TrimmedValue.Equals(TEXT("None"), ESearchCase::IgnoreCase);
    }

    // 1. 경로 생성
    static FString MakeAssetReferencePath(const FString& FolderPath, const FString& AssetName);
    static FString MakePackagePath(const FString& FolderPath, const FString& AssetName);

    // 2. 범용 리소스 로더 (Soft Object Ptr) 
    template<typename T>
    static TSoftObjectPtr<T> LoadResource(const FString& Path)
    {
        return TSoftObjectPtr<T>(FSoftObjectPath(Path));
    }

    /** 3. 경로 내의 특정 포맷 에셋들을 모두 찾기 
     * @param FolderPath 탐색할 폴더 (예: /Game/Items)
     * @param NameFilter 이름 필터 (예: Item_*, * 기호 사용 가능)
     */
    UFUNCTION(BlueprintCallable, Category="Google Sheet|Helper")
    static TArray<FAssetData> GetAssetsByPathFilter(
        const FString& FolderPath,
        const FString& NameFilter);

    // 4. 템플릿 기반 GetOrCreate (에디터 전용) 
    template<typename T>
    static T* GetOrCreateAsset(
        const FString& FolderPath,
        const FString& NameFormat,
        UClass* SpecificClass = nullptr)
    {
        if (IsUnsetPathValue(FolderPath) || IsUnsetPathValue(NameFormat))
        {
            UE_LOG(LogTemp, Warning, TEXT("Asset path settings are empty. FolderPath: '%s', NameFormat: '%s'"), *FolderPath, *NameFormat);
            return nullptr;
        }

        UClass* ClassToUse = SpecificClass ? SpecificClass : T::StaticClass();
    
        FString PackagePath = MakePackagePath(FolderPath, NameFormat);
        FString AssetReferencePath = MakeAssetReferencePath(FolderPath, NameFormat);

        if (!FPackageName::IsValidLongPackageName(PackagePath))
        {
            UE_LOG(LogTemp, Warning, TEXT("Invalid package path: %s"), *PackagePath);
            return nullptr;
        }

        // [로드 시도] - 객체 경로 사용
        T* Loaded = Cast<T>(StaticLoadObject(ClassToUse, nullptr, *AssetReferencePath));
        if (Loaded) return Loaded;

#if WITH_EDITOR
        // [생성 시도] - 패키지 경로 사용 (중요!)
        UPackage* Package = CreatePackage(*PackagePath); 
        if (!Package) 
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to Create Package at: %s"), *PackagePath);
            return nullptr;
        }

        // NewObject의 이름 인자는 NameFormat(Item_12) 그대로 사용
        T* NewObj = NewObject<T>(Package, ClassToUse, *NameFormat, RF_Public | RF_Standalone);
        if (NewObj)
        {
            FAssetRegistryModule::AssetCreated(NewObj);
            NewObj->MarkPackageDirty();
            return NewObj;
        }
#endif
        return nullptr;
    }
};
