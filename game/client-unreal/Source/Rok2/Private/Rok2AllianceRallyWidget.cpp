// Copyright ROK2.

#include "Rok2AllianceRallyWidget.h"
#include "Rok2Api.h"
#include "Rok2Typography.h"
#include "Rok2MotionLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void URok2AllianceRallyWidget::Setup(URok2Api* InApi, const FRok2AllianceRally& InRally)
{
	Api = InApi;
	Rally = InRally;
	if (WidgetTree && WidgetTree->RootWidget)
	{
		RefreshDisplay();
	}
}

void URok2AllianceRallyWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildCard();
	RefreshDisplay();
}

void URok2AllianceRallyWidget::BuildCard()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}
	if (WidgetTree->RootWidget) return;

	CardBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RallyCard"));
	CardBorder->SetPadding(FMargin(12.f));
	WidgetTree->RootWidget = CardBorder;

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RallyVBox"));
	CardBorder->AddChild(Box);

	TargetText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TargetText"));
	TargetText->SetText(FText::FromString(FString::Printf(TEXT("رالي %s: %s"), *Rally.TargetType.ToUpper(), *Rally.TargetId)));
	TargetText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.78f, 0.24f)));
	URok2Typography::ApplyFont(TargetText, ERok2TextRole::Heading);
	Box->AddChildToVerticalBox(TargetText)->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	URok2Typography::ApplyFont(StatusText, ERok2TextRole::Body);
	Box->AddChildToVerticalBox(StatusText)->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));

	CountdownText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CountdownText"));
	CountdownText->SetColorAndOpacity(FSlateColor(FLinearColor(0.80f, 0.88f, 0.96f)));
	URok2Typography::ApplyFont(CountdownText, ERok2TextRole::Body);
	Box->AddChildToVerticalBox(CountdownText)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));

	JoinButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("JoinRallyButton"));
	JoinButton->WidgetStyle.Normal.TintColor = FSlateColor(FLinearColor(0.18f, 0.55f, 0.30f));
	UTextBlock* JoinText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("JoinText"));
	JoinText->SetText(FText::FromString(TEXT("الانضمام بقوات منزلية")));
	JoinText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	URok2Typography::ApplyFont(JoinText, ERok2TextRole::Button);
	JoinButton->AddChild(JoinText);
	JoinButton->OnClicked.AddDynamic(this, &URok2AllianceRallyWidget::OnJoinClicked);
	URok2MotionLibrary::BindPress(JoinButton);
	Box->AddChildToVerticalBox(JoinButton);
}

void URok2AllianceRallyWidget::RefreshDisplay()
{
	if (!TargetText || !StatusText || !CountdownText || !JoinButton) return;
	TargetText->SetText(FText::FromString(FString::Printf(TEXT("رالي %s: %s"), *Rally.TargetType.ToUpper(), *Rally.TargetId)));
	const bool bForming = Rally.Status == TEXT("forming");
	const bool bCanJoin = bForming && !Rally.bIsJoined && Api && !BuildHomeContribution().IsEmpty();
	const FString State = Rally.bIsJoined
		? TEXT("تم حجز قواتك في الرالي")
		: (bForming ? FString::Printf(TEXT("قيد التجميع · المشاركون: %d"), Rally.Participants) : TEXT("انطلق الرالي"));
	StatusText->SetText(FText::FromString(State));
	StatusText->SetColorAndOpacity(FSlateColor(Rally.bIsJoined ? FLinearColor(0.35f, 0.95f, 0.55f) : (bForming ? FLinearColor(0.95f, 0.78f, 0.30f) : FLinearColor(0.65f, 0.75f, 0.90f))));
	JoinButton->SetIsEnabled(bCanJoin);
	if (CardBorder)
	{
		CardBorder->SetBrushColor(bForming ? FLinearColor(0.10f, 0.075f, 0.035f, 0.96f) : FLinearColor(0.06f, 0.10f, 0.15f, 0.96f));
	}
}

void URok2AllianceRallyWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!CountdownText) return;
	if (Rally.Status != TEXT("forming"))
	{
		CountdownText->SetText(FText::FromString(TEXT("المسيرة الموحدة في الطريق")));
		return;
	}
	const int64 NowMs = FDateTime::UtcNow().ToUnixTimestamp() * 1000LL;
	const int64 Remaining = FMath::Max<int64>(0, Rally.LaunchMs - NowMs);
	const int64 Seconds = Remaining / 1000LL;
	CountdownText->SetText(FText::FromString(FString::Printf(TEXT("الإطلاق خلال %02lld:%02lld"), Seconds / 60LL, Seconds % 60LL)));
}

TMap<FString, int32> URok2AllianceRallyWidget::BuildHomeContribution() const
{
	TMap<FString, int32> Result;
	if (!Api) return Result;
	for (const FRok2TroopEntry& Troop : Api->GetTroops())
	{
		if (Troop.Count > 0)
		{
			// مساهمة افتراضية تحفظ غالبية الحامية؛ يبقى الخادم صاحب القرار الفعلي.
			Result.Add(Troop.UnitId, FMath::Clamp(FMath::FloorToInt(Troop.Count * 0.20f), 1, 500));
		}
	}
	return Result;
}

void URok2AllianceRallyWidget::OnJoinClicked()
{
	if (!Api || Rally.Id.IsEmpty() || Rally.Status != TEXT("forming") || Rally.bIsJoined) return;
	const TMap<FString, int32> Troops = BuildHomeContribution();
	if (Troops.IsEmpty()) return;
	JoinButton->SetIsEnabled(false);
	Api->JoinAllianceRally(Rally.Id, Troops);
}
