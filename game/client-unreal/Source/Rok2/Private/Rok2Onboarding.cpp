// Copyright ROK2. First-minute onboarding — FTUE model (P6-T4).

#include "Rok2Onboarding.h"
#include "Rok2Api.h"
#include "Components/Widget.h"

DEFINE_LOG_CATEGORY_STATIC(LogRok2Ftue, Log, All);

// التعريف الوحيد لثوابت المواصفة. الهيدر يعلنها extern فتكتسب هذه التعريفات
// ربطاً خارجياً — نسخة واحدة في الوحدة كلها لا واحدة لكل وحدة ترجمة.
namespace Rok2FtueSpec
{
	const FName AnchorBuild(TEXT("Ftue.Build"));
	const FName AnchorTrain(TEXT("Ftue.Train"));
	const FName AnchorMap(TEXT("Ftue.Map"));

	const FString FarmBuildingId(TEXT("farm"));
	const FString QueueTypeBuilding(TEXT("building"));
	const FString QueueTypeTraining(TEXT("training"));
}

// ---------------------------------------------------------------------------
// النصوص — نَفَس قصصي قصير، على هوية «المخطوطة الملكية» (ui §1) وتقنيات
// theme-and-values §5 (لحظات تتويج) و§6 (حكاية في الواجهة).
//
// كل خطوة سطران بدورين مختلفين: Story يُلهم، وAction يُرشد. الفصل مقصود —
// جملة أدبية وحدها تترك اللاعب لا يعرف أين يضغط، وتعليمة جافة وحدها تُلغي
// الطابع. فالبطاقة تحمل الاثنين بوزنين بصريين مختلفين.
// ---------------------------------------------------------------------------
namespace
{
	FRok2FtueStepInfo MakeInfo(
		ERok2FtueStep Step,
		int32 Ordinal,
		const FString& Title,
		const FString& Story,
		const FString& Action,
		const FString& IconId,
		FName AnchorId)
	{
		FRok2FtueStepInfo Info;
		Info.Step = Step;
		Info.Ordinal = Ordinal;
		Info.Title = Title;
		Info.Story = Story;
		Info.Action = Action;
		Info.IconId = IconId;
		Info.AnchorId = AnchorId;
		return Info;
	}
}

URok2Onboarding* URok2Onboarding::Get()
{
	static URok2Onboarding* Instance = nullptr;
	if (!Instance || !IsValid(Instance))
	{
		Instance = NewObject<URok2Onboarding>();
		Instance->AddToRoot();
	}
	return Instance;
}

int32 URok2Onboarding::OrdinalOf(ERok2FtueStep Step)
{
	// الترتيب هو قيمة التعداد نفسها — التعداد مُرقَّم صراحةً لهذا السبب،
	// فالمِزلاج يقارن أعداداً لا يفرّع على حالات.
	return (int32)Step;
}

TArray<ERok2FtueStep> URok2Onboarding::GuidedSteps()
{
	return { ERok2FtueStep::BuildFarm, ERok2FtueStep::TrainTroops, ERok2FtueStep::GatherMarch };
}

FRok2FtueStepInfo URok2Onboarding::StepInfo(ERok2FtueStep Step)
{
	switch (Step)
	{
	case ERok2FtueStep::BuildFarm:
		return MakeInfo(
			Step, 1,
			TEXT("رغيفٌ قبل السيف"),
			TEXT("لا تُبنى مملكةٌ على جوع. ابدأ بحقلٍ يُطعم من سيحملون رايتك."),
			TEXT("المس المطرقة، ثم اختر «المزرعة»."),
			TEXT("farm"),
			Rok2FtueSpec::AnchorBuild);

	case ERok2FtueStep::TrainTroops:
		return MakeInfo(
			Step, 2,
			TEXT("أول من يحمل الراية"),
			TEXT("صارت لك حقولٌ، وما لها حارس. ادعُ أول رجالك إلى الميدان."),
			TEXT("من لوحة المدينة، درّب حرساً من المشاة."),
			TEXT("helmet"),
			Rok2FtueSpec::AnchorTrain);

	case ERok2FtueStep::GatherMarch:
		return MakeInfo(
			Step, 3,
			TEXT("أول الخُطى خارج السور"),
			TEXT("ما في الأرض أوسع من مخزنك. أرسل من يجمع لك خيرها ويعود."),
			TEXT("افتح الخريطة، والمس عقدة موارد ثم أطلق المسيرة."),
			TEXT("march"),
			Rok2FtueSpec::AnchorMap);

	case ERok2FtueStep::Done:
		return CompletionInfo();

	case ERok2FtueStep::None:
	default:
		return FRok2FtueStepInfo();
	}
}

FRok2FtueStepInfo URok2Onboarding::CompletionInfo()
{
	// لحظة تتويج لا مجرد إخفاء بطاقة — theme-and-values §5: «لحظات لازم
	// تتحوّل لـ screenshot». Ordinal يساوي عدد الخطوات فتقرأ اللافتة «٣ من ٣».
	return MakeInfo(
		ERok2FtueStep::Done,
		Rok2FtueSpec::GuidedStepCount,
		TEXT("تمّت الدقيقة الأولى"),
		TEXT("حقلٌ يُطعم، وحرسٌ يحمي، وطريقٌ يعود بالخير. ما بعدها مملكةٌ تُبنى بيدك."),
		TEXT("امضِ — المملكة لك."),
		TEXT("crown"),
		NAME_None);
}

// ---------------------------------------------------------------------------
// المُسنَدات
// ---------------------------------------------------------------------------

bool URok2Onboarding::IsStateReady(const URok2Api* Api)
{
	if (!Api) return false;
	// UpdatedAt يملؤه ParseCity من updated_at (Rok2Api.cpp) — فصفره يعني أن
	// حمولة مدينة حقيقية لم تصل بعد. HallLevel لا يصلح دليلاً: قيمته
	// الافتراضية 1 فمدينة فارغة تُشبه مدينةً حقيقية بمستوى 1.
	return Api->HasPlayer() && Api->GetCity().UpdatedAt > 0;
}

bool URok2Onboarding::HasFarm(const URok2Api* Api)
{
	if (!Api) return false;

	// مبنية
	if (const int32* Level = Api->GetBuildings().Find(Rok2FtueSpec::FarmBuildingId))
	{
		if (*Level > 0) return true;
	}

	// أو في الطابور — الطابور يُحتسب إتماماً (انظر تعليق الهيدر)
	for (const FRok2QueueEntry& Q : Api->GetCity().ActiveQueues)
	{
		if (Q.Type == Rok2FtueSpec::QueueTypeBuilding && Q.RefId == Rok2FtueSpec::FarmBuildingId)
		{
			return true;
		}
	}
	return false;
}

bool URok2Onboarding::HasTroops(const URok2Api* Api)
{
	if (!Api) return false;

	for (const FRok2TroopEntry& T : Api->GetTroops())
	{
		if (T.Count > 0) return true;
	}

	// أي تدريب في الطابور — لا نقيّد بوحدة بعينها لأن اللاعب قد يدرّب
	// فارساً أو رامياً بدل المشاة، وقد أدّى الإجراء المطلوب فعلاً.
	for (const FRok2QueueEntry& Q : Api->GetCity().ActiveQueues)
	{
		if (Q.Type == Rok2FtueSpec::QueueTypeTraining)
		{
			return true;
		}
	}
	return false;
}

bool URok2Onboarding::HasOwnMarch(const URok2Api* Api)
{
	if (!Api) return false;

	const FString& MyId = Api->GetPlayer().Id;
	if (MyId.IsEmpty()) return false;

	// أي مسيرة للاعب تُحتسب، لا مسيرات الجمع وحدها. المواصفة تقول «أول مسيرة
	// جمع» والبطاقة ترشد إلى عقدة موارد فعلاً — لكن لو أطلق اللاعب مسيرةً على
	// برابرة بدلاً منها فقد تعلّم الآلية، وحصرُها على node كان سيتركه عالقاً
	// في بطاقةٍ لا تُغلق. الهدف التربوي: أن يُطلق مسيرته الأولى.
	for (const FRok2MarchEntity& M : Api->GetWorldSnapshot().Marches)
	{
		if (M.OwnerPlayerId == MyId) return true;
	}
	return false;
}

ERok2FtueStep URok2Onboarding::DeriveRawStep(const URok2Api* Api)
{
	if (!IsStateReady(Api)) return ERok2FtueStep::None;

	if (!HasFarm(Api))     return ERok2FtueStep::BuildFarm;
	if (!HasTroops(Api))   return ERok2FtueStep::TrainTroops;
	if (!HasOwnMarch(Api)) return ERok2FtueStep::GatherMarch;

	return ERok2FtueStep::Done;
}

// ---------------------------------------------------------------------------
// المِزلاج
// ---------------------------------------------------------------------------

ERok2FtueStep URok2Onboarding::Evaluate(const URok2Api* Api)
{
	// لا نقرّر شيئاً قبل وصول حالة حقيقية — وهذا هو الفخّ الأهم: التقييم على
	// مدينة فارغة يُسلّح الإرشاد لمحاربٍ قديم.
	if (!IsStateReady(Api))
	{
		return CurrentStep;
	}

	const ERok2FtueStep Raw = DeriveRawStep(Api);

	if (!bEvaluatedOnce)
	{
		bEvaluatedOnce = true;
		// التسليح يحدث مرة واحدة فقط: من كان قد أنجز الثلاثة سلفاً لاعبٌ عائد،
		// فلا يُسلَّح أبداً ولا تُبنى له الطبقة.
		bArmed = (Raw != ERok2FtueStep::Done);
		CurrentStep = bArmed ? Raw : ERok2FtueStep::Done;

		UE_LOG(LogRok2Ftue, Log, TEXT("FTUE first evaluation: raw=%d armed=%d"),
			(int32)Raw, bArmed ? 1 : 0);
		return CurrentStep;
	}

	// لاعب عائد: Done نهائية، لا شيء يعيد الإرشاد.
	if (!bArmed)
	{
		return ERok2FtueStep::Done;
	}

	// تقدّم للأمام فقط — هذا هو ما يمنع الارتداد حين يفقد اللاعب جنوده
	// في معركة فيعود المُسنَد الخام إلى TrainTroops.
	if (OrdinalOf(Raw) > OrdinalOf(CurrentStep))
	{
		CurrentStep = Raw;
	}
	return CurrentStep;
}

bool URok2Onboarding::IsShowingGuidance() const
{
	return bArmed
		&& CurrentStep != ERok2FtueStep::None
		&& CurrentStep != ERok2FtueStep::Done;
}

void URok2Onboarding::Reset()
{
	CurrentStep = ERok2FtueStep::None;
	bEvaluatedOnce = false;
	bArmed = false;
	// المراسي لا تُمسح: الأزرار نفسها لم تختفِ، وإعادة تسجيلها مسؤولية
	// الودجات عند إعادة بنائها.
}

// ---------------------------------------------------------------------------
// سجلّ المراسي
// ---------------------------------------------------------------------------

void URok2Onboarding::RegisterAnchor(FName AnchorId, UWidget* Widget)
{
	if (AnchorId.IsNone() || !Widget) return;
	Anchors.Add(AnchorId, Widget);
}

void URok2Onboarding::ClearAnchor(FName AnchorId)
{
	Anchors.Remove(AnchorId);
}

UWidget* URok2Onboarding::ResolveAnchor(FName AnchorId) const
{
	if (AnchorId.IsNone()) return nullptr;
	if (const TWeakObjectPtr<UWidget>* Found = Anchors.Find(AnchorId))
	{
		// Get() على TWeakObjectPtr يعيد nullptr تلقائياً إن جُمِعت الودجة —
		// وهو سبب اختيار المؤشر الضعيف: شبكات الـHUD تُعاد بناؤها.
		return Found->Get();
	}
	return nullptr;
}

UWidget* URok2Onboarding::ResolveCurrentAnchor() const
{
	if (!IsShowingGuidance()) return nullptr;
	return ResolveAnchor(StepInfo(CurrentStep).AnchorId);
}
