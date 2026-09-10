#include "LandscapeHDToolWidget.h"
#include "LandscapeStreamingProxy.h"
#include "Landscape.h"
#include "LandscapeInfo.h"
#include "LandscapeComponent.h"
#include "LandscapeEdit.h"
#include "LandscapeLayerInfoObject.h"
#include "Editor.h"
#include "Selection.h"
#include "Engine/World.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "SLandscapeHDToolWidget"

void SLandscapeHDToolWidget::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("LandscapeHDTool")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10, 2, 10, 6)
		[
			SNew(SCheckBox)
			.IsChecked(this, &SLandscapeHDToolWidget::GetCopyMaterialCheckState)
			.OnCheckStateChanged(this, &SLandscapeHDToolWidget::OnCopyMaterialCheckChanged)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Copy material from Parent Landscape")))
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10, 0, 10, 6)
		[
			SNew(SCheckBox)
			.IsChecked(this, &SLandscapeHDToolWidget::GetEnableNaniteCheckState)
			.OnCheckStateChanged(this, &SLandscapeHDToolWidget::OnEnableNaniteCheckChanged)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Enable Nanite")))
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 5, 0)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Refresh")))
				.OnClicked(this, &SLandscapeHDToolWidget::OnRefreshClicked)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 5, 0)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Create 1x")))
				.OnClicked_Lambda([this]() { return OnCreateClicked(1); })
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 5, 0)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Create 2x")))
				.OnClicked_Lambda([this]() { return OnCreateClicked(2); })
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 5, 0)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Create 4x")))
				.OnClicked_Lambda([this]() { return OnCreateClicked(4); })
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 5, 0)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Create 8x")))
				.OnClicked_Lambda([this]() { return OnCreateClicked(8); })
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 5, 0)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Create 16x")))
				.OnClicked_Lambda([this]() { return OnCreateClicked(16); })
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Create 32x")))
				.OnClicked_Lambda([this]() { return OnCreateClicked(32); })
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(10)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SAssignNew(ResultText, STextBlock)
				.Text(FText::FromString(TEXT("Select landscape streaming proxies in the viewport and click Refresh")))
				.AutoWrapText(true)
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10, 5, 10, 10)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("by cathxrsys")))
			.Font(FCoreStyle::GetDefaultFontStyle("Italic", 10))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)))
			.Justification(ETextJustify::Right)
		]
	];
}

static TArray<ALandscapeStreamingProxy*> GetSelectedProxies()
{
	TArray<ALandscapeStreamingProxy*> SelectedProxies;
	if (!GEditor) return SelectedProxies;

	USelection* SelectedActors = GEditor->GetSelectedActors();
	if (!SelectedActors) return SelectedProxies;

	for (int32 Idx = 0; Idx < SelectedActors->Num(); ++Idx)
	{
		ALandscapeStreamingProxy* Proxy = Cast<ALandscapeStreamingProxy>(SelectedActors->GetSelectedObject(Idx));
		if (Proxy)
		{
			SelectedProxies.Add(Proxy);
		}
	}
	return SelectedProxies;
}

static void UpscaleHeightmap(const TArray<uint16>& Src, int32 SrcW, int32 SrcH,
	TArray<uint16>& Dst, int32 DstW, int32 DstH)
{
	Dst.SetNumUninitialized(DstW * DstH);

	for (int32 Y = 0; Y < DstH; ++Y)
	{
		for (int32 X = 0; X < DstW; ++X)
		{
			float FX = (float)X * (SrcW - 1) / (DstW - 1);
			float FY = (float)Y * (SrcH - 1) / (DstH - 1);

			int32 X0 = FMath::Clamp(FMath::FloorToInt(FX), 0, SrcW - 2);
			int32 Y0 = FMath::Clamp(FMath::FloorToInt(FY), 0, SrcH - 2);
			int32 X1 = FMath::Min(X0 + 1, SrcW - 1);
			int32 Y1 = FMath::Min(Y0 + 1, SrcH - 1);

			float FracX = FX - X0;
			float FracY = FY - Y0;

			float V00 = Src[Y0 * SrcW + X0];
			float V10 = Src[Y0 * SrcW + X1];
			float V01 = Src[Y1 * SrcW + X0];
			float V11 = Src[Y1 * SrcW + X1];

			float Val = V00 * (1 - FracX) * (1 - FracY)
				+ V10 * FracX * (1 - FracY)
				+ V01 * (1 - FracX) * FracY
				+ V11 * FracX * FracY;

			Dst[Y * DstW + X] = (uint16)FMath::Clamp(FMath::RoundToInt(Val), 0, 65535);
		}
	}
}

static const size_t ChannelOffsets[4] = {
	STRUCT_OFFSET(FColor, R),
	STRUCT_OFFSET(FColor, G),
	STRUCT_OFFSET(FColor, B),
	STRUCT_OFFSET(FColor, A)
};

static bool ReadLayerWeightDataFromComponents(
	ALandscapeStreamingProxy* Proxy, ULandscapeLayerInfoObject* LayerInfo,
	int32 MinX, int32 MinY, int32 MaxX, int32 MaxY,
	TArray<uint8>& OutData)
{
	int32 VertsX = (MaxX - MinX) + 1;
	int32 VertsY = (MaxY - MinY) + 1;
	OutData.SetNumZeroed(VertsX * VertsY);

	TArray<ULandscapeComponent*> Components;
	Proxy->GetComponents<ULandscapeComponent>(Components);

	bool bFound = false;

	for (ULandscapeComponent* Comp : Components)
	{
		if (!Comp) continue;

		FIntPoint CompBase = Comp->GetSectionBase();
		int32 CompSize = Comp->ComponentSizeQuads;
		int32 SubSize = Comp->SubsectionSizeQuads;
		int32 NumSubs = Comp->NumSubsections;
		int32 SubSizeVerts = SubSize + 1;

		const TArray<FWeightmapLayerAllocationInfo>& Allocations = Comp->GetWeightmapLayerAllocations();
		const TArray<UTexture2D*>& Textures = Comp->GetWeightmapTextures();

		int32 AllocIdx = INDEX_NONE;
		for (int32 i = 0; i < Allocations.Num(); i++)
		{
			if (Allocations[i].LayerInfo == LayerInfo)
			{
				AllocIdx = i;
				break;
			}
		}

		if (AllocIdx < 0) continue;
		if (Allocations[AllocIdx].WeightmapTextureIndex >= Textures.Num()) continue;

		UTexture2D* Texture = Textures[Allocations[AllocIdx].WeightmapTextureIndex];
		uint8 Channel = Allocations[AllocIdx].WeightmapTextureChannel;

		const uint8* MipData = (const uint8*)Texture->Source.LockMipReadOnly(0);
		if (!MipData)
		{
			Texture->Source.UnlockMip(0);
			continue;
		}

		const uint8* ChannelData = MipData + ChannelOffsets[Channel];
		int32 TexSize = SubSizeVerts * NumSubs;

		for (int32 CY = 0; CY <= CompSize; CY++)
		{
			for (int32 CX = 0; CX <= CompSize; CX++)
			{
				int32 LandX = CompBase.X + CX;
				int32 LandY = CompBase.Y + CY;

				if (LandX < MinX || LandX > MaxX || LandY < MinY || LandY > MaxY)
					continue;

				int32 OutX = LandX - MinX;
				int32 OutY = LandY - MinY;

				int32 SubNumX = (CX > 0) ? (CX - 1) / SubSize : 0;
				int32 SubNumY = (CY > 0) ? (CY - 1) / SubSize : 0;
				int32 SubX = (CX > 0) ? (CX - 1) % SubSize + 1 : 0;
				int32 SubY = (CY > 0) ? (CY - 1) % SubSize + 1 : 0;

				int32 TexX = SubNumX * SubSizeVerts + SubX;
				int32 TexY = SubNumY * SubSizeVerts + SubY;

				int32 TexelIndex = TexX + TexY * TexSize;
				OutData[OutY * VertsX + OutX] = ChannelData[4 * TexelIndex];
				bFound = true;
			}
		}

		Texture->Source.UnlockMip(0);
	}

	return bFound;
}

static void UpscaleWeightmap(const TArray<uint8>& Src, int32 SrcW, int32 SrcH,
	TArray<uint8>& Dst, int32 DstW, int32 DstH)
{
	Dst.SetNumUninitialized(DstW * DstH);

	for (int32 Y = 0; Y < DstH; ++Y)
	{
		for (int32 X = 0; X < DstW; ++X)
		{
			float FX = (float)X * (SrcW - 1) / (DstW - 1);
			float FY = (float)Y * (SrcH - 1) / (DstH - 1);

			int32 X0 = FMath::Clamp(FMath::FloorToInt(FX), 0, SrcW - 2);
			int32 Y0 = FMath::Clamp(FMath::FloorToInt(FY), 0, SrcH - 2);
			int32 X1 = FMath::Min(X0 + 1, SrcW - 1);
			int32 Y1 = FMath::Min(Y0 + 1, SrcH - 1);

			float FracX = FX - X0;
			float FracY = FY - Y0;

			float V00 = Src[Y0 * SrcW + X0];
			float V10 = Src[Y0 * SrcW + X1];
			float V01 = Src[Y1 * SrcW + X0];
			float V11 = Src[Y1 * SrcW + X1];

			float Val = V00 * (1 - FracX) * (1 - FracY)
				+ V10 * FracX * (1 - FracY)
				+ V01 * (1 - FracX) * FracY
				+ V11 * FracX * FracY;

			Dst[Y * DstW + X] = (uint8)FMath::Clamp(FMath::RoundToInt(Val), 0, 255);
		}
	}
}

void SLandscapeHDToolWidget::OnCopyMaterialCheckChanged(ECheckBoxState NewState)
{
	bCopyMaterialFromParent = NewState;
}

ECheckBoxState SLandscapeHDToolWidget::GetCopyMaterialCheckState() const
{
	return bCopyMaterialFromParent;
}

void SLandscapeHDToolWidget::OnEnableNaniteCheckChanged(ECheckBoxState NewState)
{
	bEnableNanite = NewState;
}

ECheckBoxState SLandscapeHDToolWidget::GetEnableNaniteCheckState() const
{
	return bEnableNanite;
}

FReply SLandscapeHDToolWidget::OnRefreshClicked()
{
	if (!GEditor)
	{
		ResultText->SetText(FText::FromString(TEXT("Error: No Editor")));
		return FReply::Handled();
	}

	TArray<ALandscapeStreamingProxy*> SelectedProxies = GetSelectedProxies();

	if (SelectedProxies.Num() == 0)
	{
		ResultText->SetText(FText::FromString(TEXT("No landscape streaming proxies selected.\n\nSelect ALandscapeStreamingProxy actors in the viewport, then click Refresh.")));
		return FReply::Handled();
	}

	FString Result = FString::Printf(TEXT("Selected proxies: %d\n\n"), SelectedProxies.Num());

	for (int32 Idx = 0; Idx < SelectedProxies.Num(); ++Idx)
	{
		ALandscapeStreamingProxy* Proxy = SelectedProxies[Idx];
		FString ProxyName = Proxy->GetName();

		bool bHasLandscapeInfo = Proxy->GetLandscapeInfo() != nullptr;
		bool bHasHeightmap = false;
		int32 ComponentCount = 0;

		if (bHasLandscapeInfo)
		{
			TArray<ULandscapeComponent*> Components;
			Proxy->GetComponents<ULandscapeComponent>(Components);
			ComponentCount = Components.Num();

			for (ULandscapeComponent* Comp : Components)
			{
				if (Comp && Comp->GetHeightmap())
				{
					bHasHeightmap = true;
					break;
				}
			}
		}

		FVector Location = Proxy->GetActorLocation();
		FRotator Rotation = Proxy->GetActorRotation();
		FVector Scale = Proxy->GetActorScale3D();

		int32 ComponentSizeQuads = Proxy->ComponentSizeQuads;
		int32 SubsectionSizeQuads = Proxy->SubsectionSizeQuads;

		Result += FString::Printf(TEXT("[%d] %s\n"), Idx + 1, *ProxyName);
		Result += FString::Printf(TEXT("    Position: X=%.1f  Y=%.1f  Z=%.1f\n"), Location.X, Location.Y, Location.Z);
		Result += FString::Printf(TEXT("    Rotation: P=%.1f  Y=%.1f  R=%.1f\n"), Rotation.Pitch, Rotation.Yaw, Rotation.Roll);
		Result += FString::Printf(TEXT("    Scale: X=%.2f  Y=%.2f  Z=%.2f\n"), Scale.X, Scale.Y, Scale.Z);
		Result += FString::Printf(TEXT("    Component Size: %d quads\n"), ComponentSizeQuads);
		Result += FString::Printf(TEXT("    Subsection Size: %d quads\n"), SubsectionSizeQuads);

		double CurrentResolution = Scale.X / 100.0;
		Result += FString::Printf(TEXT("    Current Resolution: 1 point / %.2f m\n"), CurrentResolution);
		Result += FString::Printf(TEXT("    2x Resolution: 1 point / %.2f m\n"), CurrentResolution / 2.0);
		Result += FString::Printf(TEXT("    4x Resolution: 1 point / %.2f m\n"), CurrentResolution / 4.0);
		Result += FString::Printf(TEXT("    8x Resolution: 1 point / %.2f m\n"), CurrentResolution / 8.0);
		Result += FString::Printf(TEXT("    16x Resolution: 1 point / %.2f m\n"), CurrentResolution / 16.0);
		Result += FString::Printf(TEXT("    32x Resolution: 1 point / %.2f m\n"), CurrentResolution / 32.0);

		Result += FString::Printf(TEXT("    Components: %d\n"), ComponentCount);
		Result += FString::Printf(TEXT("    Heightmap accessible: %s\n"), bHasHeightmap ? TEXT("YES") : TEXT("NO"));

		TSet<ULandscapeLayerInfoObject*> LayerInfos = Proxy->GetValidTargetLayerObjects();
		Result += FString::Printf(TEXT("    TargetLayers: %d\n"), LayerInfos.Num());

		if (ComponentCount > 0)
		{
			TArray<ULandscapeComponent*> Comps;
			Proxy->GetComponents<ULandscapeComponent>(Comps);
			if (Comps.Num() > 0 && Comps[0])
			{
				const TArray<FWeightmapLayerAllocationInfo>& Allocs = Comps[0]->GetWeightmapLayerAllocations();
				Result += FString::Printf(TEXT("    WeightmapAllocs[0]: %d\n"), Allocs.Num());
				for (const FWeightmapLayerAllocationInfo& A : Allocs)
				{
					FString LayerNameStr = A.LayerInfo ? A.LayerInfo->GetLayerName().ToString() : TEXT("null");
					Result += FString::Printf(TEXT("      - %s (tex=%d ch=%d)\n"),
						*LayerNameStr,
						A.WeightmapTextureIndex, A.WeightmapTextureChannel);
				}
			}
		}

		ALandscape* ParentLandscape = Proxy->GetLandscapeActor();
		if (ParentLandscape)
		{
			TSet<ULandscapeLayerInfoObject*> ParentLayers = ParentLandscape->GetValidTargetLayerObjects();
			Result += FString::Printf(TEXT("    Parent TargetLayers: %d\n"), ParentLayers.Num());
		}

		Result += TEXT("\n");
	}

	ResultText->SetText(FText::FromString(Result));
	return FReply::Handled();
}

FReply SLandscapeHDToolWidget::OnCreateClicked(int32 ResolutionMultiplier)
{
	if (!GEditor)
	{
		ResultText->SetText(FText::FromString(TEXT("Error: No Editor")));
		return FReply::Handled();
	}

	TArray<ALandscapeStreamingProxy*> SelectedProxies = GetSelectedProxies();

	if (SelectedProxies.Num() == 0)
	{
		ResultText->SetText(FText::FromString(TEXT("No landscape streaming proxies selected.")));
		return FReply::Handled();
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		ResultText->SetText(FText::FromString(TEXT("Error: No World")));
		return FReply::Handled();
	}

	FScopedTransaction Transaction(LOCTEXT("CreateLandscapesFromProxies", "Create Landscapes from Proxies"));

	int32 CreatedCount = 0;
	FString Result = FString::Printf(TEXT("Creating %d landscapes (%dx resolution)...\n\n"), SelectedProxies.Num(), ResolutionMultiplier);

	for (int32 Idx = 0; Idx < SelectedProxies.Num(); ++Idx)
	{
		ALandscapeStreamingProxy* SourceProxy = SelectedProxies[Idx];

		ALandscape* SourceLandscape = SourceProxy->GetLandscapeActor();
		if (!SourceLandscape)
		{
			Result += FString::Printf(TEXT("[%d] %s - ERROR: No source landscape actor\n\n"), Idx + 1, *SourceProxy->GetName());
			continue;
		}

		ULandscapeInfo* LandscapeInfo = SourceProxy->GetLandscapeInfo();
		if (!LandscapeInfo)
		{
			Result += FString::Printf(TEXT("[%d] %s - ERROR: No landscape info\n\n"), Idx + 1, *SourceProxy->GetName());
			continue;
		}

		TArray<ULandscapeComponent*> SourceComponents;
		SourceProxy->GetComponents<ULandscapeComponent>(SourceComponents);

		if (SourceComponents.Num() == 0)
		{
			Result += FString::Printf(TEXT("[%d] %s - ERROR: No components\n\n"), Idx + 1, *SourceProxy->GetName());
			continue;
		}

		int32 ComponentSizeQuads = SourceProxy->ComponentSizeQuads;
		int32 SubsectionSizeQuads = SourceProxy->SubsectionSizeQuads;
		int32 NumSubsections = ComponentSizeQuads / SubsectionSizeQuads;
		if (NumSubsections < 1) NumSubsections = 1;

		FVector Scale = SourceProxy->GetActorScale3D();

		int32 MinX = MAX_int32, MinY = MAX_int32;
		int32 MaxX = MIN_int32, MaxY = MIN_int32;

		for (ULandscapeComponent* Comp : SourceComponents)
		{
			if (!Comp) continue;
			FIntPoint CompBase = Comp->GetSectionBase();
			MinX = FMath::Min(MinX, CompBase.X);
			MinY = FMath::Min(MinY, CompBase.Y);
			MaxX = FMath::Max(MaxX, CompBase.X + ComponentSizeQuads);
			MaxY = FMath::Max(MaxY, CompBase.Y + ComponentSizeQuads);
		}

		int32 OrigSizeX = MaxX - MinX;
		int32 OrigSizeY = MaxY - MinY;
		int32 OrigVertsX = OrigSizeX + 1;
		int32 OrigVertsY = OrigSizeY + 1;

		TArray<uint16> OrigHeightData;
		OrigHeightData.AddZeroed(OrigVertsX * OrigVertsY);

		{
			FLandscapeEditDataInterface SourceEdit(LandscapeInfo);
			int32 RX1 = MinX, RY1 = MinY, RX2 = MaxX, RY2 = MaxY;
			SourceEdit.GetHeightData(RX1, RY1, RX2, RY2, OrigHeightData.GetData(), 0);
		}

		int32 NewSizeX = OrigSizeX * ResolutionMultiplier;
		int32 NewSizeY = OrigSizeY * ResolutionMultiplier;
		int32 NewVertsX = NewSizeX + 1;
		int32 NewVertsY = NewSizeY + 1;

		int32 NewSubsectionSize = SubsectionSizeQuads;
		int32 NewComponentSize = ComponentSizeQuads;
		int32 NewNumSubsections = NumSubsections;

		TArray<uint16> UpscaledHeightData;
		UpscaleHeightmap(OrigHeightData, OrigVertsX, OrigVertsY, UpscaledHeightData, NewVertsX, NewVertsY);

		FVector OrigScale = SourceLandscape->GetActorScale3D();
		FVector NewScale(OrigScale.X / ResolutionMultiplier, OrigScale.Y / ResolutionMultiplier, OrigScale.Z);

		FVector NewLocation = SourceLandscape->GetActorLocation();

		ALandscape* NewLandscape = World->SpawnActor<ALandscape>(NewLocation, SourceLandscape->GetActorRotation());
		if (!NewLandscape)
		{
			Result += FString::Printf(TEXT("[%d] %s - ERROR: Failed to spawn landscape\n\n"), Idx + 1, *SourceProxy->GetName());
			continue;
		}

		NewLandscape->SetActorScale3D(NewScale);

		FString Suffix = ResolutionMultiplier > 1 ? FString::Printf(TEXT("_%dx"), ResolutionMultiplier) : TEXT("");
		FString NewName = FString::Printf(TEXT("Landscape_%s%s"), *SourceProxy->GetName(), *Suffix);
		NewLandscape->SetActorLabel(NewName);

		TMap<FGuid, TArray<uint16>> HeightDataPerLayers;
		HeightDataPerLayers.Add(FGuid(), UpscaledHeightData);

		TMap<FGuid, TArray<FLandscapeImportLayerInfo>> MaterialLayerDataPerLayers;

		TArray<FLandscapeImportLayerInfo>& DefaultLayerInfos = MaterialLayerDataPerLayers.Add(FGuid());

		TSet<ULandscapeLayerInfoObject*> SourceLayerInfos;
		if (SourceLandscape)
		{
			SourceLayerInfos = SourceLandscape->GetValidTargetLayerObjects();
		}
		if (SourceLayerInfos.Num() == 0)
		{
			SourceLayerInfos = SourceProxy->GetValidTargetLayerObjects();
		}

		Result += FString::Printf(TEXT("[%d] Layer sources: proxy=%d parent=%d\n"), Idx + 1,
			SourceProxy->GetValidTargetLayerObjects().Num(),
			SourceLandscape ? SourceLandscape->GetValidTargetLayerObjects().Num() : 0);

		int32 LayersWithWeight = 0;

		for (ULandscapeLayerInfoObject* LayerInfo : SourceLayerInfos)
		{
			if (!LayerInfo) continue;
			FName LayerName = LayerInfo->GetLayerName();

			FLandscapeImportLayerInfo ImportInfo(LayerName);
			ImportInfo.LayerInfo = LayerInfo;

			TArray<uint8> OrigWeightData;
			bool bGotWeight = ReadLayerWeightDataFromComponents(
				SourceProxy, LayerInfo, MinX, MinY, MaxX, MaxY, OrigWeightData);

			Result += FString::Printf(TEXT("    %s: weightData=%d found=%d\n"),
				*LayerName.ToString(), OrigWeightData.Num(), bGotWeight ? 1 : 0);

			if (bGotWeight && OrigWeightData.Num() == OrigVertsX * OrigVertsY)
			{
				TArray<uint8> UpscaledWeightData;
				UpscaleWeightmap(OrigWeightData, OrigVertsX, OrigVertsY, UpscaledWeightData, NewVertsX, NewVertsY);
				ImportInfo.LayerData = MoveTemp(UpscaledWeightData);
				LayersWithWeight++;
			}

			DefaultLayerInfos.Add(MoveTemp(ImportInfo));
		}

		Result += FString::Printf(TEXT("    Total: %d layers, %d with weight data\n"), DefaultLayerInfos.Num(), LayersWithWeight);

		NewLandscape->Import(
			FGuid::NewGuid(),
			MinX * ResolutionMultiplier, MinY * ResolutionMultiplier,
			(MinX + OrigSizeX) * ResolutionMultiplier, (MinY + OrigSizeY) * ResolutionMultiplier,
			NewNumSubsections,
			NewSubsectionSize,
			HeightDataPerLayers,
			nullptr,
			MaterialLayerDataPerLayers,
			ELandscapeImportAlphamapType::Additive,
			TArrayView<const FLandscapeLayer>()
		);

		if (bCopyMaterialFromParent == ECheckBoxState::Checked && SourceLandscape)
		{
			UMaterialInterface* ParentMaterial = SourceLandscape->GetLandscapeMaterial();
			if (ParentMaterial)
			{
				NewLandscape->LandscapeMaterial = ParentMaterial;
				Result += FString::Printf(TEXT("    Material copied from parent: %s\n"), *ParentMaterial->GetName());
			}
			else
			{
				Result += TEXT("    Parent has no material set\n");
			}
		}

		if (bEnableNanite == ECheckBoxState::Checked && SourceLandscape)
		{
			FProperty* NaniteProp = ALandscapeProxy::StaticClass()->FindPropertyByName(FName("bEnableNanite"));
			if (NaniteProp)
			{
				bool bSourceNanite = SourceLandscape->IsNaniteEnabled();
				NaniteProp->SetValue_InContainer(NewLandscape, &bSourceNanite);
				Result += FString::Printf(TEXT("    Nanite: %s (copied from parent)\n"), bSourceNanite ? TEXT("enabled") : TEXT("disabled"));
			}
		}

		CreatedCount++;
		Result += FString::Printf(TEXT("[%d] %s - CREATED (%dx)\n"), Idx + 1, *SourceProxy->GetName(), ResolutionMultiplier);
		Result += FString::Printf(TEXT("    New: %s\n"), *NewLandscape->GetName());
		Result += FString::Printf(TEXT("    Heightmap: %d x %d vertices\n"), NewVertsX, NewVertsY);
		Result += FString::Printf(TEXT("    Layers: %d\n\n"), SourceLayerInfos.Num());
	}

	Result += FString::Printf(TEXT("\nDone! Created %d landscapes."), CreatedCount);
	ResultText->SetText(FText::FromString(Result));

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE