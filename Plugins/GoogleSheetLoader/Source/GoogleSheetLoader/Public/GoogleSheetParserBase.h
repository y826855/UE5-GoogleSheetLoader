#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Dom/JsonObject.h"
#include "GoogleSheetParserBase.generated.h"

UCLASS(Abstract, BlueprintType, EditInlineNew)
class GOOGLESHEETLOADER_API UGoogleSheetParserBase : public UObject
{
    GENERATED_BODY()

public:
    // ── 파싱 ────────────────────────────────────

    // CSV 문자열을 받아서 내부 JSON 배열로 변환
    UFUNCTION(BlueprintCallable, Category="Google Sheet|Parser")
    bool Parse(const FString& RawCSV, FString& OutResult);

    // 특정 행 인덱스의 값 반환 (0부터 시작, 헤더 제외)
    UFUNCTION(BlueprintCallable, Category="Google Sheet|Parser")
    bool GetRowAt(int32 Index, TMap<FString, FString>& OutRow) const;

    // 전체 행 수 반환
    UFUNCTION(BlueprintCallable, Category="Google Sheet|Parser")
    int32 GetRowCount() const { return ParsedRows.Num(); }

    // 헤더(컬럼명) 목록 반환
    UFUNCTION(BlueprintCallable, Category="Google Sheet|Parser")
    TArray<FString> GetHeaders() const { return Headers; }


protected:
    // 자식 클래스에서 행 처리 커스텀 가능
    virtual void OnRowParsed(const TMap<FString, FString>& Row) {}
    virtual void OnParseComplete() {}
    
    // 파싱 완료된 데이터
    TArray<TMap<FString, FString>> ParsedRows;
    TArray<FString> Headers;
    bool bIsParsed = false;

protected:
    template<typename TEnum>
    TEnum GetEnumValueFromString(const FString& EnumName, const FString& StringValue);
 
    FColor ParseToColor(const FString& ColorString);

    FVector2D ParseToVector2(const FString& RangeString);
    FVector ParseToVector(const FString& VectorString);

    bool DoesAssetExist(const FString& AssetPath);
};
