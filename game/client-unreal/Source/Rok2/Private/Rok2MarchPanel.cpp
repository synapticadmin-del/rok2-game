// Copyright ROK2.
// P6-T1: أيقونات إجرائية من URok2ArtAssets في أزرار الكشافة والإرسال (بدل الإيموجي).
// P6-T3: اللوحة تفتح من المركز + ضغطة محسوسة على الكشافة والإرسال.

#include "Rok2MarchPanel.h"
#include "Rok2Typography.h"
#include "Rok2Api.h"
#include "Rok2ArtAssets.h"
#include "Rok2MotionLibrary.h"
#include "Rok2WorldRenderer.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/SpinBox.h"
#include "Components/Button.h"
#include "Components/Spacer.h"
#include "Components/Image.h"
#include "Components/ComboBoxString.h"

void URok2MarchPanel::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		WidgetTree->RootWidget = RootCanvas;

		UBorder* MainBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MainBorder"));
		MainBorder->SetBrushColor(FLinearColor(0.05f, 0.05f, 0.05f, 0.9f));
		
		UCanvasPanelSlot* BorderSlot = RootCanvas->AddChildToCanvas(MainBorder);
		BorderSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		BorderSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			BorderSlot->SetSize(FVector2D(400.f, 600.f));

		UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VBox"));
		MainBorder->AddChild(VBox);

		// Title
		TargetNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TargetNameText"));
		TargetNameText->SetText(FText::FromString(TargetName.IsEmpty() ? TargetType : TargetName));
		TargetNameText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.8f, 0.2f)));
		URok2Typography::ApplyFont(TargetNameText, ERok2TextRole::Display);
		UVerticalBoxSlot* TitleSlot = VBox->AddChildToVerticalBox(TargetNameText);
		TitleSlot->SetPadding(FMargin(20.f, 20.f, 20.f, 10.f));
		TitleSlot->SetHorizontalAlignment(HAlign_Center);

		// Distance
		DistanceText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DistanceText"));
		float Dist = 0.f;
		if (Api && Api->HasPlayer())
		{
			const FRok2Player& P = Api->GetPlayer();
			Dist = FVector2D::Distance(FVector2D(P.X, P.Y), FVector2D(ToX, ToY));
		}
		DistanceText->SetText(FText::FromString(FString::Printf(TEXT("Distance: %.0f"), Dist)));
					DistanceText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			UVerticalBoxSlot* DistSlot = VBox->AddChildToVerticalBox(DistanceText);
			DistSlot->SetPadding(FMargin(20.f, 0.f, 20.f, 6.f));
			DistSlot->SetHorizontalAlignment(HAlign_Center);

			MarchAvailabilityText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MarchAvailabilityText"));
			ARok2WorldRenderer* WorldRenderer = Cast<ARok2WorldRenderer>(UGameplayStatics::GetActorOfClass(GetWorld(), ARok2WorldRenderer::StaticClass()));
			const int32 ActiveMarches = WorldRenderer ? WorldRenderer->GetActiveMarchCount() : 0;
			const int32 MarchCapacity = WorldRenderer ? WorldRenderer->GetMarchCapacity() : 1;
			const bool bCanDispatch = WorldRenderer && WorldRenderer->CanInteractWithWorldTarget(TargetType, true);
			MarchAvailabilityText->SetText(FText::FromString(bCanDispatch
				? FString::Printf(TEXT("المسيرات: %d / %d · تكبير تكتيكي جاهز"), ActiveMarches, MarchCapacity)
				: FString::Printf(TEXT("المسيرات: %d / %d · قرّب الخريطة أو حرّر مسيرة"), ActiveMarches, MarchCapacity)));
			MarchAvailabilityText->SetColorAndOpacity(FSlateColor(bCanDispatch ? FLinearColor(0.35f, 0.95f, 0.55f) : FLinearColor(1.0f, 0.65f, 0.25f)));
			UVerticalBoxSlot* AvailabilitySlot = VBox->AddChildToVerticalBox(MarchAvailabilityText);
			AvailabilitySlot->SetPadding(FMargin(20.f, 0.f, 20.f, 14.f));
			AvailabilitySlot->SetHorizontalAlignment(HAlign_Center);



		auto CreateTroopInput = [this, VBox](const FString& Label, USpinBox*& OutSpinBox)
		{
			UHorizontalBox* HBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			UTextBlock* Lbl = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Lbl->SetText(FText::FromString(Label));
			Lbl->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			UHorizontalBoxSlot* LblSlot = HBox->AddChildToHorizontalBox(Lbl);
			LblSlot->SetVerticalAlignment(VAlign_Center);
			LblSlot->Size.SizeRule = ESlateSizeRule::Fill;

			OutSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass());
			OutSpinBox->SetMinValue(0);
			OutSpinBox->SetMaxValue(100000);
			OutSpinBox->SetValue(0);
			UHorizontalBoxSlot* SpinSlot = HBox->AddChildToHorizontalBox(OutSpinBox);
			SpinSlot->Size.SizeRule = ESlateSizeRule::Fill;

			UVerticalBoxSlot* VSlot = VBox->AddChildToVerticalBox(HBox);
			VSlot->SetPadding(FMargin(20.f, 10.f, 20.f, 10.f));
		};

		CreateTroopInput(TEXT("Infantry"), InfantrySpinBox);
		CreateTroopInput(TEXT("Cavalry"), CavalrySpinBox);
		CreateTroopInput(TEXT("Archer"), ArcherSpinBox);

		PrimaryCommanderBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("PrimaryCommanderBox"));
		UVerticalBoxSlot* PrimarySlot = VBox->AddChildToVerticalBox(PrimaryCommanderBox);
		PrimarySlot->SetPadding(FMargin(20.f, 5.f, 20.f, 5.f));

		SecondaryCommanderBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("SecondaryCommanderBox"));
		UVerticalBoxSlot* SecondarySlot = VBox->AddChildToVerticalBox(SecondaryCommanderBox);
		SecondarySlot->SetPadding(FMargin(20.f, 5.f, 20.f, 5.f));

					if (Api)
			{
				for (const FRok2Commander& Cmd : Api->GetCommanders())
				{
					PrimaryCommanderBox->AddOption(Cmd.Id);
					SecondaryCommanderBox->AddOption(Cmd.Id);
				}
				if (PrimaryCommanderBox->GetOptionCount() > 0)
				{
					PrimaryCommanderBox->SetSelectedIndex(0);
				}
				if (SecondaryCommanderBox->GetOptionCount() > 0)
				{
					SecondaryCommanderBox->SetSelectedIndex(FMath::Min(1, SecondaryCommanderBox->GetOptionCount() - 1));
				}
			}

			// إعادة التوجيه تختار مسيرة شخصية حية فقط؛ الخادم يعيد التحقق عند التنفيذ.
			UTextBlock* RedirectLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RedirectLabel"));
			RedirectLabel->SetText(FText::FromString(TEXT("إعادة توجيه جيش متحرك")));
			RedirectLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.45f, 0.82f, 1.f)));
			URok2Typography::ApplyFont(RedirectLabel, ERok2TextRole::Label);
			UVerticalBoxSlot* RedirectLabelSlot = VBox->AddChildToVerticalBox(RedirectLabel);
			RedirectLabelSlot->SetPadding(FMargin(20.f, 12.f, 20.f, 4.f));

			RedirectMarchBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("RedirectMarchBox"));
			RedirectOptionIds.Empty();
			if (Api && Api->HasPlayer())
			{
				const FString PlayerId = Api->GetPlayer().Id;
				for (const FRok2MarchEntity& March : Api->GetWorldSnapshot().Marches)
				{
					if (March.OwnerPlayerId != PlayerId || March.State != TEXT("moving") || March.Kind == TEXT("rally")) continue;
					const FString OptionLabel = FString::Printf(TEXT("%s → %s"), *March.Id.Left(8), *March.TargetType);
					RedirectOptionIds.Add(OptionLabel, March.Id);
					RedirectMarchBox->AddOption(OptionLabel);
				}
			}
			if (RedirectMarchBox->GetOptionCount() > 0)
			{
				RedirectMarchBox->SetSelectedIndex(0);
			}
			else
			{
				RedirectMarchBox->AddOption(TEXT("لا توجد مسيرات شخصية قابلة للتحويل"));
				RedirectMarchBox->SetIsEnabled(false);
			}
			UVerticalBoxSlot* RedirectMarchSlot = VBox->AddChildToVerticalBox(RedirectMarchBox);
			RedirectMarchSlot->SetPadding(FMargin(20.f, 0.f, 20.f, 5.f));

			USpacer* Spacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("Spacer"));
		UVerticalBoxSlot* SpacerSlot = VBox->AddChildToVerticalBox(Spacer);
		SpacerSlot->Size.SizeRule = ESlateSizeRule::Fill;

		// P5-T5: زر استكشاف (يرسل كشافة بدون قوات) — P6-T1: أيقونة منظار إجرائية
		UButton* ScoutButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ScoutButton"));
		ScoutButton->WidgetStyle.Normal.TintColor = FSlateColor(FLinearColor(0.2f, 0.5f, 0.8f));
		UVerticalBoxSlot* ScoutSlot = VBox->AddChildToVerticalBox(ScoutButton);
		ScoutSlot->SetPadding(FMargin(20.f, 10.f, 20.f, 10.f));

		{
			UHorizontalBox* ScoutBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			ScoutButton->AddChild(ScoutBox);
			UImage* ScoutIco = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			ScoutIco->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("scout"), 18.f, FLinearColor::White));
			ScoutIco->SetDesiredSizeOverride(FVector2D(18.f, 18.f));
			UHorizontalBoxSlot* IcoSlot = ScoutBox->AddChildToHorizontalBox(ScoutIco);
			IcoSlot->SetPadding(FMargin(8.f, 2.f, 5.f, 2.f));
			IcoSlot->SetVerticalAlignment(VAlign_Center);
			IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			UTextBlock* ScoutText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ScoutText"));
			ScoutText->SetText(FText::FromString(TEXT("إرسال كشافة (Scout)")));
			ScoutText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			URok2Typography::ApplyFont(ScoutText, ERok2TextRole::Button);
			ScoutBox->AddChildToHorizontalBox(ScoutText)->SetVerticalAlignment(VAlign_Center);
		}
			ScoutButton->OnClicked.AddDynamic(this, &URok2MarchPanel::OnScoutClicked);
			URok2MotionLibrary::BindPress(ScoutButton);	// P6-T3: ضغطة محسوسة

			const bool bRallyTarget = TargetType == TEXT("pass") || TargetType == TEXT("throne");
			if (bRallyTarget)
			{
				RallyButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RallyButton"));
				RallyButton->WidgetStyle.Normal.TintColor = FSlateColor(FLinearColor(0.72f, 0.42f, 0.10f));
				UVerticalBoxSlot* RallySlot = VBox->AddChildToVerticalBox(RallyButton);
				RallySlot->SetPadding(FMargin(20.f, 0.f, 20.f, 10.f));
				UHorizontalBox* RallyBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
				RallyButton->AddChild(RallyBox);
				UImage* RallyIco = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
				RallyIco->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("flag"), 18.f, FLinearColor::White));
				RallyIco->SetDesiredSizeOverride(FVector2D(18.f, 18.f));
				RallyBox->AddChildToHorizontalBox(RallyIco)->SetVerticalAlignment(VAlign_Center);
				UTextBlock* RallyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RallyText"));
				RallyText->SetText(FText::FromString(TEXT("بدء رالي التحالف")));
				RallyText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
				URok2Typography::ApplyFont(RallyText, ERok2TextRole::Button);
				RallyBox->AddChildToHorizontalBox(RallyText)->SetVerticalAlignment(VAlign_Center);
				RallyButton->OnClicked.AddDynamic(this, &URok2MarchPanel::OnRallyClicked);
				URok2MotionLibrary::BindPress(RallyButton);
			}

				RedirectButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RedirectButton"));
				RedirectButton->WidgetStyle.Normal.TintColor = FSlateColor(FLinearColor(0.15f, 0.47f, 0.72f));
				UVerticalBoxSlot* RedirectButtonSlot = VBox->AddChildToVerticalBox(RedirectButton);
				RedirectButtonSlot->SetPadding(FMargin(20.f, 0.f, 20.f, 8.f));
				UTextBlock* RedirectText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RedirectText"));
				RedirectText->SetText(FText::FromString(TEXT("تحويل المسيرة المحددة إلى هذا الهدف")));
				RedirectText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
				URok2Typography::ApplyFont(RedirectText, ERok2TextRole::Button);
				RedirectButton->AddChild(RedirectText);

				DispatchButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DispatchButton"));
		DispatchButton->WidgetStyle.Normal.TintColor = FSlateColor(FLinearColor(0.8f, 0.2f, 0.2f));
		UVerticalBoxSlot* BtnSlot = VBox->AddChildToVerticalBox(DispatchButton);
		BtnSlot->SetPadding(FMargin(20.f, 10.f, 20.f, 20.f));
		BtnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

		// P6-T1: أيقونة سيف إجرائية بجانب نص الإرسال
		{
			UHorizontalBox* DispatchBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			DispatchButton->AddChild(DispatchBox);
			UImage* DispatchIco = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			DispatchIco->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("sword"), 20.f, FLinearColor::White));
			DispatchIco->SetDesiredSizeOverride(FVector2D(20.f, 20.f));
			UHorizontalBoxSlot* IcoSlot = DispatchBox->AddChildToHorizontalBox(DispatchIco);
			IcoSlot->SetPadding(FMargin(8.f, 2.f, 5.f, 2.f));
			IcoSlot->SetVerticalAlignment(VAlign_Center);
			IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			UTextBlock* BtnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BtnText"));
			BtnText->SetText(FText::FromString(TEXT("إرسال المسيرة (Dispatch March)")));
			BtnText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			URok2Typography::ApplyFont(BtnText, ERok2TextRole::Button);
			DispatchBox->AddChildToHorizontalBox(BtnText)->SetVerticalAlignment(VAlign_Center);
		}

			ARok2WorldRenderer* WorldRenderer = Cast<ARok2WorldRenderer>(UGameplayStatics::GetActorOfClass(GetWorld(), ARok2WorldRenderer::StaticClass()));
			const bool bCanDispatch = WorldRenderer && WorldRenderer->CanInteractWithWorldTarget(TargetType, true);
				DispatchButton->SetIsEnabled(bCanDispatch);
				ScoutButton->SetIsEnabled(bCanDispatch);
					if (RallyButton)
					{
						RallyButton->SetIsEnabled(bCanDispatch && Api && !Api->GetPlayer().AllianceId.IsEmpty());
					}
					if (RedirectButton)
					{
						RedirectButton->SetIsEnabled(bCanDispatch && RedirectOptionIds.Num() > 0);
						RedirectButton->OnClicked.AddDynamic(this, &URok2MarchPanel::OnRedirectClicked);
						URok2MotionLibrary::BindPress(RedirectButton);
					}
					DispatchButton->OnClicked.AddDynamic(this, &URok2MarchPanel::OnDispatchClicked);
			URok2MotionLibrary::BindPress(DispatchButton);	// P6-T3: ضغطة محسوسة

		// P6-T3: لوحة المسيرة تفتح من المركز (المعيار الموحد 0.25s)
		URok2MotionLibrary::PlayScaleInCenter(MainBorder);
	}
}

void URok2MarchPanel::OnDispatchClicked()
{
	ARok2WorldRenderer* WorldRenderer = Cast<ARok2WorldRenderer>(UGameplayStatics::GetActorOfClass(GetWorld(), ARok2WorldRenderer::StaticClass()));
	if (!Api || !WorldRenderer || !WorldRenderer->CanInteractWithWorldTarget(TargetType, true)) return;

	TMap<FString, int32> TroopsMap;
	if (InfantrySpinBox && InfantrySpinBox->GetValue() > 0)
	{
		TroopsMap.Add(TEXT("infantry_t1"), FMath::FloorToInt(InfantrySpinBox->GetValue()));
	}
	if (CavalrySpinBox && CavalrySpinBox->GetValue() > 0)
	{
		TroopsMap.Add(TEXT("cavalry_t1"), FMath::FloorToInt(CavalrySpinBox->GetValue()));
	}
	if (ArcherSpinBox && ArcherSpinBox->GetValue() > 0)
	{
		TroopsMap.Add(TEXT("archer_t1"), FMath::FloorToInt(ArcherSpinBox->GetValue()));
	}

	if (TargetType == TEXT("pass"))
	{
		Api->AttackPass(TargetId, TroopsMap);
	}
	else
	{
		FString PrimaryCmd = PrimaryCommanderBox ? PrimaryCommanderBox->GetSelectedOption() : TEXT("");
		FString SecondaryCmd = SecondaryCommanderBox ? SecondaryCommanderBox->GetSelectedOption() : TEXT("");
		Api->DispatchMarch(TargetType, TargetId, TroopsMap, PrimaryCmd, SecondaryCmd);
	}

	// P6-T3: تسريح بتلاشٍ ثم إزالة (لا قفزة مفاجئة) — §1 «لا قفزات جامدة»
	URok2MotionLibrary::PlayFadeOut(this);
}

void URok2MarchPanel::OnRedirectClicked()
{
	ARok2WorldRenderer* WorldRenderer = Cast<ARok2WorldRenderer>(UGameplayStatics::GetActorOfClass(GetWorld(), ARok2WorldRenderer::StaticClass()));
	if (!Api || !RedirectMarchBox || !WorldRenderer || !WorldRenderer->CanInteractWithWorldTarget(TargetType, true)) return;

	const FString SelectedOption = RedirectMarchBox->GetSelectedOption();
	const FString* MarchId = RedirectOptionIds.Find(SelectedOption);
	if (!MarchId || MarchId->IsEmpty()) return;

	Api->RedirectMarch(*MarchId, TargetType, TargetId, ToX, ToY);
	URok2MotionLibrary::PlayFadeOut(this);
}

void URok2MarchPanel::OnRallyClicked()
{
	ARok2WorldRenderer* WorldRenderer = Cast<ARok2WorldRenderer>(UGameplayStatics::GetActorOfClass(GetWorld(), ARok2WorldRenderer::StaticClass()));
	if (!Api || !WorldRenderer || !WorldRenderer->CanInteractWithWorldTarget(TargetType, true)) return;
	if (Api->GetPlayer().AllianceId.IsEmpty())
	{
		return;
	}

	TMap<FString, int32> TroopsMap;
	if (InfantrySpinBox && InfantrySpinBox->GetValue() > 0) TroopsMap.Add(TEXT("infantry_t1"), FMath::FloorToInt(InfantrySpinBox->GetValue()));
	if (CavalrySpinBox && CavalrySpinBox->GetValue() > 0) TroopsMap.Add(TEXT("cavalry_t1"), FMath::FloorToInt(CavalrySpinBox->GetValue()));
	if (ArcherSpinBox && ArcherSpinBox->GetValue() > 0) TroopsMap.Add(TEXT("archer_t1"), FMath::FloorToInt(ArcherSpinBox->GetValue()));
	const FString PrimaryCmd = PrimaryCommanderBox ? PrimaryCommanderBox->GetSelectedOption() : TEXT("");
	Api->LaunchAllianceRally(TargetType, TargetId, TroopsMap, PrimaryCmd);
	URok2MotionLibrary::PlayFadeOut(this);
}

void URok2MarchPanel::OnScoutClicked()
{
	ARok2WorldRenderer* WorldRenderer = Cast<ARok2WorldRenderer>(UGameplayStatics::GetActorOfClass(GetWorld(), ARok2WorldRenderer::StaticClass()));
	if (!Api || !WorldRenderer || !WorldRenderer->CanInteractWithWorldTarget(TargetType, true)) return;

	// P5-T5: إرسال كشافة للنقطة المحددة (بدون قوات)
	Api->SendScout(ToX, ToY);
	// P6-T3: تسريح بتلاشٍ ثم إزالة (لا قفزة مفاجئة) — §1 «لا قفزات جامدة»
	URok2MotionLibrary::PlayFadeOut(this);
}
