#include "LandscapeHDToolModule.h"
#include "LandscapeHDToolWidget.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "ToolMenus.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Brushes/SlateImageBrush.h"

#define LOCTEXT_NAMESPACE "FLandscapeHDToolModule"

static const FName LandscapeHDToolTabName("LandscapeHDTool");
TSharedPtr<FSlateStyleSet> FLandscapeHDToolModule::StyleInstance = nullptr;

FName FLandscapeHDToolModule::GetStyleSetName()
{
	static FName StyleSetName(TEXT("LandscapeHDToolStyle"));
	return StyleSetName;
}

TSharedRef<FSlateStyleSet> FLandscapeHDToolModule::CreateStyle()
{
	TSharedRef<FSlateStyleSet> Style = MakeShared<FSlateStyleSet>(GetStyleSetName());

	Style->SetContentRoot(IPluginManager::Get().FindPlugin(TEXT("LandscapeHDTool"))->GetBaseDir() / TEXT("Resources"));
	Style->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

	const FVector2D Icon16x16(16.f, 16.f);
	const FVector2D Icon20x20(20.f, 20.f);

	Style->Set("LandscapeHDTool.Icon", new FSlateVectorImageBrush(Style->RootToContentDir(TEXT("Icons/LandscapeHDTool"), TEXT(".svg")), Icon20x20));
	Style->Set("LandscapeHDTool.Icon.Small", new FSlateVectorImageBrush(Style->RootToContentDir(TEXT("Icons/LandscapeHDTool"), TEXT(".svg")), Icon16x16));

	return Style;
}

const ISlateStyle& FLandscapeHDToolModule::GetStyle()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = CreateStyle();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
	return *StyleInstance;
}

void FLandscapeHDToolModule::StartupModule()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = CreateStyle();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(LandscapeHDToolTabName, FOnSpawnTab::CreateRaw(this, &FLandscapeHDToolModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FLandscapeHDToolTabTitle", "LandscapeHDTool"))
		.SetIcon(FSlateIcon(GetStyleSetName(), "LandscapeHDTool.Icon"))
		.SetMenuType(ETabSpawnerMenuType::Enabled)
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetLevelEditorCategory());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FLandscapeHDToolModule::RegisterMenus));
}

void FLandscapeHDToolModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(LandscapeHDToolTabName);

	if (StyleInstance.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
		ensure(StyleInstance.IsUnique());
		StyleInstance.Reset();
	}
}

void FLandscapeHDToolModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.AssetsToolBar");
	FToolMenuSection& ToolbarSection = ToolbarMenu->FindOrAddSection("LandscapeHDTool");

	ToolbarSection.AddEntry(FToolMenuEntry::InitToolBarButton(
		TEXT("LandscapeHDToolButton"),
		FExecuteAction::CreateRaw(this, &FLandscapeHDToolModule::OpenPluginTab),
		LOCTEXT("LandscapeHDToolButtonLabel", "LandscapeHDTool"),
		LOCTEXT("LandscapeHDToolButtonTooltip", "Open LandscapeHDTool"),
		FSlateIcon(GetStyleSetName(), "LandscapeHDTool.Icon", "LandscapeHDTool.Icon.Small")
	));
}

void FLandscapeHDToolModule::OpenPluginTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(LandscapeHDToolTabName);
}

TSharedRef<SDockTab> FLandscapeHDToolModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SLandscapeHDToolWidget)
		];
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FLandscapeHDToolModule, LandscapeHDTool)