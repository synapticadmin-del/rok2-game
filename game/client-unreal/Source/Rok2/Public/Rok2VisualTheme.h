#pragma once

#include "CoreMinimal.h"

/**
 * رموز ألوان ROK2 المشتركة. تستعملها واجهات UMG/Slate ولا تحمل منطق لعبة.
 *
 * المصدر التصميمي: design/01-visual/VISUAL_IDENTITY.md.
 * لا تستعمل لوناً وحده للدلالة على حالة؛ اجمعه دائماً مع رمز ونص وحالة تفاعل.
 */
namespace Rok2Visual
{
	ROK2_API const FLinearColor& Ink();
	ROK2_API const FLinearColor& Panel();
	ROK2_API const FLinearColor& Card();
	ROK2_API const FLinearColor& Gold();
	ROK2_API const FLinearColor& Ivory();
	ROK2_API const FLinearColor& Muted();
	ROK2_API const FLinearColor& TabInactive();
	ROK2_API const FLinearColor& PrimaryAction();
	ROK2_API const FLinearColor& Success();
	ROK2_API const FLinearColor& Danger();
	ROK2_API const FLinearColor& Information();
	ROK2_API const FLinearColor& Scrim();
	ROK2_API FLinearColor CivilizationAccent(const FString& CivilizationId);
}
