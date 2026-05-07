// Copyright Epic Games, Inc. All Rights Reserved.

#include "GoogleSheetLoader.h"

#include "GoogleSheetConfig.h"
#include "GoogleSheetConfigCustomization.h"

#define LOCTEXT_NAMESPACE "FGoogleSheetLoaderModule"

void FGoogleSheetLoaderModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule =
	FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	PropertyModule.RegisterCustomClassLayout(
		UGoogleSheetConfig::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(
			&FGoogleSheetConfigCustomization::MakeInstance)
	);

	PropertyModule.NotifyCustomizationModuleChanged();
}

void FGoogleSheetLoaderModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule =
			FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomClassLayout(
			UGoogleSheetConfig::StaticClass()->GetFName());
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FGoogleSheetLoaderModule, GoogleSheetLoader)