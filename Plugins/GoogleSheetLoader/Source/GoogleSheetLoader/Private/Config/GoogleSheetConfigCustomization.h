#pragma once

#include "IDetailCustomization.h"

class IDetailLayoutBuilder;
class UGoogleSheetConfig;

class FGoogleSheetConfigCustomization : public IDetailCustomization
{
public:
	// 엔진이 이 함수로 인스턴스 생성
	static TSharedRef<IDetailCustomization> MakeInstance();

	// 여기서 패널 UI를 직접 구성
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	FReply OnFetchClicked();

	// 편집 중인 에셋 참조 (WeakPtr로 안전하게)
	TWeakObjectPtr<UGoogleSheetConfig> Config;

	// 패널 리프레시할 때 필요
	IDetailLayoutBuilder* LayoutBuilder = nullptr;
};
