// Copyright ROK2.

#include "Rok2MarchPanel.h"
#include "Rok2Api.h"
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
		BorderSlot->SetSize(FVector2D(400.f, 500.f));

		UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VBox"));
		MainBorder->AddChild(VBox);

		// Title
		TargetNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TargetNameText"));
		TargetNameText->SetText(FText::FromString(TargetName.IsEmpty() ? TargetType : TargetName));
		TargetNameText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.8f, 0.2f)));
		TargetNameText->Font.Size = 24;
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
		DistSlot->SetPadding(FMargin(20.f, 0.f, 20.f, 20.f));
		DistSlot->SetHorizontalAlignment(HAlign_Center);

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

		USpacer* Spacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("Spacer"));
		UVerticalBoxSlot* SpacerSlot = VBox->AddChildToVerticalBox(Spacer);
		SpacerSlot->Size.SizeRule = ESlateSizeRule::Fill;

		DispatchButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DispatchButton"));
		DispatchButton->WidgetStyle.Normal.TintColor = FSlateColor(FLinearColor(0.8f, 0.2f, 0.2f));
		UVerticalBoxSlot* BtnSlot = VBox->AddChildToVerticalBox(DispatchButton);
		BtnSlot->SetPadding(FMargin(20.f, 20.f, 20.f, 20.f));
		BtnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

		UTextBlock* BtnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BtnText"));
		BtnText->SetText(FText::FromString(TEXT("⚔️ إرسال المسيرة (Dispatch March)")));
		BtnText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		BtnText->Font.Size = 18;
		DispatchButton->AddChild(BtnText);

		DispatchButton->OnClicked.AddDynamic(this, &URok2MarchPanel::OnDispatchClicked);
	}
}

void URok2MarchPanel::OnDispatchClicked()
{
	if (!Api) return;

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
		Api->AttackPass(TargetId, TroopsMap); // Pass doesn't take commanders in current API maybe, or we update it?
	}
	else
	{
		FString PrimaryCmd = PrimaryCommanderBox ? PrimaryCommanderBox->GetSelectedOption() : TEXT("");
		FString SecondaryCmd = SecondaryCommanderBox ? SecondaryCommanderBox->GetSelectedOption() : TEXT("");
		Api->DispatchMarch(TargetType, TargetId, TroopsMap, PrimaryCmd, SecondaryCmd);
	}

	RemoveFromParent();
}
