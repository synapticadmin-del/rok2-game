// Copyright ROK2. عقد «طبقة قابلة للتسريح» (P18-T5) — implementation.

#include "Rok2DismissibleLayer.h"
#include "Blueprint/UserWidget.h"

bool IRok2DismissibleLayer::IsLayerOpen() const
{
	// «مفتوحة» = في المنفذ وغير مطويّة. اللوحات التي يملكها `ARok2GameMode`
	// تُنشأ مرة وتبقى محفوظة، فوجود الكائن لا يعني أن اللاعب يراه.
	const UUserWidget* AsWidget = Cast<UUserWidget>(_getUObject());
	if (!AsWidget) return false;

	return AsWidget->IsInViewport()
		&& AsWidget->GetVisibility() != ESlateVisibility::Collapsed
		&& AsWidget->GetVisibility() != ESlateVisibility::Hidden;
}
