#include "GoogleSheetTableData.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

bool FGoogleSheetTableData::ToJson(FString& OutJson, FString& OutError) const
{
	OutJson.Reset();
	OutError.Reset();

	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();

	TArray<TSharedPtr<FJsonValue>> JsonHeaders;
	JsonHeaders.Reserve(Headers.Num());
	for (const FString& Header : Headers)
	{
		JsonHeaders.Add(MakeShared<FJsonValueString>(Header));
	}
	Root->SetArrayField(TEXT("headers"), JsonHeaders);

	TArray<TSharedPtr<FJsonValue>> JsonRows;
	JsonRows.Reserve(Rows.Num());
	for (const TArray<FString>& Row : Rows)
	{
		TArray<TSharedPtr<FJsonValue>> JsonCells;
		JsonCells.Reserve(Row.Num());
		for (const FString& Cell : Row)
		{
			JsonCells.Add(MakeShared<FJsonValueString>(Cell));
		}
		JsonRows.Add(MakeShared<FJsonValueArray>(JsonCells));
	}
	Root->SetArrayField(TEXT("rows"), JsonRows);

	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJson);
	if (!FJsonSerializer::Serialize(Root, Writer))
	{
		OutError = TEXT("정규화 JSON 생성에 실패했습니다.");
		return false;
	}

	return true;
}

bool FGoogleSheetTableData::FromJson(
	const FString& Json,
	FGoogleSheetTableData& OutData,
	FString& OutError)
{
	OutData = FGoogleSheetTableData();
	OutError.Reset();

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		OutError = TEXT("정규화 JSON 파싱에 실패했습니다.");
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* JsonHeaders = nullptr;
	if (!Root->TryGetArrayField(TEXT("headers"), JsonHeaders))
	{
		OutError = TEXT("정규화 JSON에 headers가 없습니다.");
		return false;
	}

	TSet<FString> UniqueHeaders;
	for (const TSharedPtr<FJsonValue>& HeaderValue : *JsonHeaders)
	{
		FString Header;
		if (!HeaderValue.IsValid() || !HeaderValue->TryGetString(Header))
		{
			OutError = TEXT("headers에는 문자열만 사용할 수 있습니다.");
			return false;
		}

		Header = Header.TrimStartAndEnd();
		if (Header.IsEmpty())
		{
			OutError = TEXT("빈 헤더는 사용할 수 없습니다.");
			return false;
		}
		if (UniqueHeaders.Contains(Header))
		{
			OutError = FString::Printf(TEXT("중복 헤더가 있습니다: %s"), *Header);
			return false;
		}

		UniqueHeaders.Add(Header);
		OutData.Headers.Add(Header);
	}

	if (OutData.Headers.IsEmpty())
	{
		OutError = TEXT("헤더가 없습니다.");
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* JsonRows = nullptr;
	if (!Root->TryGetArrayField(TEXT("rows"), JsonRows))
	{
		OutError = TEXT("정규화 JSON에 rows가 없습니다.");
		return false;
	}

	for (int32 RowIndex = 0; RowIndex < JsonRows->Num(); ++RowIndex)
	{
		const TArray<TSharedPtr<FJsonValue>>* JsonCells = nullptr;
		if (!(*JsonRows)[RowIndex].IsValid()
			|| !(*JsonRows)[RowIndex]->TryGetArray(JsonCells))
		{
			OutError = FString::Printf(TEXT("Row %d가 배열이 아닙니다."), RowIndex + 1);
			return false;
		}

		if (JsonCells->Num() > OutData.Headers.Num())
		{
			OutError = FString::Printf(
				TEXT("Row %d의 셀 수가 헤더 수보다 많습니다."),
				RowIndex + 1);
			return false;
		}

		TArray<FString>& Row = OutData.Rows.AddDefaulted_GetRef();
		Row.Reserve(OutData.Headers.Num());
		for (const TSharedPtr<FJsonValue>& CellValue : *JsonCells)
		{
			FString Cell;
			if (!CellValue.IsValid() || !CellValue->TryGetString(Cell))
			{
				OutError = FString::Printf(
					TEXT("Row %d에는 문자열 셀만 사용할 수 있습니다."),
					RowIndex + 1);
				return false;
			}
			Row.Add(Cell);
		}

		Row.SetNum(OutData.Headers.Num());
	}

	return true;
}
