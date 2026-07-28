#include "Rok2CommanderWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"

#include "Rok2Api.h"

void URok2CommanderWidget::NativeConstruct()
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

        // Commander List
        UVerticalBox* CommanderList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CommanderList"));
        MainBox->AddChildToVerticalBox(CommanderList);

        // Sample Commander Item
        UHorizontalBox* CommanderItem = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CommanderItem"));
        CommanderList->AddChildToVerticalBox(CommanderItem);

        UImage* Portrait = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Portrait"));
        CommanderItem->AddChildToHorizontalBox(Portrait);

        UTextBlock* CommanderInfo = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CommanderInfo"));
        CommanderInfo->SetText(FText::FromString(TEXT("Name: Julius Caesar\nLevel: 10\nStars: 3\nSkills: 5/1/1/1")));
        CommanderItem->AddChildToHorizontalBox(CommanderInfo);

        // Buttons Box
        UVerticalBox* ActionBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ActionBox"));
        CommanderItem->AddChildToHorizontalBox(ActionBox);

        // Level Up Button
        UButton* LevelUpButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("LevelUpButton"));
        UTextBlock* LevelUpText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LevelUpText"));
        LevelUpText->SetText(FText::FromString(TEXT("⭐ ترقية Level Up")));
        LevelUpButton->AddChild(LevelUpText);
        ActionBox->AddChildToVerticalBox(LevelUpButton);
        LevelUpButton->OnClicked.AddDynamic(this, &URok2CommanderWidget::OnLevelUpClicked);

        // Upgrade Skill Button
        UButton* UpgradeSkillButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("UpgradeSkillButton"));
        UTextBlock* UpgradeSkillText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("UpgradeSkillText"));
        UpgradeSkillText->SetText(FText::FromString(TEXT("⚡ ترقية المهارة Upgrade Skill")));
        UpgradeSkillButton->AddChild(UpgradeSkillText);
        ActionBox->AddChildToVerticalBox(UpgradeSkillButton);
        UpgradeSkillButton->OnClicked.AddDynamic(this, &URok2CommanderWidget::OnUpgradeSkillClicked);

        CurrentCommanderId = TEXT("Cmdr_Caesar");
    }
}

void URok2CommanderWidget::OnLevelUpClicked()
{
    // if (Api)
    // {
    //     Api->LevelUpCommander(CurrentCommanderId);
    // }
}

void URok2CommanderWidget::OnUpgradeSkillClicked()
{
    // if (Api)
    // {
    //     // Api->UpgradeCommanderSkill(CurrentCommanderId);
    // }
}
