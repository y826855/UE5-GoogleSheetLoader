# Google Sheet Loader 작업 정리

- 작업일: 2026-07-30
- 대상: `Plugins/GoogleSheetLoader`, `Source/TestGoogleSheetEditor`
- 목적: 실제 프로젝트 사용 중 확인된 데이터 손실, 파서 작성 편의성, 대시보드 구조 문제를 개선한다.

## 1. 주요 결정

### 첫 행은 헤더로 고정

- 지정 범위의 첫 번째 행을 헤더로 사용한다.
- 헤더는 비어 있거나 중복될 수 없다.
- 파서는 헤더 이름으로 각 셀에 접근한다.
- 자유로운 헤더 행 지정 기능은 추가하지 않고 문서에서 규칙을 안내한다.

### GViz 대신 TSV 사용

GViz는 한 열을 하나의 데이터 타입으로 추론한다. 숫자와 문자열이 섞인 열을 숫자형으로 판단하면 문자열 셀이 `null` 또는 빈 값으로 반환된다.

실제 테스트 결과:

- GViz JSON: 문자열 셀이 `null`로 손실됨
- GViz CSV: 문자열 셀이 빈 문자열로 손실됨
- TSV export: 숫자와 한글 문자열이 모두 보존됨

따라서 데이터 다운로드는 TSV export를 사용한다.

```text
https://docs.google.com/spreadsheets/d/{SpreadsheetId}/export?format=tsv&gid={Gid}
```

### 시트 선택은 URL의 GID 사용

- 사용자는 원하는 시트 탭을 연 상태에서 Google Sheets URL을 붙여 넣는다.
- 플러그인이 URL의 `gid=` 값을 자동으로 추출한다.
- URL에 GID가 없으면 첫 번째 탭인 `0`을 사용한다.
- TSV에서는 시트 이름이 필요하지 않으므로 `PageName`과 수동 `SheetGid` 설정을 제거했다.

### Normalized JSON은 중간 포맷

처리 흐름:

```text
Google Sheets TSV
→ 문자열 테이블
→ Normalized JSON
→ 사용자 파서
→ DataTable / DataAsset
```

- 모든 JSON 셀은 문자열로 저장한다.
- 파서에서 필요한 타입으로 변환한다.
- JSON 파일 저장 기본값은 `false`다.
- 파일 저장을 켜면 `Saved/GoogleSheetLoader`에 UTF-8 JSON으로 저장한다.
- 파일 저장을 꺼도 메모리에서 JSON을 생성해 파서에 전달한다.

## 2. 최종 Config 구성

`UGoogleSheetConfig`의 주요 설정:

| 설정 | 설명 |
|---|---|
| `SheetURL` | 전체 Google Sheets URL 또는 Spreadsheet ID |
| `RangeFrom` | 시작 셀이며 해당 행을 헤더로 사용 |
| `RangeTo` | 종료 셀 |
| `DataParser` | 사용자 프로젝트에서 구현한 파서 |
| `bAutoSaveOnComplete` | 처리 후 Dirty Package 자동 저장 여부 |
| `bSaveNormalizedJson` | Normalized JSON 파일 저장 여부, 기본값 `false` |

대시보드와 Config 상세 화면에서는 자동으로 인식한 Spreadsheet ID, GID, 범위, 로딩 상태와 결과 메시지를 확인할 수 있다.

## 3. 파서 구조

사용자 파서는 프로젝트의 Editor 모듈에 작성하고 `UGoogleSheetParserBase`를 상속한다.

```cpp
UCLASS()
class MYPROJECTEDITOR_API UMySheetParser : public UGoogleSheetParserBase
{
    GENERATED_BODY()

protected:
    virtual bool OnParseComplete(FString& OutError) override;
};
```

처리 성공:

```cpp
return true;
```

처리 실패:

```cpp
OutError = TEXT("실패 원인");
return false;
```

자식 파서의 실패 사유는 Config와 대시보드의 `Failed` 상태로 전달된다.

파싱 중 사용할 수 있는 API:

- `GetHeaders()`: 헤더 목록
- `GetRowCount()`: 데이터 행 수
- `GetRowAt()`: 특정 행의 `TMap<FString, FString>`
- `OnRowParsed()`: 행 단위 처리가 필요한 경우 재정의
- `OnParseComplete()`: 전체 행을 이용한 최종 처리

파싱 데이터는 콜백 처리 중에만 유지하고 완료 후 정리한다.

## 4. 파서 유틸 분리

테스트 프로젝트의 범용 파서 유틸을 역할별로 나눴다.

### `SheetParserUtils`

- 문자열 공백 및 미설정 값 처리
- `int32`, `float`, `bool`, Enum 변환
- 문자열, 숫자, 이름 배열 변환
- Color, Vector, Rotator, Transform 변환
- DataAsset 찾기 및 생성
- Object와 Blueprint Class의 Soft Reference 생성

### `SheetValidation`

- 필수 헤더 검증
- 필수 셀과 선택 셀 조회
- 행별 Warning/Error 기록
- 성공, 경고, 오류 개수 집계

### `SheetDataTableUtils`

- 대상 DataTable과 RowStruct 검증
- 헤더 및 DataTable 오류 로그
- DataTable 변경 알림과 Dirty 처리를 보장하는 Scope 제공

## 5. TSV 및 JSON 검증

TSV Reader:

- 탭, 줄바꿈, 따옴표와 이스케이프된 따옴표 처리
- A1 형식 범위 파싱
- UTF-8 BOM 제거
- 범위 우측의 완전히 빈 열 제거
- 빈 헤더와 중복 헤더 거부
- 데이터가 있지만 헤더가 없는 열 거부
- 완전히 빈 데이터 행 제외

Normalized JSON:

```json
{
  "headers": ["ID", "DisplayName"],
  "rows": [
    ["1", "Apple"]
  ]
}
```

- 헤더와 셀은 문자열만 허용한다.
- 부족한 셀은 빈 문자열로 채운다.
- 헤더보다 셀이 많은 행은 오류 처리한다.

## 6. 대시보드 및 요청 안정성

- Config 목록 갱신
- 전체 시트 업데이트
- 개별 시트 업데이트
- Content Browser에서 Config 찾기
- 로딩 중인 Config는 중복 요청할 수 없음
- 하나라도 로딩 중이면 전체 업데이트 버튼 비활성화
- HTTP 요청 시작 자체가 실패하면 즉시 `Failed` 처리
- 사용자 파싱 취소를 성공으로 처리하지 않음

## 7. Editor Module 선택 기능 제거

자동 모듈 생성기를 제거한 뒤 모듈 선택 및 검증 기능도 최종적으로 제거했다.

제거한 기능:

- 프로젝트 Editor 모듈 검색
- 모듈 선택 UI와 설정 탭
- 선택 모듈 이름 저장
- `Build.cs` 의존성 검사
- 모듈 준비 상태에 따른 대시보드 및 Fetch 차단

파서가 들어 있는 실제 프로젝트 Editor 모듈은 계속 필요하다. 다만 플러그인이 특정 Editor 모듈을 별도로 선택하거나 기억하지 않는다.

최종 실행 흐름:

```text
대시보드 열기
→ Config 선택
→ URL, 범위, DataParser 검증
→ TSV 다운로드
→ JSON 정규화
→ 사용자 파서 실행
```

## 8. 폴더 구조

```text
GoogleSheetLoader/
├─ Public/
│  ├─ GoogleSheetConfig.h
│  ├─ GoogleSheetLoader.h
│  ├─ GoogleSheetParserBase.h
│  └─ PathDataLoadHelper.h
└─ Private/
   ├─ Asset/
   │  └─ PathDataLoadHelper.cpp
   ├─ Config/
   │  ├─ GoogleSheetConfig.cpp
   │  ├─ GoogleSheetConfigCustomization.cpp
   │  └─ GoogleSheetConfigCustomization.h
   ├─ Dashboard/
   │  ├─ SGoogleSheetDashboard.cpp
   │  └─ SGoogleSheetDashboard.h
   ├─ Parser/
   │  ├─ GoogleSheetParserBase.cpp
   │  ├─ GoogleSheetTableData.cpp
   │  ├─ GoogleSheetTableData.h
   │  ├─ GoogleSheetTsvReader.cpp
   │  └─ GoogleSheetTsvReader.h
   └─ GoogleSheetLoader.cpp
```

`GoogleSheetTableData`와 TSV Reader는 플러그인 내부 구현이므로 `Private/Parser`에 둔다. 사용자 프로젝트에서 상속하거나 호출해야 하는 Config, Parser Base, Asset Helper만 `Public`에 둔다.

## 9. 빌드 관련 주의

- Slate의 `SListView`와 `STableRow`는 내부적으로 `EKeys`를 사용하므로 `InputCore` 의존성이 필요하다.
- `InputCore`를 제거하면 `EKeys::LeftMouseButton`, `RightMouseButton`, `SpaceBar` 등의 `LNK2019` 오류가 발생한다.
- 모듈 선택 기능 제거 후 `Projects` 의존성은 필요하지 않아 제거했다.

## 10. 확인할 항목

이번 작업에서는 빌드를 실행하지 않았다. 다음 항목은 Unreal Editor에서 확인한다.

1. 원하는 시트 탭 URL을 붙여 넣었을 때 GID가 올바르게 표시되는지 확인
2. 숫자와 한글 문자열이 섞인 열이 모두 보존되는지 확인
3. 필수 헤더 및 셀 오류가 대시보드에서 `Failed`로 표시되는지 확인
4. 파싱 취소 시 성공으로 표시되지 않는지 확인
5. 개별 또는 전체 업데이트 중 중복 실행이 막히는지 확인
6. `bSaveNormalizedJson=false`일 때 JSON 파일이 생성되지 않는지 확인
7. `bSaveNormalizedJson=true`일 때 `Saved/GoogleSheetLoader`에 UTF-8 JSON이 생성되는지 확인

## 11. 기존 Config 에셋 주의

코드 기본값을 변경해도 기존 Config 에셋에 저장된 값은 유지될 수 있다.

- 기존 에셋의 `bSaveNormalizedJson`이 `true`라면 에디터에서 직접 끈다.
- 제거된 `PageName`, `SheetGid`, Editor Module 설정은 더 이상 사용하지 않는다.
