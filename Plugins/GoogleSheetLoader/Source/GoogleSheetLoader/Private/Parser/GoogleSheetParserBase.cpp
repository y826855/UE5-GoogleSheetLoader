#include "GoogleSheetParserBase.h"
#include "GoogleSheetTableData.h"
#include "Misc/ScopedSlowTask.h"

bool UGoogleSheetParserBase::Parse(const FString& NormalizedJson, FString& OutResult)
{
    ClearParsedData();
    OutResult.Reset();

    if (NormalizedJson.TrimStartAndEnd().IsEmpty())
    {
        OutResult = TEXT("정규화 JSON이 비어 있습니다.");
        return false;
    }

    FGoogleSheetTableData TableData;
    if (!FGoogleSheetTableData::FromJson(NormalizedJson, TableData, OutResult))
    {
        return false;
    }

    Headers = TableData.Headers;
    UE_LOG(LogTemp, Log, TEXT("[GoogleSheet] 데이터 행 추출 시작 (총 %d 행)"), TableData.Rows.Num());
    
    // 프로그래스 바 설정
    FScopedSlowTask ParseProgress(static_cast<float>(TableData.Rows.Num()), FText::FromString(TEXT("Parsing Google Sheet Data...")));
    ParseProgress.MakeDialog(true); // 취소 버튼이 보이도록 설정

    for (int32 i = 0; i < TableData.Rows.Num(); ++i)
    {
        // 사용자가 취소를 눌렀는지 확인
        if (ParseProgress.ShouldCancel())
        {
            OutResult = TEXT("사용자가 파싱을 취소했습니다.");
            ClearParsedData();
            return false;
        }

        ParseProgress.EnterProgressFrame(1.f, FText::Format(FText::FromString(TEXT("Parsing Row {0} of {1}")), FText::AsNumber(i + 1), FText::AsNumber(TableData.Rows.Num())));

        TMap<FString, FString> Row;
        for (int32 j = 0; j < Headers.Num(); j++)
        {
            Row.Add(Headers[j], TableData.Rows[i][j]);
        }

        ParsedRows.Add(Row);
        OnRowParsed(Row);
    }

    if (!OnParseComplete(OutResult))
    {
        if (OutResult.IsEmpty())
        {
            OutResult = TEXT("사용자 파서가 데이터 처리를 완료하지 못했습니다.");
        }
        ClearParsedData();
        return false;
    }

    const int32 ParsedRowCount = ParsedRows.Num();
    const int32 ParsedColumnCount = Headers.Num();
    OutResult = FString::Printf(
        TEXT("파싱 완료 — %d 행, %d 컬럼"),
        ParsedRowCount,
        ParsedColumnCount);
    
    ClearParsedData();
    
    return true;
}

void UGoogleSheetParserBase::ClearParsedData()
{
    ParsedRows.Empty();
    Headers.Empty();
}

bool UGoogleSheetParserBase::GetRowAt(
    int32 Index,
    TMap<FString, FString>& OutRow) const
{
    if (!ParsedRows.IsValidIndex(Index)) return false;
    OutRow = ParsedRows[Index];
    return true;
}
