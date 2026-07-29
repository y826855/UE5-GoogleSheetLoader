#pragma once

#include "CoreMinimal.h"
#include "SheetParserUtils.h"

namespace SheetValidation
{
	enum class EParseIssueSeverity : uint8
	{
		Warning,
		Error
	};

	// 한 행에서 발생한 파싱 문제입니다.
	struct FParseIssue
	{
		EParseIssueSeverity Severity = EParseIssueSeverity::Error;
		int32 RowIndex = INDEX_NONE;
		FName RowName = NAME_None;
		FString ColumnName;
		FString SourceValue;
		FString Message;
	};

	// 전체 파싱 결과와 문제 목록을 보관합니다.
	struct FParseReport
	{
		int32 SuccessCount = 0;
		int32 WarningCount = 0;
		int32 ErrorCount = 0;
		TArray<FParseIssue> Issues;

		void AddIssue(
			const EParseIssueSeverity Severity,
			const FString& Message,
			const int32 RowIndex = INDEX_NONE,
			const FName RowName = NAME_None,
			const FString& ColumnName = FString(),
			const FString& SourceValue = FString())
		{
			FParseIssue& Issue = Issues.AddDefaulted_GetRef();
			Issue.Severity = Severity;
			Issue.RowIndex = RowIndex;
			Issue.RowName = RowName;
			Issue.ColumnName = ColumnName;
			Issue.SourceValue = SourceValue;
			Issue.Message = Message;

			if (Severity == EParseIssueSeverity::Error)
			{
				++ErrorCount;
			}
			else
			{
				++WarningCount;
			}
		}

		bool HasErrors() const
		{
			return ErrorCount > 0;
		}
	};

	// 필수 헤더가 모두 존재하는지 확인합니다.
	inline bool ValidateRequiredHeaders(
		const TArray<FString>& Headers,
		const TArray<FString>& RequiredHeaders,
		FParseReport* Report = nullptr)
	{
		bool bIsValid = true;
		for (const FString& RequiredHeader : RequiredHeaders)
		{
			if (Headers.Contains(RequiredHeader))
			{
				continue;
			}

			bIsValid = false;
			if (Report)
			{
				Report->AddIssue(
					EParseIssueSeverity::Error,
					FString::Printf(
						TEXT("필수 헤더가 없습니다: %s"),
						*RequiredHeader),
					INDEX_NONE,
					NAME_None,
					RequiredHeader);
			}
		}
		return bIsValid;
	}

	// 필수 셀을 가져오고 빈 값이면 오류를 기록합니다.
	inline bool GetRequiredCell(
		const TMap<FString, FString>& RowData,
		const FString& ColumnName,
		FString& OutValue,
		FParseReport* Report = nullptr,
		const int32 RowIndex = INDEX_NONE,
		const FName RowName = NAME_None)
	{
		using namespace SheetParserUtils;

		const FString* FoundValue = RowData.Find(ColumnName);
		if (FoundValue && !IsUnsetValue(*FoundValue))
		{
			OutValue = TrimCell(*FoundValue);
			return true;
		}

		OutValue.Reset();
		if (Report)
		{
			Report->AddIssue(
				EParseIssueSeverity::Error,
				FString::Printf(
					TEXT("필수 셀이 비어 있습니다: %s"),
					*ColumnName),
				RowIndex,
				RowName,
				ColumnName,
				FoundValue ? *FoundValue : FString());
		}
		return false;
	}

	// 선택 셀을 가져오고 빈 값이면 DefaultValue를 반환합니다.
	inline FString GetCellOrDefault(
		const TMap<FString, FString>& RowData,
		const FString& ColumnName,
		const FString& DefaultValue = FString())
	{
		using namespace SheetParserUtils;

		const FString* FoundValue = RowData.Find(ColumnName);
		return FoundValue && !IsUnsetValue(*FoundValue)
			? TrimCell(*FoundValue)
			: DefaultValue;
	}
}
