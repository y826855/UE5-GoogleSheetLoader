// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FGoogleSheetLoaderModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/** 대시보드 탭 생성 콜백 */
	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

	/** 메뉴 및 툴바 확장 등록 */
	void RegisterMenus();

	/** 대시보드 열기 실행 */
	void OnOpenDashboard();
};
