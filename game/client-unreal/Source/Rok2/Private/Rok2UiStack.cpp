// Copyright ROK2. حلّال طبقات الواجهة لزر الرجوع (P18-T5) — implementation.

#include "Rok2UiStack.h"
#include "Rok2DismissibleLayer.h"
#include "Rok2BootWidget.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/World.h"

namespace
{
	/**
	 * الطبقات المفتوحة مرتّبة تنازلياً بـZOrder. تُقرأ من المنفذ نفسه لا من
	 * سِجل محلي، فلا يمكن أن تتقادم.
	 */
	void CollectOpenLayers(UObject* WorldContextObject, TArray<UUserWidget*>& OutLayers)
	{
		OutLayers.Reset();
		if (!WorldContextObject) return;

		UWorld* World = WorldContextObject->GetWorld();
		if (!World) return;

		UGameViewportSubsystem* Viewport = UGameViewportSubsystem::Get(World);
		if (!Viewport) return;

		TArray<UUserWidget*> Found;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Found, UUserWidget::StaticClass(), /*TopLevelOnly=*/true);

		// ZOrder لكل مرشّح، لأن الترتيب لا يُقرأ من الودجة نفسها.
		TMap<UUserWidget*, int32> Orders;
		for (UUserWidget* Widget : Found)
		{
			if (!Widget) continue;

			// شاشة الدخول ليست طبقة تُغلق: إغلاقها يترك اللاعب بلا مسار عودة.
			if (Widget->IsA<URok2BootWidget>()) continue;

			IRok2DismissibleLayer* Layer = Cast<IRok2DismissibleLayer>(Widget);
			if (!Layer || !Layer->IsLayerOpen()) continue;

			const FGameViewportWidgetSlot Slot = Viewport->GetWidgetSlot(Widget);
			if (Slot.ZOrder <= URok2UiStack::HudZOrder) continue;

			Orders.Add(Widget, Slot.ZOrder);
			OutLayers.Add(Widget);
		}

		// تنازلياً: الأعلى ترتيباً هو ما يراه اللاعب فوق الكل.
		OutLayers.Sort([&Orders](const UUserWidget& A, const UUserWidget& B)
		{
			const int32 OrderA = Orders.FindRef(const_cast<UUserWidget*>(&A));
			const int32 OrderB = Orders.FindRef(const_cast<UUserWidget*>(&B));
			return OrderA > OrderB;
		});
	}
}

UUserWidget* URok2UiStack::FindTopDismissibleLayer(UObject* WorldContextObject)
{
	TArray<UUserWidget*> Layers;
	CollectOpenLayers(WorldContextObject, Layers);
	return Layers.Num() > 0 ? Layers[0] : nullptr;
}

bool URok2UiStack::DismissTopLayer(UObject* WorldContextObject)
{
	UUserWidget* Top = FindTopDismissibleLayer(WorldContextObject);
	if (!Top) return false;

	IRok2DismissibleLayer* Layer = Cast<IRok2DismissibleLayer>(Top);
	if (!Layer) return false;

	Layer->DismissLayer();
	return true;
}

int32 URok2UiStack::CountOpenLayers(UObject* WorldContextObject)
{
	TArray<UUserWidget*> Layers;
	CollectOpenLayers(WorldContextObject, Layers);
	return Layers.Num();
}
