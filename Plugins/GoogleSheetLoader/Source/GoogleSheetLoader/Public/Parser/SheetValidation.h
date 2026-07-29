#pragma once

#include "CoreMinimal.h"
#include "SheetParserUtils.h"

// Editor 모듈의 시트 파서에서 공통으로 사용하는 검증 유틸입니다.
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

		void AddSuccess()
		{
			++SuccessCount;
		}
	};

	// 필수 설정의 이름과 값을 함께 전달합니다.
	struct FRequiredSetting
	{
		FString Name;
		FString Value;
	};

	// 비어 있는 필수 설정을 찾아 오류를 반환합니다.
	inline bool ValidateRequiredSettings(
		const TCHAR* ParserName,
		const TArray<FRequiredSetting>& Settings,
		FString& OutError)
	{
		using namespace SheetParserUtils;

		for (const FRequiredSetting& Setting : Settings)
		{
			if (!IsUnsetValue(Setting.Value))
			{
				continue;
			}

			OutError = FString::Printf(
				TEXT("필수 설정이 비어 있습니다: %s"),
				*Setting.Name);
			UE_LOG(
				LogTemp,
				Error,
				TEXT("[Sheet][%s] %s"),
				ParserName,
				*OutError);
			return false;
		}

		OutError.Reset();
		return true;
	}

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

	// 필수 셀을 정수로 변환하고 실패하면 오류를 기록합니다.
	inline bool GetRequiredIntCell(
		const TMap<FString, FString>& RowData,
		const FString& ColumnName,
		int32& OutValue,
		FParseReport* Report = nullptr,
		const int32 RowIndex = INDEX_NONE,
		const FName RowName = NAME_None)
	{
		FString SourceValue;
		OutValue = 0;
		if (!GetRequiredCell(
			RowData,
			ColumnName,
			SourceValue,
			Report,
			RowIndex,
			RowName))
		{
			return false;
		}

		if (LexTryParseString(OutValue, *SourceValue))
		{
			return true;
		}

		if (Report)
		{
			Report->AddIssue(
				EParseIssueSeverity::Error,
				TEXT("정수로 변환할 수 없습니다."),
				RowIndex,
				RowName,
				ColumnName,
				SourceValue);
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

	// 한 행의 값 조회와 오류 기록에 필요한 문맥을 보관합니다.
	class FSheetRowReader
	{
	public:
		FSheetRowReader(
			const TMap<FString, FString>& InRowData,
			const int32 InRowIndex,
			FParseReport& InReport)
			: RowData(InRowData)
			, RowIndex(InRowIndex)
			, Report(InReport)
		{
		}

		FString GetRequiredString(const FString& ColumnName)
		{
			FString Value;
			if (!GetRequiredCell(
				RowData,
				ColumnName,
				Value,
				&Report,
				RowIndex,
				RowName))
			{
				bIsValid = false;
			}
			return Value;
		}

		int32 GetRequiredInt(const FString& ColumnName)
		{
			int32 Value = 0;
			if (!GetRequiredIntCell(
				RowData,
				ColumnName,
				Value,
				&Report,
				RowIndex,
				RowName))
			{
				bIsValid = false;
			}
			return Value;
		}

		FName GetRequiredName(const FString& ColumnName)
		{
			using namespace SheetParserUtils;

			const FString SourceValue = GetRequiredString(ColumnName);
			if (SourceValue.IsEmpty())
			{
				return NAME_None;
			}

			const FName Value = ParseNameValue(SourceValue);
			if (!Value.IsNone())
			{
				RowName = Value;
				return Value;
			}

			bIsValid = false;
			Report.AddIssue(
				EParseIssueSeverity::Error,
				TEXT("이름으로 변환할 수 없습니다."),
				RowIndex,
				NAME_None,
				ColumnName,
				SourceValue);
			return NAME_None;
		}

		FString Get(
			const FString& ColumnName,
			const FString& DefaultValue = FString()) const
		{
			return GetCellOrDefault(
				RowData,
				ColumnName,
				DefaultValue);
		}

		bool IsValid() const
		{
			return bIsValid;
		}

		void AddWarning(
			const FString& Message,
			const FString& ColumnName = FString(),
			const FString& SourceValue = FString())
		{
			Report.AddIssue(
				EParseIssueSeverity::Warning,
				Message,
				RowIndex,
				RowName,
				ColumnName,
				SourceValue);
		}

		// 선택 에셋 할당
		template <typename TObjectType>
		void AssignOptionalAsset(
			TSoftObjectPtr<TObjectType>& Target,
			const FString& FolderPath,
			const FString& NameFormat,
			const FString& WarningContext)
		{
			using namespace SheetParserUtils;

			if (IsUnsetValue(FolderPath)
				|| IsUnsetValue(NameFormat))
			{
				return;
			}

			const TSoftObjectPtr<TObjectType> Asset =
				FindObject<TObjectType>(
					FolderPath,
					NameFormat,
					RowName);
			if (Asset.IsNull())
			{
				AddWarning(
					TEXT("에셋을 찾을 수 없습니다."),
					WarningContext,
					MakeGeneratedAssetName(NameFormat, RowName));
				return;
			}

			Target = Asset;
		}

	private:
		const TMap<FString, FString>& RowData;
		int32 RowIndex = INDEX_NONE;
		FParseReport& Report;
		FName RowName = NAME_None;
		bool bIsValid = true;
	};

	// 문제 목록과 요약을 출력하고 최종 성공 여부를 반환합니다.
	inline bool FinalizeParseReport(
		const TCHAR* ParserName,
		const FParseReport& Report,
		FString& OutError)
	{
		for (const FParseIssue& Issue : Report.Issues)
		{
			if (Issue.Severity == EParseIssueSeverity::Error)
			{
				UE_LOG(
					LogTemp,
					Error,
					TEXT("[Sheet][%s] Row=%d Column=%s Value='%s' %s"),
					ParserName,
					Issue.RowIndex,
					*Issue.ColumnName,
					*Issue.SourceValue,
					*Issue.Message);
			}
			else
			{
				UE_LOG(
					LogTemp,
					Warning,
					TEXT("[Sheet][%s] Row=%d Column=%s Value='%s' %s"),
					ParserName,
					Issue.RowIndex,
					*Issue.ColumnName,
					*Issue.SourceValue,
					*Issue.Message);
			}
		}

		UE_LOG(
			LogTemp,
			Log,
			TEXT("[Sheet][%s] Success=%d Warning=%d Error=%d"),
			ParserName,
			Report.SuccessCount,
			Report.WarningCount,
			Report.ErrorCount);

		if (Report.HasErrors())
		{
			OutError = FString::Printf(
				TEXT("%d개 행 처리 오류가 발생했습니다."),
				Report.ErrorCount);
			return false;
		}

		OutError.Reset();
		return true;
	}
}
