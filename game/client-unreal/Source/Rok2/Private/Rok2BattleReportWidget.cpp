// Copyright ROK2. Battle Report Widget.

#include "Rok2BattleReportWidget.h"

void URok2BattleReportWidget::UpdateReport(bool bVictory, int32 Losses, int32 Remaining)
{
	OnReportUpdated(bVictory, Losses, Remaining);
}
