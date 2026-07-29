#pragma once

#include "CoreMinimal.h"
#include "GoogleSheetTableData.h"

class FGoogleSheetTsvReader
{
public:
	static bool Parse(
		const FString& Tsv,
		const FString& RangeFrom,
		const FString& RangeTo,
		FGoogleSheetTableData& OutData,
		FString& OutError);
};
