#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SCheckBox.h"

class SLandscapeHDToolWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLandscapeHDToolWidget) {}
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
	
private:
	FReply OnRefreshClicked();
	FReply OnCreateClicked(int32 ResolutionMultiplier);
	void OnCopyMaterialCheckChanged(ECheckBoxState NewState);
	ECheckBoxState GetCopyMaterialCheckState() const;
	void OnEnableNaniteCheckChanged(ECheckBoxState NewState);
	ECheckBoxState GetEnableNaniteCheckState() const;
	
	TSharedPtr<STextBlock> ResultText;
	ECheckBoxState bCopyMaterialFromParent = ECheckBoxState::Unchecked;
	ECheckBoxState bEnableNanite = ECheckBoxState::Unchecked;
};