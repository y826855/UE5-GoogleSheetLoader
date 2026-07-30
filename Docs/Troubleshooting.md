# GoogleSheetLoader 문제 해결

문제가 발생하면 먼저 Config의 상태 메시지와 Unreal Editor의 Output Log를 확인하세요. `Save Normalized Json`을 활성화하면 `Saved/GoogleSheetLoader`에서 Parser에 전달된 데이터도 확인할 수 있습니다.

> 🖼️ **이미지 추가 위치**
>
> - 권장 이미지: Config의 실패 상태 메시지와 Output Log 위치
> - 파일 예시: `Images/troubleshooting-status-log.png`

## 빠른 점검표

- Google Sheet가 링크로 접근 가능한가?
- 가져올 탭을 연 상태에서 복사한 전체 URL인가?
- 범위의 첫 행에 Header가 포함되어 있는가?
- Header가 비어 있거나 중복되지 않았는가?
- 시트 Header와 Parser의 필수 Header 이름이 정확히 같은가?
- Config에 Parser와 Target DataTable이 지정되어 있는가?
- Target DataTable의 Row Structure가 Parser에서 기대하는 구조체와 같은가?
- Parser가 프로젝트의 Editor Module에 들어 있는가?

## 한글과 영어가 섞이면 깨지는 경우

현재 로더는 Google Sheet의 TSV 내보내기 응답을 받아 Unreal의 문자열로 처리합니다. 한글과 영어가 한 셀 또는 한 행에 섞여 있다는 사실 자체는 문제가 되지 않습니다.

문자가 깨진다면 다음 순서로 확인합니다.

1. Config에서 `Save Normalized Json`을 활성화합니다.
2. 다시 로드한 뒤 `Saved/GoogleSheetLoader`의 JSON을 UTF-8로 열어봅니다.
3. JSON부터 깨졌다면 HTTP 응답의 문자 인코딩 또는 원본 시트 데이터를 확인합니다.
4. JSON은 정상인데 DataTable만 깨졌다면 Parser의 문자열 변환 과정과 대상 프로퍼티 타입을 확인합니다.
5. C++ 소스에 한글 리터럴을 직접 작성했다면 해당 소스 파일도 UTF-8로 저장합니다.

응답이 실제 TSV가 아니라 로그인 페이지나 권한 안내 HTML인 경우에도 이상한 문자나 파싱 오류처럼 보일 수 있습니다. 상태 메시지에 HTML을 받았다는 내용이 있다면 아래 권한 항목부터 해결해야 합니다.

## “TSV 대신 HTML을 받았습니다”

Google이 시트 데이터 대신 로그인 또는 권한 안내 페이지를 반환한 경우입니다.

- 시트를 링크로 볼 수 있도록 공유합니다.
- 가져올 탭을 직접 연 뒤 주소창의 전체 URL을 다시 복사합니다.
- URL에 `gid=`가 있는지 확인합니다.
- 조직 계정의 외부 공유 정책이 다운로드를 막고 있지 않은지 확인합니다.

`gid`가 없으면 첫 번째 탭인 `0`을 사용합니다. 다른 탭을 가져오려면 해당 탭을 연 URL을 사용해야 합니다.

> 🖼️ **이미지 추가 위치**
>
> - 권장 이미지: Google Sheet 공유 설정과 URL의 `gid` 부분
> - 파일 예시: `Images/troubleshooting-share-and-gid.png`

## Header 관련 오류

지정 범위의 첫 번째 행이 Header가 됩니다.

### 빈 Header

범위 중간에 빈 Header가 있으면 오류가 발생합니다. 사용하지 않는 마지막 열이 비어 있는 경우에는 자동으로 제거되지만, 빈 Header 아래에 데이터가 있으면 안 됩니다.

### 중복 Header

같은 Header 이름을 두 번 사용할 수 없습니다. 열 이름을 고유하게 변경하고 Parser에서도 같은 이름을 사용합니다.

### 필수 Header 누락

`ValidateRequiredHeaders`에 등록한 이름과 시트 Header를 비교합니다. 띄어쓰기, 대소문자, 오타를 확인합니다.

```text
시트:   Max Stack
Parser: MaxStack
```

위 두 이름은 서로 다른 Header입니다.

## 범위 오류

`Range From`과 `Range To`는 `A1`, `D100` 같은 A1 표기법을 사용합니다.

- 시작 셀이 끝 셀보다 뒤에 있지 않은지 확인합니다.
- 첫 번째 행에 Header가 포함되는지 확인합니다.
- 실제 데이터 열보다 지나치게 좁은 범위를 지정하지 않았는지 확인합니다.
- 예제의 네 열을 가져오려면 `A1:D100`처럼 지정합니다.

## Parser가 목록에 보이지 않음

다음 항목을 확인합니다.

- Parser가 `UGoogleSheetParserBase`를 상속하는가?
- 클래스에 `UCLASS(EditInlineNew)`가 지정되어 있는가?
- Parser가 Runtime Module이 아닌 Editor Module에 있는가?
- Editor Module의 Build.cs에 `GoogleSheetLoader`가 포함되어 있는가?
- `.uproject`에 Editor Module이 등록되어 있는가?
- 빌드 후 Unreal Editor를 다시 시작했는가?

## DataTable이 갱신되지 않음

Config에 지정한 Target Table과 Row Structure를 확인합니다.

```cpp
ValidateTargetTable(
    ParserName,
    TargetTable,
    FItemTableRow::StaticStruct(),
    OutError);
```

Parser에서 기대하는 구조체와 DataTable의 Row Structure가 다르면 갱신하지 않습니다. 또한 각 행의 `Row.IsValid()`가 실패하면 해당 행은 추가되지 않으므로 Output Log에서 필수 셀 또는 숫자 변환 오류를 확인합니다.

## 기존 행이 사라짐

샘플 Parser의 `FScopedDataTableEditNotification TableEdit(TargetTable);`은 기본적으로 DataTable을 비운 뒤 새 행을 추가합니다. 이는 시트의 현재 상태를 DataTable에 그대로 반영하기 위한 동작입니다.

수동으로 추가한 행을 유지해야 한다면 Parser의 갱신 정책을 별도로 구현해야 합니다. 기본 사용 방식에서는 **시트 하나당 DataTable 하나**를 유지하고 DataTable을 직접 수정하지 않는 것을 권장합니다.

## 숫자 또는 필수 값 변환 실패

`GetRequiredInt`, `GetRequiredName`, `GetRequiredString`은 값이 없거나 올바르게 변환되지 않으면 해당 행을 오류로 처리합니다.

- 숫자 셀에 단위나 쉼표가 포함되어 있지 않은지 확인합니다.
- 필수 ID가 비어 있지 않은지 확인합니다.
- 선택 값은 `Row.Get(ColumnName, DefaultValue)` 또는 적절한 선택 변환 함수를 사용합니다.

## DataAsset 또는 리소스가 연결되지 않음

- Content 경로가 `/Game/...` 형식인지 확인합니다.
- 이름 형식에 ID를 넣을 위치가 `{0}`으로 지정되어 있는지 확인합니다.
- 에셋 폴더와 실제 파일 이름이 일치하는지 확인합니다.
- Sprite 등을 사용한다면 Editor Module의 Build.cs에 필요한 모듈(예: `Paper2D`)을 추가합니다.
- `Auto Save On Complete`가 꺼져 있다면 변경된 에셋을 직접 저장합니다.

## 예전 문서의 PageName 항목이 없음

현재 버전은 `PageName` 대신 Google Sheet URL의 `gid`로 탭을 선택합니다. Config에 PageName이 보이지 않는 것은 정상입니다. 원하는 탭을 연 상태에서 URL을 복사하여 사용하세요.

## 대시보드가 보이지 않음

- 플러그인이 활성화되어 있는지 확인합니다.
- 에디터를 다시 시작합니다.
- 툴바의 **Sheet Loader** 버튼을 확인합니다.
- 버튼을 눌러 **Google Sheet Dashboard**를 연 뒤 **Refresh List**를 실행합니다.

그래도 원인을 찾기 어렵다면 Config의 상태 메시지, Output Log, 저장된 Normalized JSON 세 가지를 함께 비교하면 어느 단계에서 데이터가 달라졌는지 확인하기 쉽습니다.
