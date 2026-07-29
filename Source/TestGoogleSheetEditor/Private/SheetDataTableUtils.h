#pragma once

#include "CoreMinimal.h"
#include "DataTableEditorUtils.h"
#include "Engine/DataTable.h"

namespace SheetDataTableUtils
{
	// DataTable이 예상 RowStruct를 사용하는지 확인합니다.
	inline bool ValidateTargetTable(
		const UDataTable* TargetTable,
		const UScriptStruct* ExpectedRowStruct)
	{
		return IsValid(TargetTable)
			&& IsValid(ExpectedRowStruct)
			&& TargetTable->GetRowStruct() == ExpectedRowStruct;
	}

	// 잘못 지정된 DataTable 정보를 로그로 출력합니다.
	inline void LogTargetTableError(
		const TCHAR* ParserName,
		const UDataTable* TargetTable,
		const UScriptStruct* ExpectedRowStruct)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[Sheet][%s] Invalid table. Table=%s ExpectedRow=%s ActualRow=%s"),
			ParserName,
			*GetNameSafe(TargetTable),
			*GetNameSafe(ExpectedRowStruct),
			TargetTable
				? *GetNameSafe(TargetTable->GetRowStruct())
				: TEXT("None"));
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

		bool IsActive() const
		{
			return ::IsValid(TargetTable);
		}

		FScopedDataTableEditNotification(
			const FScopedDataTableEditNotification&) = delete;
		FScopedDataTableEditNotification& operator=(
			const FScopedDataTableEditNotification&) = delete;

	private:
		UDataTable* TargetTable = nullptr;
	};
}
