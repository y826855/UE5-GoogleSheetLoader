#pragma once
#include "CoreMinimal.h"
#include "GoogleSheetParserBase.h"
#include "Engine/DataAsset.h"
#include "GoogleSheetConfig.generated.h"

// 페치 결과 상태
UENUM()
enum class EFetchStatus : uint8
{
	None      UMETA(DisplayName = "대기중"),
	Success   UMETA(DisplayName = "성공"),
	Failed    UMETA(DisplayName = "실패"),
	Loading   UMETA(DisplayName = "로딩중"),
};

UCLASS(BlueprintType)
class GOOGLESHEETLOADER_API UGoogleSheetConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	// ── 시트 설정 ──────────────────────────────
    
	// 구글 시트 전체 URL 또는 Spreadsheet ID
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Google Sheet|Config")
	FString SheetURL;

	// 시트 페이지 이름 (예: Sheet1, 캐릭터데이터)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Google Sheet|Config")
	FString PageName;

	// 시작 범위 (예: A2)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Google Sheet|Config")
	FString RangeFrom = TEXT("A1");

	// 끝 범위 (예: C100)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Google Sheet|Config")
	FString RangeTo = TEXT("Z100");

	//데이터 파서 지정
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Google Sheet|Config")
	UPROPERTY(EditAnywhere, Instanced, Category="Google Sheet|Config")
	UGoogleSheetParserBase* DataParser;

	// ── 상태 (에디터 전용, 저장 안 함) ────────────
#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Transient, Category="Google Sheet|Status")
	EFetchStatus FetchStatus = EFetchStatus::None;

	UPROPERTY(VisibleAnywhere, Transient, Category="Google Sheet|Status")
	FString LastMessage;

	UPROPERTY(VisibleAnywhere, Transient, Category="Google Sheet|Status")
	FString LastFetchTime;
#endif

public:
#if WITH_EDITOR
	// URL에서 Spreadsheet ID 파싱
	FString GetSpreadsheetID() const;

	// 범위 문자열 조합 반환 (예: Sheet1!A2:C100)
	FString GetRangeString() const;

	// 실제 페치 실행
	void Fetch();
#endif
};