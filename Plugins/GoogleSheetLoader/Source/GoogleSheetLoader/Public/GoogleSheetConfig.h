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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Google Sheet|Config",
		meta=(DisplayName="Google Sheet URL",
			ToolTip="사용할 시트 탭을 연 상태에서 Google Sheets URL을 붙여 넣으세요. URL의 gid를 자동으로 사용합니다."))
	FString SheetURL;

	// 시작 범위의 첫 행은 헤더로 사용합니다. (예: A1)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Google Sheet|Config")
	FString RangeFrom = TEXT("A1");

	// 끝 범위 (예: C100)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Google Sheet|Config")
	FString RangeTo = TEXT("Z100");

	// 데이터 파서 지정
	UPROPERTY(EditAnywhere, Instanced, Category="Google Sheet|Config")
	UGoogleSheetParserBase* DataParser;

	// 파싱 완료 후 생성된 에셋들을 즉시 저장할지 여부
	UPROPERTY(EditAnywhere, Category="Google Sheet|Config")
	bool bAutoSaveOnComplete = false;

	// TSV를 변환한 JSON을 Saved/GoogleSheetLoader에 저장합니다.
	UPROPERTY(EditAnywhere, Category="Google Sheet|Config")
	bool bSaveNormalizedJson = false;

	// ── 상태 (에디터 전용, 저장 안 함) ────────────
#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Transient, Category="Google Sheet|Status")
	EFetchStatus FetchStatus = EFetchStatus::None;

	UPROPERTY(VisibleAnywhere, Transient, Category="Google Sheet|Status")
	FString LastMessage;

	UPROPERTY(VisibleAnywhere, Transient, Category="Google Sheet|Status")
	FString LastFetchTime;

	UPROPERTY(VisibleAnywhere, Transient, Category="Google Sheet|Status")
	FString LastNormalizedJsonPath;
#endif

public:
#if WITH_EDITOR
	// URL에서 Spreadsheet ID 파싱
	FString GetSpreadsheetID() const;

	// 범위 문자열 조합 반환 (예: A1:Z100)
	FString GetRangeString() const;

	// URL에서 시트 gid를 찾아 반환
	FString GetSheetGid() const;

	// 실제 페치 실행
	void Fetch();
#endif
};
