#pragma once

#include "CoreMinimal.h"
#include "DataTableEditorUtils.h"
#include "Engine/DataTable.h"

// Editor 모듈의 시트 파서에서 공통으로 사용하는 DataTable 유틸입니다.
namespace SheetDataTableUtils
{
	// DataTable과 RowStruct를 확인하고 실패 사유를 반환합니다.
	inline bool ValidateTargetTable(
		const TCHAR* ParserName,
		const UDataTable* TargetTable,
		const UScriptStruct* ExpectedRowStruct,
		FString& OutError)
	{
		if (IsValid(TargetTable)
			&& IsValid(ExpectedRowStruct)
			&& TargetTable->GetRowStruct() == ExpectedRowStruct)
		{
			OutError.Reset();
			return true;
		}

		OutError = FString::Printf(
			TEXT("잘못된 DataTable입니다. Table=%s ExpectedRow=%s ActualRow=%s"),
			*GetNameSafe(TargetTable),
			*GetNameSafe(ExpectedRowStruct),
			TargetTable
				? *GetNameSafe(TargetTable->GetRowStruct())
				: TEXT("None"));
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[Sheet][%s] %s"),
			ParserName,
			*OutError);
		return false;
	}

	// 파서가 받은 헤더를 로그로 출력합니다.
	inline void LogHeaders(
		const TCHAR* ParserName,
		const TArray<FString>& Headers)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[Sheet][%s] Headers=%s"),
			ParserName,
			*FString::Join(Headers, TEXT(", ")));
	}

	// DataTable 수정 알림과 Dirty 처리를 Scope 종료까지 보장합니다.
	class FScopedDataTableEditNotification
	{
	public:
		// bClearTable이 true면 기존 행을 모두 제거합니다.
		explicit FScopedDataTableEditNotification(
			UDataTable* InTargetTable,
			const bool bClearTable = true)
			: TargetTable(InTargetTable)
		{
			if (!::IsValid(TargetTable))
			{
				TargetTable = nullptr;
				return;
			}

			FDataTableEditorUtils::BroadcastPreChange(
				TargetTable,
				FDataTableEditorUtils::EDataTableChangeInfo::RowList);
			TargetTable->Modify();

			if (bClearTable)
			{
				TargetTable->EmptyTable();
			}
		}

		~FScopedDataTableEditNotification()
		{
			if (!::IsValid(TargetTable))
			{
				return;
			}

			FDataTableEditorUtils::BroadcastPostChange(
				TargetTable,
				FDataTableEditorUtils::EDataTableChangeInfo::RowList);
			(void)TargetTable->MarkPackageDirty();
		}

		FScopedDataTableEditNotification(
			const FScopedDataTableEditNotification&) = delete;
		FScopedDataTableEditNotification& operator=(
			const FScopedDataTableEditNotification&) = delete;

	private:
		UDataTable* TargetTable = nullptr;
	};
}
