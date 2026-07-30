// Copyright ROK2. Civilization narrative voice — lore registry (P6-T5).

#include "Rok2CivLore.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogRok2CivLore, Log, All);

// التعريف الوحيد لثابت المواصفة غير البدائي. الهيدر يعلنه extern لأن `const`
// FString عند نطاق النطاق في هيدر عام يعطي ربطاً داخلياً — فتُنشأ نسخة
// ومُهيِّئ ساكن في كل وحدة ترجمة (نفس علّة Rok2FtueSpec).
namespace Rok2CivLoreSpec
{
	const FString JsonRelativePath(TEXT("../../data/civilizations.json"));
}

URok2CivLore* URok2CivLore::Get()
{
	static URok2CivLore* Instance = nullptr;
	if (!Instance || !IsValid(Instance))
	{
		Instance = NewObject<URok2CivLore>();
		Instance->AddToRoot();
		Instance->LoadFromDiskOrDefaults();
	}
	return Instance;
}

// ---------------------------------------------------------------------------
// قراءة
// ---------------------------------------------------------------------------

const FRok2CivLore& URok2CivLore::GetLore(const FString& CivId) const
{
	for (const FRok2CivLore& E : Entries)
	{
		if (E.CivId == CivId) return E;
	}
	// سجلّ فارغ لا أول عنصر: عرض نصّ روما لحضارةٍ أخرى عطلٌ لا يُرى، والفراغ يُرى.
	return EmptyEntry;
}

bool URok2CivLore::HasLore(const FString& CivId) const
{
	return GetLore(CivId).IsValid();
}

TArray<FString> URok2CivLore::GetCivIds() const
{
	TArray<FString> Out;
	Out.Reserve(Entries.Num());
	for (const FRok2CivLore& E : Entries)
	{
		Out.Add(E.CivId);
	}
	return Out;
}

FString URok2CivLore::StoryText(const FString& CivId) const
{
	const FRok2CivLore& E = GetLore(CivId);
	// FString::Join على مصفوفة فارغة يعيد نصاً فارغاً — لا حاجة لحرس منفصل.
	return FString::Join(E.Story, TEXT("\n"));
}

FString URok2CivLore::HintAt(const FString& CivId, int32 Index) const
{
	const FRok2CivLore& E = GetLore(CivId);
	if (E.Hints.Num() <= 0) return FString();
	// لفّ دوري بفهرس غير سالب: المنادي قد يمرّر عدّاد جلسة تصاعدياً، و%
	// على سالب في C++ يعطي سالباً فيتعطّل الفهرسة.
	const int32 Wrapped = ((Index % E.Hints.Num()) + E.Hints.Num()) % E.Hints.Num();
	return E.Hints[Wrapped];
}

// ---------------------------------------------------------------------------
// تحليل
// ---------------------------------------------------------------------------

FRok2CivLore URok2CivLore::ParseFromJson(const TSharedPtr<FJsonObject>& Obj)
{
	FRok2CivLore E;
	if (!Obj.IsValid()) return E;

	Obj->TryGetStringField(TEXT("id"), E.CivId);
	Obj->TryGetStringField(TEXT("name"), E.NameLatin);
	Obj->TryGetStringField(TEXT("name_ar"), E.NameAr);
	Obj->TryGetStringField(TEXT("fantasy"), E.FantasyLatin);
	Obj->TryGetStringField(TEXT("fantasy_ar"), E.FantasyAr);
	Obj->TryGetStringField(TEXT("greeting"), E.Greeting);

	const TArray<TSharedPtr<FJsonValue>>* StoryArr = nullptr;
	if (Obj->TryGetArrayField(TEXT("story"), StoryArr) && StoryArr)
	{
		for (const TSharedPtr<FJsonValue>& V : *StoryArr)
		{
			FString Line;
			if (V.IsValid() && V->TryGetString(Line) && !Line.IsEmpty())
			{
				E.Story.Add(Line);
			}
		}
	}

	// special_unit كائن متشعّب — نأخذ المعرّف وحده، لا الفرع ولا مستوى الفتح
	// (بيانات توازن لا هوية).
	const TSharedPtr<FJsonObject>* UnitObj = nullptr;
	if (Obj->TryGetObjectField(TEXT("special_unit"), UnitObj) && UnitObj && UnitObj->IsValid())
	{
		(*UnitObj)->TryGetStringField(TEXT("id"), E.SpecialUnitId);
	}

	const TArray<TSharedPtr<FJsonValue>>* HintsArr = nullptr;
	if (Obj->TryGetArrayField(TEXT("hints"), HintsArr) && HintsArr)
	{
		for (const TSharedPtr<FJsonValue>& V : *HintsArr)
		{
			FString Hint;
			if (V.IsValid() && V->TryGetString(Hint) && !Hint.IsEmpty())
			{
				E.Hints.Add(Hint);
			}
		}
	}

	// الاسم العربي يسند إلى اللاتيني عند غيابه: بطاقة بلا اسم أسوأ من بطاقة
	// باسمٍ لاتيني، والحرس يمنع غيابه في البيانات المؤلَّفة أصلاً.
	if (E.NameAr.IsEmpty()) E.NameAr = E.NameLatin;

	return E;
}

bool URok2CivLore::IsCompleteEntry(const FRok2CivLore& Entry)
{
	if (Entry.CivId.IsEmpty()) return false;
	if (Entry.NameAr.IsEmpty()) return false;
	if (Entry.Greeting.IsEmpty()) return false;
	// المدى من المواصفة لا رقم مكرَّر — «(3-4 أسطر)» في نصّ البند.
	if (Entry.Story.Num() < Rok2CivLoreSpec::MinStoryLines) return false;
	if (Entry.Story.Num() > Rok2CivLoreSpec::MaxStoryLines) return false;
	return true;
}

bool URok2CivLore::AddEntry(const FRok2CivLore& Entry)
{
	if (!IsCompleteEntry(Entry))
	{
		UE_LOG(LogRok2CivLore, Warning, TEXT("Skipping incomplete civ lore entry: '%s' (story lines: %d)"),
			*Entry.CivId, Entry.Story.Num());
		return false;
	}
	for (const FRok2CivLore& E : Entries)
	{
		if (E.CivId == Entry.CivId) return false;	// لا تكرار
	}
	Entries.Add(Entry);
	return true;
}

int32 URok2CivLore::LoadFromJsonString(const FString& JsonString)
{
	TSharedPtr<FJsonObject> RootObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, RootObj) || !RootObj.IsValid())
	{
		return 0;
	}

	const TArray<TSharedPtr<FJsonValue>>* CivsArray = nullptr;
	if (!RootObj->TryGetArrayField(TEXT("civilizations"), CivsArray) || !CivsArray)
	{
		return 0;
	}

	int32 Added = 0;
	for (const TSharedPtr<FJsonValue>& V : *CivsArray)
	{
		const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
		if (V.IsValid() && V->TryGetObject(ObjPtr) && ObjPtr && ObjPtr->IsValid())
		{
			if (AddEntry(ParseFromJson(*ObjPtr))) Added++;
		}
	}
	return Added;
}

// ---------------------------------------------------------------------------
// مسار الخادم — أعلى سلطة (AGENTS.md §3)
// ---------------------------------------------------------------------------

bool URok2CivLore::ApplyServerCivs(const TArray<TSharedPtr<FJsonValue>>& CivsArray)
{
	// نبني جانباً ثم نستبدل: حمولة من نسخة backend قديمة (ids بلا story) كانت
	// ستمحو النصّ المدمج على مسار الاستبدال المباشر فتُترك الشاشة بلا حكاية.
	TArray<FRok2CivLore> Staged;
	for (const TSharedPtr<FJsonValue>& V : CivsArray)
	{
		const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
		if (!V.IsValid() || !V->TryGetObject(ObjPtr) || !ObjPtr || !ObjPtr->IsValid()) continue;

		const FRok2CivLore E = ParseFromJson(*ObjPtr);
		if (!IsCompleteEntry(E)) continue;

		bool bDuplicate = false;
		for (const FRok2CivLore& S : Staged)
		{
			if (S.CivId == E.CivId) { bDuplicate = true; break; }
		}
		if (!bDuplicate) Staged.Add(E);
	}

	if (Staged.Num() <= 0)
	{
		UE_LOG(LogRok2CivLore, Warning,
			TEXT("Server civ payload carried no complete lore entry — keeping %d local entries"),
			Entries.Num());
		return false;
	}

	Entries = MoveTemp(Staged);
	bLoaded = true;
	bFromServer = true;
	UE_LOG(LogRok2CivLore, Log, TEXT("Civ lore replaced from server: %d entries"), Entries.Num());
	return true;
}

// ---------------------------------------------------------------------------
// التحميل المحلي: القرص (تطوير) ثم المدمج (الشحن)
// ---------------------------------------------------------------------------

void URok2CivLore::LoadFromDiskOrDefaults()
{
	if (bLoaded) return;
	bLoaded = true;

	const FString JsonPath = FPaths::ProjectContentDir() / Rok2CivLoreSpec::JsonRelativePath;
	FString JsonString;
	if (FFileHelper::LoadFileToString(JsonString, *JsonPath))
	{
		const int32 Added = LoadFromJsonString(JsonString);
		if (Added > 0)
		{
			UE_LOG(LogRok2CivLore, Log, TEXT("Loaded %d civ lore entries from %s"), Added, *JsonPath);
			return;
		}
		UE_LOG(LogRok2CivLore, Warning, TEXT("Failed to read civ lore from %s — using built-in copy"), *JsonPath);
		// تحليل جزئي قد يكون أضاف سجلات ناقصة الترتيب — نبدأ من نظيف قبل المدمج.
		Entries.Empty();
	}
	else
	{
		UE_LOG(LogRok2CivLore, Log, TEXT("civilizations.json not found at %s — using built-in copy"), *JsonPath);
	}

	BuildDefaults();
}

// ---------------------------------------------------------------------------
// النسخة المدمجة — **مطابقة لـdata/civilizations.json حرفياً**
//
// ليست «قيماً افتراضية» بل نسخة الشحن: مجلد data/ خارج حزمة Android، فهذه هي
// النصوص التي يقرأها اللاعب حتى تصل حمولة /v1/meta/all. أي انحراف عن الملف
// يعني لاعبين يقرأون نصّاً غير المؤلَّف — ولا شيء في المترجم يرصده، فيقارنهما
// verify_civ_lore نصّاً بنصّ.
//
// مولَّدة من الملف لا مكتوبة يدوياً (scripts/verify_civ_lore.mjs يحرس التطابق).
// ---------------------------------------------------------------------------

void URok2CivLore::BuildDefaults()
{
	auto Add = [this](const TCHAR* Id, const TCHAR* NameAr, const TCHAR* NameLatin,
		const TCHAR* FantasyAr, const TCHAR* FantasyLatin, const TCHAR* SpecialUnitId,
		TArray<FString>&& Story, const TCHAR* Greeting, TArray<FString>&& Hints)
	{
		FRok2CivLore E;
		E.CivId = Id;
		E.NameAr = NameAr;
		E.NameLatin = NameLatin;
		E.FantasyAr = FantasyAr;
		E.FantasyLatin = FantasyLatin;
		E.SpecialUnitId = SpecialUnitId;
		E.Story = MoveTemp(Story);
		E.Greeting = Greeting;
		E.Hints = MoveTemp(Hints);
		AddEntry(E);
	};

// >>> GENERATED_CIV_LORE_DEFAULTS (from data/civilizations.json)
	Add(TEXT("rome"), TEXT("روما"), TEXT("Rome"), TEXT("نظام وجيوش ثقيلة"),
		TEXT("Order and heavy infantry discipline"), TEXT("legionary"),
		{
			TEXT("من حجرٍ واحدٍ يبدأ الطريق، ومن ألفِ حجرٍ تقوم القناطر."),
			TEXT("لا تنتصر روما بعدد رجالها، بل بصفٍّ لا يتفرّق ودرعٍ لا يتزحزح."),
			TEXT("مَن حمل السكوتوم عرف أن الأمان في الكتف الذي يجاوره."),
			TEXT("ارفع نسورك الذهبية: النظام أطول عمراً من الفتح.")
		},
		TEXT("الصفّ منتظم يا حاكم، والقناطر تنتظر أمرك."),
		{
			TEXT("رصّ الصفّ قبل أن ترصّ الحجر — دفاع مشاتك أثقل من غيره."),
			TEXT("الطريق الممهّد نصف النصر: أرضك تُغلّ لمن يجمع بانتظام."),
			TEXT("لا تستعجل الفتح؛ روما بُنيت بالانضباط لا بالحماسة.")
		});

	Add(TEXT("china"), TEXT("الصين"), TEXT("China"), TEXT("بناء وإمداد ودهاء"),
		TEXT("Builders, logistics, and cunning"), TEXT("chu_ko_nu"),
		{
			TEXT("الحكمة أن تبني ما يبني نفسه، وأن تسبق خصمك بموسمٍ لا بخطوة."),
			TEXT("تحت الأسقف المنحنية تُعدّ الخطط قبل أن تُشدّ الأقواس."),
			TEXT("مَن أتقن حساب أيامه حاصر عدوّه ولمّا يرفع سيفاً."),
			TEXT("أشعل فوانيسك: من رأى بعيداً لم يحتج أن يجري.")
		},
		TEXT("الفوانيس مضاءة يا حاكم، والخطّة تنتظر ريشتك."),
		{
			TEXT("ابنِ سريعاً: مبانيك تقوم قبل مباني غيرك."),
			TEXT("البحث سلاحٌ صامت — كل ورقةٍ تُقرأ تُوفّر معركة."),
			TEXT("نقاط عملك تعود أسرع؛ لا تدع يومك يمرّ بلا حركة.")
		});

	Add(TEXT("arabia"), TEXT("العرب"), TEXT("Arabia"), TEXT("خفة وحركة الصحراء"),
		TEXT("Swift riders of the open dunes"), TEXT("desert_rider"),
		{
			TEXT("الصحراء لا تُمنح لمن يقف؛ تُمنح لمن يعرف أين يشرب وأين يمضي."),
			TEXT("تحت القباب الفيروزية يُروى أن الخبر يصل قبل الرمح."),
			TEXT("خفّةُ الفارس أمضى من ثقل الدرع إذا كان الأفق واسعاً."),
			TEXT("أطلق ركابك: من سبق إلى البئر سبق إلى المُلك.")
		},
		TEXT("النوافير تُغنّي يا حاكم، وخيلك مسرَجة على طرف الأفق."),
		{
			TEXT("مسيراتك أسرع — اجعل البعد حليفك لا عائقك."),
			TEXT("كشافتك تبصر أوسع؛ اعرف الأرض قبل أن تطأها."),
			TEXT("فرسانك أحدّ من غيرهم: اضرب ثم انسحب قبل أن يُغلق الطوق.")
		});

	Add(TEXT("egypt"), TEXT("مصر"), TEXT("Egypt"), TEXT("ثروة النيل والحجر الأبدي"),
		TEXT("River wealth and eternal stone"), TEXT("khopesh_guard"),
		{
			TEXT("النيل لا يعرف العجلة، ومع ذلك صنع أطول الحضارات."),
			TEXT("هنا يُقاس المُلك بما يبقى بعدك: حجرٌ نُقش عليه اسمك."),
			TEXT("مَن ملأ مخازنه وشفى جرحاه غلب من ملأ مقابره."),
			TEXT("ارفع مسلّاتك: الحجر الأبدي شهادةٌ لا تُنسى.")
		},
		TEXT("النيل وفى بعطائه يا حاكم، والحجر ينتظر نقش اسمك."),
		{
			TEXT("إنتاجك أغزر — مخزنك سلاحك الأول."),
			TEXT("الحجر يأتيك أسهل؛ ارفع أسوارك قبل أن تُحتاج."),
			TEXT("جرحاك يشفون أسرع: لا تخشَ معركة تُخلي بها الميدان.")
		});

	Add(TEXT("vikings"), TEXT("الفايكنج"), TEXT("Vikings"), TEXT("غزو ونهب وتحميل"),
		TEXT("Raiders who live by the load and the axe"), TEXT("huskarl"),
		{
			TEXT("الشمال لا يُطعم أبناءه؛ يُعلّمهم أن يأخذوا."),
			TEXT("في قاعة الولائم يُوزن الرجل بما حمله لا بما قاله."),
			TEXT("سفينةٌ محمّلة أصدق من مئة حجرٍ رونيّ."),
			TEXT("اقرع الطبول: من عاد بالحمولة كتب الحكاية.")
		},
		TEXT("القرون تنفخ يا حاكم، والسفينة راسية بانتظار حمولتها."),
		{
			TEXT("حمولة جنودك أكبر — الرحلة الواحدة تكفي رحلتين."),
			TEXT("البرابرة رزقك: ضربك لهم أقسى من ضرب غيرك."),
			TEXT("لا تُحصّن أكثر مما تغزو؛ فأسك أنفع من سورك.")
		});

	Add(TEXT("japan"), TEXT("اليابان"), TEXT("Japan"), TEXT("دقة وإتقان وضربات حاسمة"),
		TEXT("Precision, mastery, and decisive strikes"), TEXT("samurai"),
		{
			TEXT("ضربةٌ واحدة تُتقن خيرٌ من ألفٍ تُجرَّب."),
			TEXT("تحت أزهار الساكورا يُتعلَّم أن الجمال والزوال شيءٌ واحد."),
			TEXT("مَن صقل نفسه أطول حسم أمره أقصر."),
			TEXT("اعبر بوابة التوري: الإتقان طريقٌ لا غاية.")
		},
		TEXT("الساكورا تتناثر يا حاكم، ونصلك مصقول لأمرك."),
		{
			TEXT("هجومك أحدّ — اجعل كل اشتباكٍ قصيراً وحاسماً."),
			TEXT("قادتك يتعلّمون أسرع؛ استثمر في من يقود لا في من يُقاد."),
			TEXT("تدريبك أسرع: عوّض خسارتك قبل أن يلتقط عدوّك نفَسه.")
		});

// <<< GENERATED_CIV_LORE_DEFAULTS

	UE_LOG(LogRok2CivLore, Log, TEXT("Built-in civ lore: %d entries"), Entries.Num());
}
