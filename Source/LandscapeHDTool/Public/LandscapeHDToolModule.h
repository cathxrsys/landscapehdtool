#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class SDockTab;
class FSpawnTabArgs;
class FSlateStyleSet;

class FLandscapeHDToolModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static const ISlateStyle& GetStyle();
	static FName GetStyleSetName();

private:
	void RegisterMenus();
	void OpenPluginTab();
	TSharedRef<SDockTab> OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs);

	static TSharedRef<FSlateStyleSet> CreateStyle();
	static TSharedPtr<FSlateStyleSet> StyleInstance;
};