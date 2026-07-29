#pragma once

#include "CoreMinimal.h"

// TSV의 모든 셀을 문자열로 보존하는 내부 데이터 형식입니다.
struct FGoogleSheetTableData
{
	TArray<FString> Headers;
	TArray<TArray<FString>> Rows;

	bool ToJson(FString& OutJson, FString& OutError) const;
	static bool FromJson(const FString& Json, FGoogleSheetTableData& OutData, FString& OutError);
};
