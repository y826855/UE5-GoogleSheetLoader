// Copyright Epic Games, Inc. All Rights Reserved.

#include "GoogleSheetLoader.h"

#include "GoogleSheetConfig.h"
#include "GoogleSheetConfigCustomization.h"
#include "SGoogleSheetDashboard.h"
#include "ToolMenus.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "FGoogleSheetLoaderModule"

static const FName GoogleSheetDashboardTabName("GoogleSheetDashboard");

void FGoogleSheetLoaderModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule =
	FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// 1. 기존 디테일 커스터마이징 등록
	PropertyModule.RegisterCustomClassLayout(
		UGoogleSheetConfig::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(
			&FGoogleSheetConfigCustomization::MakeInstance)
	);

	PropertyModule.NotifyCustomizationModuleChanged();

	if (GIsEditor && !IsRunningCommandlet())
	{
		// 2. 대시보드 탭 등록 (메뉴 그룹 지정 포함)
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(GoogleSheetDashboardTabName,
			FOnSpawnTab::CreateRaw(this, &FGoogleSheetLoaderModule::OnSpawnPluginTab))
			.SetDisplayName(LOCTEXT("FGoogleSheetDashboardTabTitle", "Google Sheet Dashboard"))
			.SetMenuType(ETabSpawnerMenuType::Enabled)
			.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());

		// 3. 툴바 및 메뉴 확장 등록
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FGoogleSheetLoaderModule::RegisterMenus));
	}
}

void FGoogleSheetLoaderModule::ShutdownModule()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(GoogleSheetDashboardTabName);

	if (UToolMenus::Get())
	{
		UToolMenus::UnregisterOwner(this);
	}

	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule =
			FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomClassLayout(
			UGoogleSheetConfig::StaticClass()->GetFName());
	}
}

TSharedRef<SDockTab> FGoogleSheetLoaderModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SGoogleSheetDashboard)
		];
}

void FGoogleSheetLoaderModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	// 툴바의 'Play' 섹션 옆에 버튼 추가
	UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
	if (ToolbarMenu)
	{
		FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("GoogleSheet");
		
		FToolMenuEntry Entry = FToolMenuEntry::InitToolBarButton(
			"OpenGoogleSheetDashboard",
			FUIAction(FExecuteAction::CreateRaw(this, &FGoogleSheetLoaderModule::OnOpenDashboard)),
			LOCTEXT("DashboardLabel", "Sheet Loader"),
			LOCTEXT("DashboardTooltip", "Open Google Sheet Management Dashboard"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "EditorViewport.RelativeTransformMode")
		);
		
		Section.AddEntry(Entry);
	}
}

void FGoogleSheetLoaderModule::OnOpenDashboard()
{
	FGlobalTabmanager::Get()->TryInvokeTab(GoogleSheetDashboardTabName);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FGoogleSheetLoaderModule, GoogleSheetLoader)