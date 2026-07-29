#include "GoogleSheetTsvReader.h"

namespace
{
	bool IsEmptyRow(const TArray<FString>& Row)
	{
		for (const FString& Cell : Row)
		{
			if (!Cell.IsEmpty())
			{
				return false;
			}
		}
		return true;
	}

	bool ParseCellAddress(
		const FString& Address,
		int32& OutColumn,
		int32& OutRow)
	{
		const FString CleanAddress = Address.TrimStartAndEnd().Replace(TEXT("$"), TEXT(""));
		int32 LetterCount = 0;
		while (LetterCount < CleanAddress.Len()
			&& FChar::IsAlpha(CleanAddress[LetterCount]))
		{
			++LetterCount;
		}

		if (LetterCount == 0 || LetterCount == CleanAddress.Len())
		{
			return false;
		}

		OutColumn = 0;
		for (int32 Index = 0; Index < LetterCount; ++Index)
		{
			const TCHAR Letter = FChar::ToUpper(CleanAddress[Index]);
			if (Letter < TEXT('A') || Letter > TEXT('Z'))
			{
				return false;
			}
			OutColumn = OutColumn * 26 + (Letter - TEXT('A') + 1);
		}

		const FString RowText = CleanAddress.Mid(LetterCount);
		if (!RowText.IsNumeric())
		{
			return false;
		}

		OutRow = FCString::Atoi(*RowText);
		--OutColumn;
		--OutRow;
		return OutColumn >= 0 && OutRow >= 0;
	}

	bool ReadRows(
		const FString& Tsv,
		TArray<TArray<FString>>& OutRows,
		FString& OutError)
	{
		TArray<FString> CurrentRow;
		FString CurrentCell;
		bool bInQuotes = false;

		for (int32 Index = 0; Index < Tsv.Len(); ++Index)
		{
			const TCHAR Character = Tsv[Index];
			if (Character == TEXT('"'))
			{
				if (bInQuotes && Index + 1 < Tsv.Len() && Tsv[Index + 1] == TEXT('"'))
				{
					CurrentCell.AppendChar(TEXT('"'));
					++Index;
				}
				else
				{
					bInQuotes = !bInQuotes;
				}
				continue;
			}

			if (!bInQuotes && Character == TEXT('\t'))
			{
				CurrentRow.Add(MoveTemp(CurrentCell));
				CurrentCell.Reset();
				continue;
			}

			if (!bInQuotes && (Character == TEXT('\r') || Character == TEXT('\n')))
			{
				if (Character == TEXT('\r')
					&& Index + 1 < Tsv.Len()
					&& Tsv[Index + 1] == TEXT('\n'))
				{
					++Index;
				}

				CurrentRow.Add(MoveTemp(CurrentCell));
				CurrentCell.Reset();
				OutRows.Add(MoveTemp(CurrentRow));
				CurrentRow.Reset();
				continue;
			}

			CurrentCell.AppendChar(Character);
		}

		if (bInQuotes)
		{
			OutError = TEXT("TSV의 따옴표가 닫히지 않았습니다.");
			return false;
		}

		if (!CurrentCell.IsEmpty() || !CurrentRow.IsEmpty())
		{
			CurrentRow.Add(MoveTemp(CurrentCell));
			OutRows.Add(MoveTemp(CurrentRow));
		}

		while (!OutRows.IsEmpty() && IsEmptyRow(OutRows.Last()))
		{
			OutRows.Pop();
		}

		return true;
	}
}

bool FGoogleSheetTsvReader::Parse(
	const FString& Tsv,
	const FString& RangeFrom,
	const FString& RangeTo,
	FGoogleSheetTableData& OutData,
	FString& OutError)
{
	OutData = FGoogleSheetTableData();
	OutError.Reset();

	if (Tsv.TrimStartAndEnd().IsEmpty())
	{
		OutError = TEXT("TSV 응답이 비어 있습니다.");
		return false;
	}

	TArray<TArray<FString>> RawRows;
	if (!ReadRows(Tsv, RawRows, OutError) || RawRows.IsEmpty())
	{
		if (OutError.IsEmpty())
		{
			OutError = TEXT("TSV에서 행을 찾지 못했습니다.");
		}
		return false;
	}

	int32 StartColumn = 0;
	int32 StartRow = 0;
	int32 EndColumn = 0;
	int32 EndRow = 0;
	if (!ParseCellAddress(RangeFrom, StartColumn, StartRow)
		|| !ParseCellAddress(RangeTo, EndColumn, EndRow)
		|| EndColumn < StartColumn
		|| EndRow < StartRow)
	{
		OutError = FString::Printf(
			TEXT("잘못된 시트 범위입니다: %s:%s"),
			*RangeFrom,
			*RangeTo);
		return false;
	}

	if (!RawRows.IsValidIndex(StartRow))
	{
		OutError = TEXT("범위의 시작 행이 TSV 데이터 밖에 있습니다.");
		return false;
	}

	const int32 LastRow = FMath::Min(EndRow, RawRows.Num() - 1);
	TArray<TArray<FString>> CroppedRows;
	CroppedRows.Reserve(LastRow - StartRow + 1);
	for (int32 RowIndex = StartRow; RowIndex <= LastRow; ++RowIndex)
	{
		TArray<FString>& CroppedRow = CroppedRows.AddDefaulted_GetRef();
		CroppedRow.Reserve(EndColumn - StartColumn + 1);
		for (int32 ColumnIndex = StartColumn; ColumnIndex <= EndColumn; ++ColumnIndex)
		{
			CroppedRow.Add(
				RawRows[RowIndex].IsValidIndex(ColumnIndex)
					? RawRows[RowIndex][ColumnIndex]
					: FString());
		}
	}

	if (CroppedRows.IsEmpty())
	{
		OutError = TEXT("지정 범위에 데이터가 없습니다.");
		return false;
	}

	OutData.Headers = MoveTemp(CroppedRows[0]);
	if (!OutData.Headers.IsEmpty() && !OutData.Headers[0].IsEmpty()
		&& OutData.Headers[0][0] == 0xFEFF)
	{
		OutData.Headers[0].RemoveAt(0);
	}

	// 범위를 넓게 잡았을 때 우측의 완전히 빈 열은 자동으로 제외합니다.
	while (!OutData.Headers.IsEmpty()
		&& OutData.Headers.Last().TrimStartAndEnd().IsEmpty())
	{
		OutData.Headers.Pop();
	}
	if (OutData.Headers.IsEmpty())
	{
		OutError = TEXT("지정 범위의 첫 행에 헤더가 없습니다.");
		return false;
	}

	TSet<FString> UniqueHeaders;
	for (FString& Header : OutData.Headers)
	{
		Header = Header.TrimStartAndEnd();
		if (Header.IsEmpty())
		{
			OutError = TEXT("첫 행에는 빈 헤더를 사용할 수 없습니다.");
			return false;
		}
		if (UniqueHeaders.Contains(Header))
		{
			OutError = FString::Printf(TEXT("중복 헤더가 있습니다: %s"), *Header);
			return false;
		}
		UniqueHeaders.Add(Header);
	}

	for (int32 RowIndex = 1; RowIndex < CroppedRows.Num(); ++RowIndex)
	{
		TArray<FString>& Row = CroppedRows[RowIndex];
		for (int32 ColumnIndex = OutData.Headers.Num(); ColumnIndex < Row.Num(); ++ColumnIndex)
		{
			if (!Row[ColumnIndex].IsEmpty())
			{
				OutError = FString::Printf(
					TEXT("Row %d의 데이터 열에 헤더가 없습니다."),
					StartRow + RowIndex + 1);
				return false;
			}
		}

		Row.SetNum(OutData.Headers.Num());
		if (!IsEmptyRow(Row))
		{
			OutData.Rows.Add(MoveTemp(Row));
		}
	}

	return true;
}
