# GoogleSheetLoader

GoogleSheetLoader는 공개 Google Sheet 데이터를 Unreal Engine 에디터에서 불러와 프로젝트의 `DataTable`, `DataAsset` 등 원하는 에셋으로 변환하기 위한 에디터 플러그인입니다.

반복적으로 CSV를 내려받고 복사하는 흐름 대신, `GoogleSheetConfig` 데이터 에셋에 시트 정보와 파서 클래스를 설정한 뒤 Details 패널이나 대시보드에서 한 번에 갱신할 수 있습니다.

## 주요 기능

- 공개 Google Sheet의 GViz 응답을 HTTP로 로드
- `UGoogleSheetParserBase` 기반 커스텀 파서 작성
- `GoogleSheetConfig` 데이터 에셋 Details 패널에서 개별 로드
- 에디터 툴바의 `Sheet Loader` 대시보드에서 여러 Config 조회 및 일괄 갱신
- 파싱 진행률 표시
- 로드 완료 후 Dirty 패키지 자동 저장 옵션
- 빈 값, `None`, 비정상 JSON 셀, 누락된 Parser/Table 설정에 대한 크래시 방어
- 에셋 경로 생성 및 기존 에셋 로드/생성을 돕는 `UPathDataLoadHelper`

## 요구 사항

- Unreal Engine 5.x
- 플러그인은 Editor 모듈로 동작합니다.
- Google Sheet는 링크 접근 또는 공개 접근이 가능해야 합니다.

## 설치

1. 프로젝트의 `Plugins` 폴더에 `GoogleSheetLoader` 폴더를 배치합니다.
2. `.uproject`에서 플러그인을 활성화합니다.
3. 에디터를 다시 열거나 프로젝트 파일을 재생성한 뒤 빌드합니다.

예시 구조:

```text
YourProject/
  Plugins/
    GoogleSheetLoader/
      GoogleSheetLoader.uplugin
      Source/
```

## 기본 사용법

### 1. Google Sheet 준비

시트의 첫 행은 컬럼 이름으로 사용합니다.

예시:

| ID | DisplayName | Description |
| --- | --- | --- |
| 1 | Potion | Restores health |
| 2 | Sword | Basic weapon |

시트는 플러그인이 접근할 수 있도록 공개 또는 링크 접근 가능 상태여야 합니다.

### 2. 파서 클래스 만들기

`UGoogleSheetParserBase`를 상속한 클래스를 만들고 `OnParseComplete()`에서 변환 로직을 구현합니다.

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GoogleSheetParserBase.h"
#include "MyItemParser.generated.h"

UCLASS()
class MYPROJECTEDITOR_API UMyItemParser : public UGoogleSheetParserBase
{
    GENERATED_BODY()

protected:
    virtual void OnParseComplete() override;
};
```

```cpp
#include "MyItemParser.h"

void UMyItemParser::OnParseComplete()
{
    for (int32 RowIndex = 0; RowIndex < GetRowCount(); ++RowIndex)
    {
        TMap<FString, FString> Row;
        if (!GetRowAt(RowIndex, Row))
        {
            continue;
        }

        const FString ID = Row.FindRef(TEXT("ID")).TrimStartAndEnd();
        const FString DisplayName = Row.FindRef(TEXT("DisplayName"));
        const FString Description = Row.FindRef(TEXT("Description"));

        // 여기에서 DataTable 행 추가, DataAsset 생성, 기존 에셋 갱신 등을 처리합니다.
    }
}
```

이 프로젝트에는 예제 파서로 `UItemDataParser`가 포함되어 있습니다.

### 3. GoogleSheetConfig 에셋 생성

콘텐츠 브라우저에서 `GoogleSheetConfig` 데이터 에셋을 생성한 뒤 아래 값을 설정합니다.

| 필드 | 설명 |
| --- | --- |
| `SheetURL` | Google Sheet 전체 URL 또는 Spreadsheet ID |
| `PageName` | 시트 탭 이름 |
| `RangeFrom` | 읽기 시작 셀. 예: `A1` |
| `RangeTo` | 읽기 끝 셀. 예: `Z100` |
| `DataParser` | `UGoogleSheetParserBase`를 상속한 파서 인스턴스 |
| `bAutoSaveOnComplete` | 로드 완료 후 Dirty 패키지를 자동 저장할지 여부 |

`SheetURL`, `PageName`, `RangeFrom`, `RangeTo`, `DataParser`가 비어 있거나 `None`이면 로드를 중단하고 실패 메시지를 표시합니다.

### 4. 개별 로드

`GoogleSheetConfig` 에셋을 열고 Details 패널의 `Load Google Sheet Data` 버튼을 누릅니다.

로드 결과는 상태 영역에 표시됩니다.

- `Success`: 로드 및 파싱 성공
- `Failed`: 설정값, 네트워크, 파싱 문제 등으로 실패
- `Loading`: 요청 진행 중

### 5. 대시보드에서 일괄 갱신

에디터 툴바의 `Sheet Loader` 버튼을 누르면 `Google Sheet Dashboard` 탭이 열립니다.

대시보드 기능:

- 프로젝트 내 `GoogleSheetConfig` 목록 조회
- 개별 Config `Update`
- 전체 Config `Update All Sheets`
- Config 에셋 위치 찾기
- 목록 새로고침

## 파서에서 사용할 수 있는 주요 API

### `UGoogleSheetParserBase`

| 함수 | 설명 |
| --- | --- |
| `Parse(RawResponse, OutResult)` | Google Sheet 응답을 파싱합니다. 일반적으로 직접 호출하지 않고 Config가 호출합니다. |
| `GetRowAt(Index, OutRow)` | 파싱된 특정 행을 가져옵니다. |
| `GetRowCount()` | 파싱된 행 수를 반환합니다. |
| `GetHeaders()` | 컬럼 헤더 목록을 반환합니다. |

보조 함수:

- `ParseToColor`
- `ParseToVector2`
- `ParseToVector`
- `DoesAssetExist`

### `UPathDataLoadHelper`

| 함수 | 설명 |
| --- | --- |
| `MakeAssetReferencePath(FolderPath, AssetName)` | `/Game/Folder/Asset.Asset` 형태의 참조 경로를 만듭니다. |
| `MakePackagePath(FolderPath, AssetName)` | `/Game/Folder/Asset` 형태의 패키지 경로를 만듭니다. |
| `LoadResource<T>(Path)` | Soft Object Ptr 형태로 리소스를 참조합니다. |
| `GetAssetsByPathFilter(FolderPath, NameFilter)` | 특정 폴더에서 이름 패턴에 맞는 에셋을 찾습니다. |
| `GetOrCreateAsset<T>(FolderPath, NameFormat)` | 기존 에셋을 로드하거나 없으면 새로 생성합니다. |

`None`, 빈 문자열, 잘못된 패키지 경로는 크래시 대신 로그를 남기고 실패 처리합니다.

## 예제: ItemDataParser 흐름

현재 샘플 파서는 Google Sheet의 `ID`, `DisplayName`, `Description` 컬럼을 읽어 `FItemDataStructure`로 변환합니다.

처리 흐름:

1. `TargetTable`이 설정되어 있는지 확인합니다.
2. 각 행에서 `ID`를 읽습니다.
3. `ID`가 비어 있거나 `None`이면 해당 행을 건너뜁니다.
4. `AssetFolderPath`와 `AssetNameFormat`으로 `UItemDataAsset`을 찾거나 생성합니다.
5. `SpriteFolderPath`와 `SpriteFileFormat`으로 아이콘 참조를 설정합니다.
6. `TargetTable`에 행을 추가합니다.

## 주의 사항

- Google Sheet가 비공개이면 HTTP 요청이 실패할 수 있습니다.
- 컬럼 이름은 파서 코드에서 사용하는 이름과 정확히 맞아야 합니다.
- `RangeFrom`은 보통 헤더가 포함된 `A1`부터 설정합니다.
- 에셋을 생성하려면 `AssetFolderPath`가 `/Game/...` 형태의 유효한 Long Package Name이어야 합니다.
- 파서 내부에서 `None`이나 빈 값이 들어올 수 있다고 보고 방어 코드를 작성하는 것이 좋습니다.

## 문제 해결

### `DataParser is empty.`

`GoogleSheetConfig`의 `DataParser`에 파서 인스턴스가 설정되어 있지 않습니다.

### `SheetURL is empty.`

`SheetURL`이 비어 있거나 `None`입니다. Google Sheet URL 또는 Spreadsheet ID를 입력하세요.

### `PageName is empty.`

시트 탭 이름이 비어 있거나 `None`입니다.

### `RangeFrom or RangeTo is empty.`

읽을 범위가 비어 있거나 `None`입니다.

### `Invalid package path`

에셋 생성 경로가 `/Game/...` 형태가 아니거나 잘못된 값입니다.

## 라이선스

프로젝트의 `LICENSE` 파일을 참고하세요.
