#include "Rok2ResearchWidget.h"
#include "Rok2ArtAssets.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"

#include "Rok2Api.h"

void URok2ResearchWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (!WidgetTree->RootWidget)
    {
        UCanvasPanel* RootPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootPanel"));
        WidgetTree->RootWidget = RootPanel;

        UVerticalBox* MainBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainBox"));
        UCanvasPanelSlot* MainBoxSlot = RootPanel->AddChildToCanvas(MainBox);
        MainBoxSlot->SetAnchors(FAnchors(0.1f, 0.1f, 0.9f, 0.9f));
        MainBoxSlot->SetOffsets(FMargin(0.0f));

        // Tabs
        UHorizontalBox* TabBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TabBox"));
        MainBox->AddChildToVerticalBox(TabBox);

        // Economy Tab
        UButton* EconomyTab = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("EconomyTab"));
        UTextBlock* EconomyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EconomyText"));
        EconomyText->SetText(FText::FromString(TEXT("Economy")));
        EconomyTab->AddChild(EconomyText);
        TabBox->AddChildToHorizontalBox(EconomyTab);

        // Military Tab
        UButton* MilitaryTab = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("MilitaryTab"));
        UTextBlock* MilitaryText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MilitaryText"));
        MilitaryText->SetText(FText::FromString(TEXT("Military")));
        MilitaryTab->AddChild(MilitaryText);
        TabBox->AddChildToHorizontalBox(MilitaryTab);

        // Defense Tab
        UButton* DefenseTab = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DefenseTab"));
        UTextBlock* DefenseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DefenseText"));
        DefenseText->SetText(FText::FromString(TEXT("Defense")));
        DefenseTab->AddChild(DefenseText);
        TabBox->AddChildToHorizontalBox(DefenseTab);

        // Tech List
        UVerticalBox* TechList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TechList"));
        MainBox->AddChildToVerticalBox(TechList);

        // Sample Tech Item
        UHorizontalBox* TechItem = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TechItem"));
        TechList->AddChildToVerticalBox(TechItem);

        UTextBlock* TechInfo = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TechInfo"));
        TechInfo->SetText(FText::FromString(TEXT("Tech: Architecture Lv.1\nReq: Academy Lv.5\nCost: 1000 Wood")));
        TechItem->AddChildToHorizontalBox(TechInfo);

        UButton* ResearchButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ResearchButton"));
        // P6-T1: زر بحث بأيقونة قارورة إجرائية + نص (بدل 🔬)
        {
            UHorizontalBox* BtnBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
            ResearchButton->AddChild(BtnBox);
            UImage* Ico = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
            Ico->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("flask"), 16.f, FLinearColor::White));
            Ico->SetDesiredSizeOverride(FVector2D(16.f, 16.f));
            UHorizontalBoxSlot* IcoSlot = BtnBox->AddChildToHorizontalBox(Ico);
            IcoSlot->SetPadding(FMargin(4, 0, 4, 0));
            IcoSlot->SetVerticalAlignment(VAlign_Center);
            IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            UTextBlock* ResearchButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResearchButtonText"));
            ResearchButtonText->SetText(FText::FromString(TEXT("بحث (Research)")));
            BtnBox->AddChildToHorizontalBox(ResearchButtonText)->SetVerticalAlignment(VAlign_Center);
        }
        TechItem->AddChildToHorizontalBox(ResearchButton);

        ResearchButton->OnClicked.AddDynamic(this, &URok2ResearchWidget::OnResearchButtonClicked);
        
        CurrentSelectedTechId = TEXT("Tech_Architecture");
    }
}

void URok2ResearchWidget::OnResearchButtonClicked()
{
    // if (Api)
    // {
    //     Api->StartResearch(CurrentSelectedTechId);
    // }
}
