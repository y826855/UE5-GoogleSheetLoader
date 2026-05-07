#include "GoogleSheetParserBase.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Dom/JsonObject.h"


// GoogleSheetParserBase.cpp
bool UGoogleSheetParserBase::Parse(const FString& RawResponse, FString& OutResult)
{
    ParsedRows.Empty();
    Headers.Empty();
    bIsParsed = false;

    // ── Step 1. 래퍼 제거 ──────────────────────────
    // "google.visualization.Query.setResponse(" ... ");"
    FString JsonStr = RawResponse;

    int32 Start = JsonStr.Find(TEXT("("));
    int32 End   = JsonStr.Find(TEXT(")"), ESearchCase::IgnoreCase,
                               ESearchDir::FromEnd);

    if (Start == INDEX_NONE || End == INDEX_NONE || End <= Start)
    {
        OutResult = TEXT("GViz 래퍼 파싱 실패");
        return false;
    }

    // ( ) 안쪽만 추출
    JsonStr = JsonStr.Mid(Start + 1, End - Start - 1);

    // ── Step 2. JSON 파싱 ──────────────────────────
    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        OutResult = TEXT("JSON 파싱 실패");
        return false;
    }

    // ── Step 3. table.cols → 헤더 추출 ────────────
    const TSharedPtr<FJsonObject>* TableObj;
    if (!Root->TryGetObjectField(TEXT("table"), TableObj))
    {
        OutResult = TEXT("table 필드 없음");
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>>* Cols;
    if (!(*TableObj)->TryGetArrayField(TEXT("cols"), Cols))
    {
        OutResult = TEXT("cols 필드 없음");
        return false;
    }

    for (const TSharedPtr<FJsonValue>& Col : *Cols)
    {
        FString Label;
        Col->AsObject()->TryGetStringField(TEXT("label"), Label);
        Headers.Add(Label.IsEmpty() ? TEXT("Unknown") : Label);
    }

    // ── Step 4. table.rows → 데이터 추출 ──────────
    const TArray<TSharedPtr<FJsonValue>>* Rows;
    if (!(*TableObj)->TryGetArrayField(TEXT("rows"), Rows))
    {
        OutResult = TEXT("rows 필드 없음");
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("[GoogleSheet] 데이터 행 추출 시작 (총 %d 행)"), Rows->Num());
    
    for (const TSharedPtr<FJsonValue>& RowVal : *Rows)
    {
        const TArray<TSharedPtr<FJsonValue>>* Cells;
        if (!RowVal->AsObject()->TryGetArrayField(TEXT("c"), Cells)) continue;

        TMap<FString, FString> Row;
        for (int32 j = 0; j < Headers.Num(); j++)
        {
            if (!Cells->IsValidIndex(j) || !(*Cells)[j].IsValid()
                || (*Cells)[j]->IsNull())
            {
                Row.Add(Headers[j], TEXT(""));
                continue;
            }

            const TSharedPtr<FJsonObject> Cell = (*Cells)[j]->AsObject();

            // "f" (formatted) 값 우선, 없으면 "v" (raw value)
            FString Value;
            if (!Cell->TryGetStringField(TEXT("f"), Value))
            {
                // f가 없으면 v를 문자열로 변환
                const TSharedPtr<FJsonValue>* VField = Cell->Values.Find(TEXT("v"));
                if (VField && VField->IsValid())
                {
                    if ((*VField)->Type == EJson::Number)
                        Value = FString::SanitizeFloat((*VField)->AsNumber());
                    else
                        (*VField)->TryGetString(Value);
                }
            }

            Row.Add(Headers[j], Value);
        }

        ParsedRows.Add(Row);
        OnRowParsed(Row);
    }

    bIsParsed = true;
    OnParseComplete();

    OutResult = FString::Printf(
        TEXT("파싱 완료 — %d 행, %d 컬럼"), ParsedRows.Num(), Headers.Num());
    return true;
}

// ── 검색 구현 ───────────────────────────────────
bool UGoogleSheetParserBase::GetRowAt(
    int32 Index,
    TMap<FString, FString>& OutRow) const
{
    if (!bIsParsed || !ParsedRows.IsValidIndex(Index)) return false;
    OutRow = ParsedRows[Index];
    return true;
}

//Parsing Data Utility Functions
template<typename TEnum>
TEnum UGoogleSheetParserBase::GetEnumValueFromString(const FString& EnumName, const FString& StringValue)
{
    const UEnum* EnumPtr = FindObject<UEnum>(nullptr, *EnumName, EFindObjectFlags::ExactClass);
    if (!EnumPtr) return TEnum(0);
    return static_cast<TEnum>(EnumPtr->GetValueByName(FName(*StringValue)));
}

FColor UGoogleSheetParserBase::ParseToColor(const FString& ColorString)
{
    return FColor::FromHex(ColorString);
}

FVector2D UGoogleSheetParserBase::ParseToVector2(const FString& RangeString)
{
    FString Left, Right;
    if (RangeString.Split(TEXT("~"), &Left, &Right) || RangeString.Split(TEXT(","), &Left, &Right))
    {
        return FVector2D(FCString::Atof(*Left), FCString::Atof(*Right));
    }
    return FVector2D(FCString::Atof(*RangeString), FCString::Atof(*RangeString));
}

FVector UGoogleSheetParserBase::ParseToVector(const FString& VectorString)
{
    if (VectorString.IsEmpty()) return FVector::ZeroVector;

    TArray<FString> Tokens;
    // 쉼표(,), 물결(~), 혹은 파이프(|) 중 원하는 구분자를 추가하세요.
    VectorString.ParseIntoArray(Tokens, TEXT(","), true);
    // 만약 쉼표가 없는데 물결이 있다면 물결로 다시 시도 (유연한 대응)
    if (Tokens.Num() == 1) VectorString.ParseIntoArray(Tokens, TEXT("~"), true);

    float X = Tokens.IsValidIndex(0) ? FCString::Atof(*Tokens[0].TrimStartAndEnd()) : 0.0f;
    float Y = Tokens.IsValidIndex(1) ? FCString::Atof(*Tokens[1].TrimStartAndEnd()) : 0.0f;
    float Z = Tokens.IsValidIndex(2) ? FCString::Atof(*Tokens[2].TrimStartAndEnd()) : 0.0f;

    //데이터가 하나일 때 모든 축을 동일하게 채움
    if (Tokens.Num() == 1)
        return FVector(X, X, X);
    return FVector(X, Y, Z);
}

bool UGoogleSheetParserBase::DoesAssetExist(const FString& AssetPath)
{
    return FPackageName::DoesPackageExist(AssetPath);
}
