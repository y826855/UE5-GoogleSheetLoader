#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GoogleSheetParserBase.generated.h"

UCLASS(Abstract, BlueprintType, EditInlineNew)
class GOOGLESHEETLOADER_API UGoogleSheetParserBase : public UObject
{
    GENERATED_BODY()

public:
    // 플러그인이 생성한 정규화 JSON을 파싱합니다.
    bool Parse(const FString& NormalizedJson, FString& OutResult);

protected:
    // 특정 행 인덱스의 값 반환 (0부터 시작, 헤더 제외)
    bool GetRowAt(int32 Index, TMap<FString, FString>& OutRow) const;

    // 전체 행 수 반환
    int32 GetRowCount() const { return ParsedRows.Num(); }

    // 헤더(컬럼명) 목록 반환
    TArray<FString> GetHeaders() const { return Headers; }

    // 자식 클래스에서 행 처리 커스텀 가능
    virtual void OnRowParsed(const TMap<FString, FString>& Row) {}

    // 전체 행 처리 후 호출하며 실패 사유를 OutError에 기록합니다.
    virtual bool OnParseComplete(FString&) { return true; }

    void ClearParsedData();
    
    // 파싱 완료된 데이터
    TArray<TMap<FString, FString>> ParsedRows;
    TArray<FString> Headers;
};
