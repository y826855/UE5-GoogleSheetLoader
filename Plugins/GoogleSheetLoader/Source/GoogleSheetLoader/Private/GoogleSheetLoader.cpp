// Copyright Epic Games, Inc. All Rights Reserved.

#include "GoogleSheetLoader.h"

#include "GoogleSheetConfig.h"
#include "Config/GoogleSheetConfigCustomization.h"
#include "Dashboard/SGoogleSheetDashboard.h"
#include "ToolMenus.h"
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
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			GoogleSheetDashboardTabName,
			FOnSpawnTab::CreateRaw(
				this,
				&FGoogleSheetLoaderModule::OnSpawnDashboardTab))
			.SetDisplayName(LOCTEXT("FGoogleSheetDashboardTabTitle", "Google Sheet Dashboard"))
			.SetMenuType(ETabSpawnerMenuType::Enabled)
			.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());

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

TSharedRef<SDockTab> FGoogleSheetLoaderModule::OnSpawnDashboardTab(
	const FSpawnTabArgs& SpawnTabArgs)
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
			LOCTEXT("DashboardTooltip", "Open the Google Sheet dashboard"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "EditorViewport.RelativeTransformMode")
		);
		
		Section.AddEntry(Entry);
	}

	UToolMenu* ToolsMenu =
		UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
	if (ToolsMenu)
	{
		FToolMenuSection& Section =
			ToolsMenu->FindOrAddSection("GoogleSheetLoader");

		Section.AddEntry(FToolMenuEntry::InitMenuEntry(
			"OpenGoogleSheetDashboardFromTools",
			LOCTEXT("DashboardMenuLabel", "Google Sheet Dashboard"),
			LOCTEXT("DashboardMenuTooltip", "Open the Google Sheet dashboard"),
			FSlateIcon(
				FAppStyle::GetAppStyleSetName(),
				"EditorViewport.RelativeTransformMode"),
			FUIAction(FExecuteAction::CreateRaw(
				this,
				&FGoogleSheetLoaderModule::OnOpenDashboard))));
	}
}

void FGoogleSheetLoaderModule::OnOpenDashboard()
{
	FGlobalTabmanager::Get()->TryInvokeTab(GoogleSheetDashboardTabName);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FGoogleSheetLoaderModule, GoogleSheetLoader)
