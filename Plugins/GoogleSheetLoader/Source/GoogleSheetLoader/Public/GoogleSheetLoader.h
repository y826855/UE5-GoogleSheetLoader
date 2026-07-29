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
	TSharedRef<class SDockTab> OnSpawnDashboardTab(
		const class FSpawnTabArgs& SpawnTabArgs);

	void RegisterMenus();

	void OnOpenDashboard();
};
