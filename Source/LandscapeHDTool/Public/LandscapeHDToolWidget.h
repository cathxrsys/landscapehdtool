#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SLandscapeHDToolWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLandscapeHDToolWidget) {}
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
	
private:
	FReply OnRefreshClicked();
	FReply OnCreateClicked(int32 ResolutionMultiplier);
	
	TSharedPtr<STextBlock> ResultText;
};