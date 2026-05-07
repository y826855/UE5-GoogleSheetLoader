# 📑 GoogleSheetLoader (Unreal Engine Plugin)

Google Sheet의 데이터를 Unreal Engine의 **Data Asset**으로 자동 변환해주는 에디터 유틸리티 플러그인입니다. 유저가 직접 파싱 로직을 정의할 수 있는 베이스 클래스를 제공하여, 프로젝트마다 다른 다양한 데이터 구조에 유연하게 대응할 수 있습니다.

---

## 🛠 주요 특징

### 1. 확장 가능한 파서 베이스 (`UGoogleSheetParserBase`)
* 유저는 `UGoogleSheetParserBase`를 상속받아 자신만의 데이터 에셋 생성 로직을 구현합니다.
* 시트 로드, JSON 변환 등 공통적인 복잡한 과정은 베이스 클래스가 처리하며, 유저는 **데이터를 에셋에 할당하는 부분**에만 집중할 수 있습니다.

### 2. 에디터 디테일 커스터마이징 (`IDetailCustomization`)
* 별도의 창을 띄울 필요 없이, 생성한 파서 에셋의 **디테일(Details) 패널**에서 모든 작업을 수행합니다.
* [Import from Google Sheet] 버튼을 클릭하면 즉시 시트 데이터를 읽어와 프로젝트 내 에셋들을 생성하거나 갱신합니다.

---

## 💻 사용 방법 (How to Use)

### 1. 커스텀 파서 클래스 구현
`UGoogleSheetParserBase`를 상속받는 C++ 클래스를 생성하고, `OnParseComplete` 함수를 오버라이드하여 파싱 로직을 작성합니다.
테스트를 위해 포함된 ItemDataParser 클래스를 확인 하면 이해에 도움이 될것입니다.

```cpp
// MyItemParser.h
UCLASS()
class MYGAME_API UMyItemParser : public UGoogleSheetParserBase
{
    GENERATED_BODY()

protected:
    // 구글 시트 데이터 로드 및 JSON 변환이 완료된 후 호출됩니다.
    virtual void OnParseComplete() override;
    
    // 개별 데이터 에셋을 생성/업데이트하는 헬퍼 함수 예시
    void ProcessItemRow(const FString& ItemID, const TMap<FString, FString>& RowData);
};
