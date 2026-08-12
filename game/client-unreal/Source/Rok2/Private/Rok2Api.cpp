// Copyright ROK2. Cloudflare API + WebSocket client impl.
// P1-T2: معالجة أخطاء الشبكة + إعادة الاتصال التلقائي + بث حالة الاتصال للواجهات.

#include "Rok2Api.h"
#include "Rok2AudioManager.h"
#include "Rok2CivLore.h"
#include "Rok2BlueprintLibrary.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "HAL/PlatformFileManager.h"
#include "JsonUtilities.h"
#include "WebSocketsModule.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogRok2, Log, All);

// ---------------------------------------------------------------------------
// قراءة آمنة لحقول JSON.
//
// السبب: FJsonObject::GetStringField / GetNumberField / GetBoolField تفشل
// بـ check قاتل إذا كان الحقل غائباً أو من نوع مختلف. الخادم يرسل
// "allianceId": null للاعب بلا تحالف (router.ts:567)، وهذا مسار كل لاعب
// جديد — أي انهيار مضمون عند أول تسجيل دخول. الدوال أدناه تُرجع قيمة
// افتراضية بدل الانهيار، فيبقى العميل حياً أمام أي استجابة ناقصة أو
// أثناء نشر تدريجي للخادم.
// ---------------------------------------------------------------------------
namespace Rok2Json
{
	static FString Str(const TSharedPtr<FJsonObject>& Obj, const TCHAR* Field, const FString& Default = FString())
	{
		FString Value;
		return (Obj.IsValid() && Obj->TryGetStringField(Field, Value)) ? Value : Default;
	}

	static double Num(const TSharedPtr<FJsonObject>& Obj, const TCHAR* Field, double Default = 0.0)
	{
		double Value = 0.0;
		return (Obj.IsValid() && Obj->TryGetNumberField(Field, Value)) ? Value : Default;
	}

	static bool Bool(const TSharedPtr<FJsonObject>& Obj, const TCHAR* Field, bool Default = false)
	{
		bool Value = false;
		return (Obj.IsValid() && Obj->TryGetBoolField(Field, Value)) ? Value : Default;
	}
}

void URok2Api::Init(const FString& ApiBaseUrl, const FString& InKingdomId, const FString& InAdminKey)
{
	BaseUrl = ApiBaseUrl;
	KingdomId = InKingdomId;
	AdminKey = InAdminKey;
	DeviceId = TEXT("ue5_dev_") + FGuid::NewGuid().ToString();


	// load device id from saved file; generate if missing
	FString DeviceIdPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("rok2_device.txt"));
	if (!FPaths::FileExists(DeviceIdPath))
	{
		DeviceId = TEXT("ue_") + FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(16);
		FFileHelper::SaveStringToFile(DeviceId, *DeviceIdPath);
	}
	else
	{
		FFileHelper::LoadFileToString(DeviceId, *DeviceIdPath);
	}

	// قيم fallback محلية لبيانات التوازن — تُستبدل ببيانات الخادم عند FetchMeta (P1-T6)
	Meta.bLoaded = false;
	Meta.ProductionLevelMult = 1.2;
	Meta.ProductionBase.Add(TEXT("farm"), 100.0);
	Meta.ProductionBase.Add(TEXT("lumber_mill"), 100.0);
	Meta.ProductionBase.Add(TEXT("quarry"), 70.0);
	Meta.ProductionBase.Add(TEXT("goldmine"), 40.0);
	Meta.TrainableUnits.Add({TEXT("infantry_t1"), TEXT("مشاة T1"), TEXT("infantry")});
	Meta.TrainableUnits.Add({TEXT("cavalry_t1"), TEXT("فرسان T1"), TEXT("cavalry")});
	Meta.TrainableUnits.Add({TEXT("archer_t1"), TEXT("رماة T1"), TEXT("archer")});

	UE_LOG(LogRok2, Log, TEXT("Rok2Api init: %s device=%s"), *BaseUrl, *DeviceId);

	FetchMeta();
}

// ---------------------------------------------------------------------------
// P1-T6: سحب بيانات التوازن الموحدة من الخادم بدل القيم الثابتة
// ---------------------------------------------------------------------------
void URok2Api::FetchMeta()
{
	TWeakObjectPtr<URok2Api> WeakThis(this);
	Get(TEXT("/v1/meta/all"), [WeakThis](const TSharedPtr<FJsonObject>& Obj)
	{
		if (!WeakThis.IsValid()) return;
		URok2Api* Self = WeakThis.Get();

		// وحدات قابلة للتدريب
		const TSharedPtr<FJsonObject>* ConstObj;
		if (Obj->TryGetObjectField(TEXT("constants"), ConstObj) && ConstObj->IsValid())
		{
			const TSharedPtr<FJsonObject>* ProdObj;
			if ((*ConstObj)->TryGetObjectField(TEXT("productionBase"), ProdObj) && ProdObj->IsValid())
			{
				Self->Meta.ProductionBase.Empty();
				for (const auto& KV : (*ProdObj)->Values)
				{
					Self->Meta.ProductionBase.Add(FString(KV.Key), KV.Value->AsNumber());
				}
			}
			Self->Meta.ProductionLevelMult = Rok2Json::Num((*ConstObj), TEXT("productionLevelMult"));

			const TArray<TSharedPtr<FJsonValue>>* UnitsArr;
			if ((*ConstObj)->TryGetArrayField(TEXT("trainableUnits"), UnitsArr))
			{
				Self->Meta.TrainableUnits.Empty();
				for (const auto& V : *UnitsArr)
				{
					const TSharedPtr<FJsonObject> U = V->AsObject();
					if (!U.IsValid()) continue;
					FRok2TrainableUnit Unit;
					Unit.Id = Rok2Json::Str(U, TEXT("id"));
					Unit.Name = Rok2Json::Str(U, TEXT("name"));
					Unit.Branch = Rok2Json::Str(U, TEXT("branch"));
					Self->Meta.TrainableUnits.Add(Unit);
				}
			}
		}

		// المباني
		const TSharedPtr<FJsonObject>* BldObj;
		if (Obj->TryGetObjectField(TEXT("buildings"), BldObj) && BldObj->IsValid())
		{
			const TArray<TSharedPtr<FJsonValue>>* BldArr;
			if ((*BldObj)->TryGetArrayField(TEXT("buildings"), BldArr))
			{
				Self->Meta.Buildings.Empty();
				for (const auto& V : *BldArr)
				{
					const TSharedPtr<FJsonObject> B = V->AsObject();
					if (!B.IsValid()) continue;
					FRok2BuildingMeta BM;
					BM.Id = Rok2Json::Str(B, TEXT("id"));
					BM.Category = Rok2Json::Str(B, TEXT("category"));
					BM.Name = Rok2Json::Str(B, TEXT("name"));
					BM.Desc = Rok2Json::Str(B, TEXT("desc"));
					Self->Meta.Buildings.Add(BM);
				}
			}
		}

		// ---------------------------------------------------------------
		// P6-T5: الحضارات ونَفَسها القصصي — الخادم هو السلطة
		//
		// قبل هذا البند كانت الحضارات تأتي من قائمة مكتوبة في العميل ولا
		// تُقرأ من هذه الحمولة أصلاً، رغم أن /v1/meta/all يخدمها منذ P1-T6.
		// النتيجة: قائمة الاختيار كانت تعرض حضارة يرفضها /v1/city/init
		// (byzantium) وتُسقط أخرى يقبلها (egypt).
		//
		// الترتيب مقصود: نُطبّق النصّ على السجلّ **أولاً**، ثم نبني قائمة
		// الحضارات منه. العكس كان سيبني القائمة من النسخة المدمجة ثم يحدّث
		// السجلّ، فتبقى القائمة على النصّ القديم حتى إعادة التشغيل.
		// ---------------------------------------------------------------
		const TSharedPtr<FJsonObject>* CivsObj;
		if (Obj->TryGetObjectField(TEXT("civilizations"), CivsObj) && CivsObj->IsValid())
		{
			const TArray<TSharedPtr<FJsonValue>>* CivsArr;
			if ((*CivsObj)->TryGetArrayField(TEXT("civilizations"), CivsArr) && CivsArr)
			{
				if (URok2CivLore* Lore = URok2CivLore::Get())
				{
					if (Lore->ApplyServerCivs(*CivsArr))
					{
						// القائمة تُشتقّ من السجلّ الذي صار الآن نسخة الخادم
						Self->Civilizations = URok2BlueprintLibrary::GetDefaultCivilizations();
						UE_LOG(LogRok2, Log, TEXT("Civilizations from server: %d"),
							Self->Civilizations.Num());
					}
				}
			}
		}

		Self->Meta.bLoaded = true;
		// أعد حساب المعدلات من بيانات الخادم لو المباني محمّلة
		Self->RecomputeResourceRates();
		UE_LOG(LogRok2, Log, TEXT("Meta loaded from server: %d units, %d buildings"),
			Self->Meta.TrainableUnits.Num(), Self->Meta.Buildings.Num());
		Self->OnMetaLoaded.Broadcast(true);
	});
}

FString URok2Api::BuildUrl(const FString& Path) const
{
	FString P = Path;
	if (!P.StartsWith(TEXT("/"))) P = TEXT("/") + P;
	return BaseUrl + P;
}

FString URok2Api::AuthHeader() const
{
	return Token.IsEmpty() ? FString() : FString::Printf(TEXT("Bearer %s"), *Token);
}

void URok2Api::SetOnline(bool bNewOnline, const FString& Reason)
{
	OnConnectionState.Broadcast(bNewOnline, Reason);
}

// ---------------------------------------------------------------------------
// HTTP مع retry backoff — يعيد المحاولة فقط على أخطاء الشبكة (لا استجابة/مهلة)
// وأخطاء 5xx المؤقتة. أخطاء 4xx منطقية تُمرر فوراً بدون إعادة.
// ---------------------------------------------------------------------------

/** سياق طلب واحد قابل لإعادة المحاولة — يحمل كل ما يلزم لإعادة بناء الطلب */
struct FRok2RetryCtx
{
	FString Verb;
	FString Path;
	FString JsonBody;
	bool bAuth = false;
	int32 MaxRetries = 0;
	int32 Attempt = 0;
	TFunction<void(const TSharedPtr<FJsonObject>&)> OnOk;
	TFunction<void(const FString&)> OnErr;
};

void URok2Api::Rok2SendRequest(URok2Api* Self, TSharedPtr<FRok2RetryCtx> Ctx)
{
	TWeakObjectPtr<URok2Api> WeakThis(Self);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = FHttpModule::Get().CreateRequest();
	Req->SetURL(Self->BuildUrl(Ctx->Path));
	Req->SetVerb(Ctx->Verb);
	Req->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Req->SetTimeout(URok2Api::HttpTimeoutSeconds);
	if (Ctx->bAuth && !Self->GetToken().IsEmpty())
		Req->SetHeader(TEXT("Authorization"), Self->AuthHeader());
	if (!Ctx->JsonBody.IsEmpty())
		Req->SetContentAsString(Ctx->JsonBody);

	Req->OnProcessRequestComplete().BindLambda(
		[WeakThis, Ctx](FHttpRequestPtr DoneReq, FHttpResponsePtr Resp, bool bSuccess)
	{
		if (!WeakThis.IsValid()) return;
		URok2Api* Self = WeakThis.Get();

		const bool bHasResponse = Resp.IsValid();
		const int32 Code = bHasResponse ? Resp->GetResponseCode() : 0;
		const FString Body = bHasResponse ? Resp->GetContentAsString() : FString();

		// نجاح مؤكد
		if (bSuccess && bHasResponse && Code < 400)
		{
			TSharedPtr<FJsonObject> Obj;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
			if (FJsonSerializer::Deserialize(Reader, Obj) && Obj.IsValid())
			{
				Self->SetOnline(true, TEXT("متصل"));
				Ctx->OnOk(Obj);
				return;
			}
			FString Err = FString::Printf(TEXT("استجابة غير صالحة من الخادم (HTTP %d)"), Code);
			UE_LOG(LogRok2, Error, TEXT("%s %s -> bad json"), *Ctx->Verb, *Ctx->Path);
			Self->EmitError(Err);
			if (Ctx->OnErr) Ctx->OnErr(Err);
			return;
		}

		// هل الخطأ يستحق إعادة المحاولة؟ (انقطاع شبكة/مهلة أو 5xx مؤقت)
		const bool bRetryable = (!bHasResponse) || (Code == 0) || (Code >= 500);
		if (bRetryable && Ctx->Attempt < Ctx->MaxRetries)
		{
			Ctx->Attempt++;
			const float Delay = FMath::Pow(2.f, (float)Ctx->Attempt); // 2s, 4s, ...
			UE_LOG(LogRok2, Warning, TEXT("%s %s failed (code=%d) — retry %d/%d in %.0fs"),
				*Ctx->Verb, *Ctx->Path, Code, Ctx->Attempt, Ctx->MaxRetries, Delay);
			Self->SetOnline(false, FString::Printf(TEXT("انقطع الاتصال — إعادة المحاولة %d/%d..."), Ctx->Attempt, Ctx->MaxRetries));

			FTimerHandle TimerHandle;
			FTimerDelegate D;
			D.BindLambda([WeakThis, Ctx]()
			{
				if (!WeakThis.IsValid()) return;
				Rok2SendRequest(WeakThis.Get(), Ctx);
			});

			if (UWorld* W = Self->GetWorld())
			{
				W->GetTimerManager().SetTimer(TimerHandle, D, Delay, false);
				return;
			}
			// بلا عالم صالح — نكمل للفشل النهائي أدناه
		}

		// استخراج رسالة الخطأ من جسم الاستجابة إن وُجد
		FString Err;
		if (bHasResponse)
		{
			TSharedPtr<FJsonObject> Obj;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
			if (FJsonSerializer::Deserialize(Reader, Obj) && Obj.IsValid())
				Obj->TryGetStringField(TEXT("error"), Err);
		}
		if (Err.IsEmpty())
		{
			Err = bHasResponse
				? FString::Printf(TEXT("فشل الطلب (HTTP %d)"), Code)
				: TEXT("فشل الاتصال بالخادم — تحقق من الشبكة");
		}
		UE_LOG(LogRok2, Warning, TEXT("%s %s -> %s"), *Ctx->Verb, *Ctx->Path, *Err);
		if (bRetryable) Self->SetOnline(false, TEXT("تعذر الاتصال بالخادم"));
		Self->EmitError(Err);
		if (Ctx->OnErr) Ctx->OnErr(Err);
	});
	Req->ProcessRequest();
}

void URok2Api::RequestWithRetry(const FString& Verb, const FString& Path, const FString& JsonBody, bool bAuth,
	TFunction<void(const TSharedPtr<FJsonObject>&)> OnOk, TFunction<void(const FString&)> OnErr, int32 MaxRetries)
{
	TSharedPtr<FRok2RetryCtx> Ctx = MakeShared<FRok2RetryCtx>();
	Ctx->Verb = Verb;
	Ctx->Path = Path;
	Ctx->JsonBody = JsonBody;
	Ctx->bAuth = bAuth;
	Ctx->MaxRetries = MaxRetries;
	Ctx->OnOk = MoveTemp(OnOk);
	Ctx->OnErr = MoveTemp(OnErr);
	Rok2SendRequest(this, Ctx);
}

void URok2Api::Get(const FString& Path, TFunction<void(const TSharedPtr<FJsonObject>&)> OnOk)
{
	RequestWithRetry(TEXT("GET"), Path, FString(), !Token.IsEmpty(), OnOk, nullptr, HttpMaxRetries);
}

void URok2Api::Post(const FString& Path, const FString& JsonBody, bool bAuth,
	TFunction<void(const TSharedPtr<FJsonObject>&)> OnOk, TFunction<void(const FString&)> OnErr)
{
	RequestWithRetry(TEXT("POST"), Path, JsonBody, bAuth, OnOk, OnErr, HttpMaxRetries);
}

void URok2Api::LoginAsGuest()
{
	FString Body = FString::Printf(TEXT("{\"deviceId\":\"%s\"}"), *DeviceId);
	TWeakObjectPtr<URok2Api> WeakThis(this);
	Post(TEXT("/v1/auth/guest"), Body, false, [WeakThis](const TSharedPtr<FJsonObject>& Obj)
	{
		if (!WeakThis.IsValid()) return;
		URok2Api* Self = WeakThis.Get();
		Obj->TryGetStringField(TEXT("token"), Self->Token);
		const TSharedPtr<FJsonObject>* PlayerObj;
		if (Obj->TryGetObjectField(TEXT("player"), PlayerObj) && PlayerObj->IsValid())
		{
			Self->ParsePlayer(*PlayerObj);
		}
		UE_LOG(LogRok2, Log, TEXT("Login ok token=%s player=%s"), *Self->Token.Left(12), *Self->Player.Id);
			Self->OnLoginComplete.Broadcast(Self->Token);
			Self->EmitToast(TEXT("تم تسجيل الدخول"));
			// اللاعب العائد يملك سجلاً بالفعل؛ حمّل تقدمه السلطوي قبل فتح شاشة القادة.
			if (Self->HasPlayer()) Self->FetchCommanders();
	},
	[WeakThis](const FString& Err)
	{
		if (!WeakThis.IsValid()) return;
		WeakThis->EmitError(FString::Printf(TEXT("فشل تسجيل الدخول: %s"), *Err));
	});
}

void URok2Api::InitCity(const FString& Civ, const FString& InPlayerName)
{
	FString Body = FString::Printf(TEXT("{\"civ\":\"%s\",\"name\":\"%s\"}"), *Civ, *InPlayerName);
	TWeakObjectPtr<URok2Api> WeakThis(this);
	Post(TEXT("/v1/city/init"), Body, true, [WeakThis](const TSharedPtr<FJsonObject>& Obj)
	{
		if (!WeakThis.IsValid()) return;
		URok2Api* Self = WeakThis.Get();
		FString NewToken;
		if (Obj->TryGetStringField(TEXT("token"), NewToken) && !NewToken.IsEmpty())
		{
			Self->Token = NewToken;
		}
		const TSharedPtr<FJsonObject>* PlayerObj;
		if (Obj->TryGetObjectField(TEXT("player"), PlayerObj) && PlayerObj->IsValid())
		{
			Self->ParsePlayer(*PlayerObj);
			Self->OnPlayerLoaded.Broadcast(Self->Player);
		}
			Self->EmitToast(TEXT("تم تأسيس المدينة"));
			// city/init يمنح قائد البداية؛ نزامنه فوراً بدلاً من ترك قائمة العميل الفارغة.
			Self->FetchCommanders();
			// P5-T6: تشغيل موسيقى الحضارة
		if (URok2AudioManager* Audio = URok2AudioManager::Get())
		{
			Audio->InitForCiv(Civ);
			Audio->PlayMusic();
		}
		Self->LoadCity();
	});
}

void URok2Api::ParsePlayer(const TSharedPtr<FJsonObject>& Obj)
{
	Player.Id = Rok2Json::Str(Obj, TEXT("id"));
	Player.Name = Rok2Json::Str(Obj, TEXT("name"));
	Player.Civ = Rok2Json::Str(Obj, TEXT("civ"));
	Player.AllianceId = Rok2Json::Str(Obj, TEXT("allianceId"));
	Player.RegionId = Rok2Json::Str(Obj, TEXT("regionId"));
	Player.X = Rok2Json::Num(Obj, TEXT("x"));
	Player.Y = Rok2Json::Num(Obj, TEXT("y"));
	Player.Power = (int32)Rok2Json::Num(Obj, TEXT("power"));
}

void URok2Api::ParseCity(const TSharedPtr<FJsonObject>& Obj)
{
	const TSharedPtr<FJsonObject>* CityObj;
	if (Obj->TryGetObjectField(TEXT("city"), CityObj) && CityObj->IsValid())
	{
		City.HallLevel = (int32)Rok2Json::Num((*CityObj), TEXT("hall_level"));
		City.Resources.Food = Rok2Json::Num((*CityObj), TEXT("food"));
		City.Resources.Wood = Rok2Json::Num((*CityObj), TEXT("wood"));
		City.Resources.Stone = Rok2Json::Num((*CityObj), TEXT("stone"));
					City.Resources.Gold = Rok2Json::Num((*CityObj), TEXT("gold"));
			City.Gems = (int32)Rok2Json::Num((*CityObj), TEXT("gems"));
			City.UpdatedAt = (int64)Rok2Json::Num((*CityObj), TEXT("updated_at"));

	}
	const TSharedPtr<FJsonObject>* PlayerObj;
	if (Obj->TryGetObjectField(TEXT("player"), PlayerObj) && PlayerObj->IsValid())
	{
		ParsePlayer(*PlayerObj);
		OnPlayerLoaded.Broadcast(Player);
	}

	// buildings
	Buildings.Empty();
	const TSharedPtr<FJsonObject>* BObj;
	if (Obj->TryGetObjectField(TEXT("buildings"), BObj) && BObj->IsValid())
	{
		for (const auto& KV : (*BObj)->Values)
		{
			Buildings.Add(FString(KV.Key), (int32)KV.Value->AsNumber());
		}
	}

	// troops
	Troops.Empty();
	const TSharedPtr<FJsonObject>* TObj;
	if (Obj->TryGetObjectField(TEXT("troops"), TObj) && TObj->IsValid())
	{
		for (const auto& KV : (*TObj)->Values)
		{
			FRok2TroopEntry Entry;
			Entry.UnitId = FString(KV.Key);
			Entry.Count = (int32)KV.Value->AsNumber();
			Troops.Add(Entry);
		}
	}

	// التخطيط السلطوي للقلعة — يظل فارغاً في الحسابات القديمة التي لم تحفظ نسخة بعد.
	City.LayoutPlacements.Empty();
	City.LayoutVersion = 0;
	City.LayoutUpdatedAt = 0;
	const TSharedPtr<FJsonObject>* LayoutObj = nullptr;
	if (Obj->TryGetObjectField(TEXT("layout"), LayoutObj) && LayoutObj && LayoutObj->IsValid())
	{
		City.LayoutVersion = (int32)Rok2Json::Num(*LayoutObj, TEXT("version"));
		City.LayoutUpdatedAt = (int64)Rok2Json::Num(*LayoutObj, TEXT("updatedAt"));
		const TArray<TSharedPtr<FJsonValue>>* Placements = nullptr;
		if ((*LayoutObj)->TryGetArrayField(TEXT("placements"), Placements) && Placements)
		{
			for (const TSharedPtr<FJsonValue>& Value : *Placements)
			{
				const TSharedPtr<FJsonObject> PlacementObj = Value->AsObject();
				if (!PlacementObj.IsValid()) continue;
				FRok2CityLayoutPlacement Placement;
				Placement.BuildingId = Rok2Json::Str(PlacementObj, TEXT("buildingId"));
				Placement.Q = (int32)Rok2Json::Num(PlacementObj, TEXT("q"));
				Placement.R = (int32)Rok2Json::Num(PlacementObj, TEXT("r"));
				Placement.RotationSteps = (int32)Rok2Json::Num(PlacementObj, TEXT("rotationSteps"));
				Placement.Facade = Rok2Json::Str(PlacementObj, TEXT("facade"));
				if (!Placement.BuildingId.IsEmpty()) City.LayoutPlacements.Add(Placement);
			}
		}
	}

	// queues
	City.ActiveQueues.Empty();
	const TArray<TSharedPtr<FJsonValue>>* QueuesArr;
		const bool bHasQueues = Obj->TryGetArrayField(TEXT("activeQueues"), QueuesArr)
			|| Obj->TryGetArrayField(TEXT("active_queues"), QueuesArr);
		if (bHasQueues && QueuesArr)
		{
			for (const auto& V : *QueuesArr)
			{
				const TSharedPtr<FJsonObject> Q = V->AsObject();
				if (!Q.IsValid()) continue;
				FRok2QueueEntry Entry;
				Entry.Id = Rok2Json::Str(Q, TEXT("id"));
				Entry.Type = Rok2Json::Str(Q, TEXT("type"));
				if (Entry.Type == TEXT("build")) Entry.Type = TEXT("building");
				Entry.StartMs = (int64)Rok2Json::Num(Q, TEXT("startMs"));
				Entry.EndMs = (int64)Rok2Json::Num(Q, TEXT("etaMs"));
				if (Entry.EndMs <= 0) Entry.EndMs = (int64)Rok2Json::Num(Q, TEXT("endMs"));
				Entry.RemainingSeconds = (int32)Rok2Json::Num(Q, TEXT("remainingSeconds"));
				Entry.FinishCostGems = (int32)Rok2Json::Num(Q, TEXT("finishCostGems"));
				Entry.State = Rok2Json::Str(Q, TEXT("state"));
				const TSharedPtr<FJsonObject>* DataObj = nullptr;
				if (Q->TryGetObjectField(TEXT("data"), DataObj) && DataObj && DataObj->IsValid())
				{
					Entry.RefId = Rok2Json::Str(*DataObj, TEXT("buildingId"));
					Entry.Level = (int32)Rok2Json::Num(*DataObj, TEXT("level"));
				}
				if (Entry.RefId.IsEmpty()) Entry.RefId = Rok2Json::Str(Q, TEXT("refId"));
				City.ActiveQueues.Add(Entry);
			}
		}

		const TSharedPtr<FJsonObject>* ProductionObj = nullptr;
		if (Obj->TryGetObjectField(TEXT("production"), ProductionObj) && ProductionObj && ProductionObj->IsValid())
		{
			const TSharedPtr<FJsonObject>* RatesObj = nullptr;
			if ((*ProductionObj)->TryGetObjectField(TEXT("ratesPerHour"), RatesObj) && RatesObj && RatesObj->IsValid())
			{
				City.Rates.Food = Rok2Json::Num(*RatesObj, TEXT("food"));
				City.Rates.Wood = Rok2Json::Num(*RatesObj, TEXT("wood"));
				City.Rates.Stone = Rok2Json::Num(*RatesObj, TEXT("stone"));
				City.Rates.Gold = Rok2Json::Num(*RatesObj, TEXT("gold"));
			}
			else RecomputeResourceRates();
		}
		else RecomputeResourceRates();

	// P6-T5: تحية الحضارة بنبرتها — هنا لأن الـApi هو منتج الإشعارات كلها
	// (قتال/منطقة/بحث/حملة/كشافة)، وهذه أول لحظة تُعرف فيها حضارة اللاعب من
	// الخادم. تُبثّ **قبل** OnCityLoaded فترى الودجات المدينة والتحية معاً.
	MaybeGreetCiv();

	OnCityLoaded.Broadcast(City);
}

// ---------------------------------------------------------------------------
// P6-T5: تحية الحضارة — مرة واحدة في الجلسة
//
// ParseCity يُنادى مع كل نبضة شبكة (WS tick + كل LoadCity)، وتحيةٌ تتكرر كل
// ثانية ضجيج لا نبرة — فالمِزلاج راية جلسة لا حالة محفوظة: التحية ترحيب
// بالدخول، ودخولٌ جديد يستحق ترحيباً جديداً.
// ---------------------------------------------------------------------------
void URok2Api::MaybeGreetCiv()
{
	if (bCivGreetingShown) return;

	// الحمولة قد تصل قبل بلوك player (مدينة بلا لاعب) — ليس وقت التحية بعد،
	// ولا نرفع الراية فتُفقد التحية إلى الأبد.
	const FString CivId = Player.Civ;
	if (CivId.IsEmpty()) return;

	URok2CivLore* Lore = URok2CivLore::Get();
	if (!Lore || !Lore->HasLore(CivId)) return;

	const FRok2CivLore& L = Lore->GetLore(CivId);
	if (L.Greeting.IsEmpty()) return;

	bCivGreetingShown = true;

	// عبر نظام الإشعارات القائم لا بطاقة جديدة: الـHUD يعرضها بطاقةً تتلاشى
	// (وثيقة UI §7: «إشعار داخل اللعبة... ولا توقف اللعب أبداً») فتُقرأ ولا
	// تُطالَب بإغلاق. العنوان اسم الحضارة والمتن تحيتها — فالنبرة معنونة
	// بصاحبها. والنوع "toast" لأن الـHUD يلوّن الأنواع المعروفة فقط ويردّ ما
	// سواها إلى لون اللوحة، فنوعٌ مخترع كان سيمرّ بلا لون بلا فائدة.
	PushNotification(TEXT("toast"), L.NameAr, L.Greeting, CivGreetingTtlSeconds);
}

// ---------------------------------------------------------------------------
// معدلات الإنتاج (P1-T5) — نفس معادلة الخادم: base * 1.2^(level-1)
// ---------------------------------------------------------------------------
void URok2Api::RecomputeResourceRates()
{
	auto Lvl = [this](const FString& Id) -> int32
	{
		const int32* P = Buildings.Find(Id);
		return P ? *P : 1; // الخادم يعامل المباني المفقودة كمستوى 1
	};
	auto Rate = [this](const FString& Id, int32 Level) -> float
	{
		const double* Base = Meta.ProductionBase.Find(Id);
		if (!Base) return 0.f;
		return (float)(*Base) * FMath::Pow((float)Meta.ProductionLevelMult, (float)FMath::Max(0, Level - 1));
	};
	City.Rates.Food = Rate(TEXT("farm"), Lvl(TEXT("farm")));
	City.Rates.Wood = Rate(TEXT("lumber_mill"), Lvl(TEXT("lumber_mill")));
	City.Rates.Stone = Rate(TEXT("quarry"), Lvl(TEXT("quarry")));
	City.Rates.Gold = Rate(TEXT("goldmine"), Lvl(TEXT("goldmine")));
}

void URok2Api::ParseWorld(const TSharedPtr<FJsonObject>& Obj)
{
	World.SeasonDay = (int32)Rok2Json::Num(Obj, TEXT("seasonDay"));

	World.Cities.Empty();
	const TArray<TSharedPtr<FJsonValue>>* CitiesArr;
	if (Obj->TryGetArrayField(TEXT("cities"), CitiesArr))
	{
		for (const auto& V : *CitiesArr)
		{
			const TSharedPtr<FJsonObject> C = V->AsObject();
			if (!C.IsValid()) continue;
			FRok2CityEntity E;
			E.PlayerId = Rok2Json::Str(C, TEXT("playerId"));
			E.Name = Rok2Json::Str(C, TEXT("name"));
			E.AllianceId = Rok2Json::Str(C, TEXT("allianceId"));
			E.X = Rok2Json::Num(C, TEXT("x"));
			E.Y = Rok2Json::Num(C, TEXT("y"));
			E.HallLevel = (int32)Rok2Json::Num(C, TEXT("hallLevel"));
			E.RegionId = Rok2Json::Str(C, TEXT("regionId"));
			World.Cities.Add(E);
		}
	}

	World.Passes.Empty();
	const TArray<TSharedPtr<FJsonValue>>* PassesArr;
	if (Obj->TryGetArrayField(TEXT("passes"), PassesArr))
	{
		for (const auto& V : *PassesArr)
		{
			const TSharedPtr<FJsonObject> P = V->AsObject();
			if (!P.IsValid()) continue;
			FRok2PassEntity E;
			E.Id = Rok2Json::Str(P, TEXT("id"));
			E.OwnerAllianceId = Rok2Json::Str(P, TEXT("ownerAllianceId"));
			E.CaptureProgress = Rok2Json::Num(P, TEXT("captureProgress"));
			E.State = Rok2Json::Str(P, TEXT("state"));
			E.Level = (int32)Rok2Json::Num(P, TEXT("level"));
			E.From = Rok2Json::Str(P, TEXT("from"));
			E.To = Rok2Json::Str(P, TEXT("to"));
			E.X = Rok2Json::Num(P, TEXT("x"));
			E.Y = Rok2Json::Num(P, TEXT("y"));
			E.UnlockDay = (int32)Rok2Json::Num(P, TEXT("unlockDay"));
			World.Passes.Add(E);
		}
	}

	World.Nodes.Empty();
	const TArray<TSharedPtr<FJsonValue>>* NodesArr;
	if (Obj->TryGetArrayField(TEXT("nodes"), NodesArr))
	{
		for (const auto& V : *NodesArr)
		{
			const TSharedPtr<FJsonObject> N = V->AsObject();
			if (!N.IsValid()) continue;
			FRok2NodeEntity E;
			E.Id = Rok2Json::Str(N, TEXT("id"));
			E.Kind = Rok2Json::Str(N, TEXT("kind"));
			E.Level = (int32)Rok2Json::Num(N, TEXT("level"));
			E.X = Rok2Json::Num(N, TEXT("x"));
			E.Y = Rok2Json::Num(N, TEXT("y"));
			E.Remaining = Rok2Json::Num(N, TEXT("remaining"));
			World.Nodes.Add(E);
		}
	}

	// ---- alliance structures: منشآت تحالف ثابتة مع نطاقات مرئية من الخادم ----
	World.AllianceStructures.Empty();
	const TArray<TSharedPtr<FJsonValue>>* StructuresArr;
	if (Obj->TryGetArrayField(TEXT("allianceStructures"), StructuresArr))
	{
		for (const auto& V : *StructuresArr)
		{
			const TSharedPtr<FJsonObject> S = V->AsObject();
			if (!S.IsValid()) continue;
			FRok2AllianceStructure E;
			E.Id = Rok2Json::Str(S, TEXT("id"));
			E.Kind = Rok2Json::Str(S, TEXT("kind"));
			E.AllianceId = Rok2Json::Str(S, TEXT("allianceId"));
			E.X = Rok2Json::Num(S, TEXT("x"));
			E.Y = Rok2Json::Num(S, TEXT("y"));
			E.Radius = Rok2Json::Num(S, TEXT("radius"));
			E.ProtectionRadius = Rok2Json::Num(S, TEXT("protectionRadius"));
			E.MarchDamageReduction = Rok2Json::Num(S, TEXT("marchDamageReduction"));
			E.MapMarker = Rok2Json::Str(S, TEXT("mapMarker"));
			World.AllianceStructures.Add(E);
		}
	}

	// ---- marches (P1-T3) ----
	World.Marches.Empty();
	const TArray<TSharedPtr<FJsonValue>>* MarchesArr;
	if (Obj->TryGetArrayField(TEXT("marches"), MarchesArr))
	{
		for (const auto& V : *MarchesArr)
		{
			const TSharedPtr<FJsonObject> M = V->AsObject();
			if (!M.IsValid()) continue;
			FRok2MarchEntity E;
			ParseMarchEntity(M, E);
			World.Marches.Add(E);
		}
	}

	// ---- scouts (P5-T5) ----
	World.Scouts.Empty();
	const TArray<TSharedPtr<FJsonValue>>* ScoutsArr;
	if (Obj->TryGetArrayField(TEXT("scouts"), ScoutsArr))
	{
		for (const auto& V : *ScoutsArr)
		{
			const TSharedPtr<FJsonObject> S = V->AsObject();
			if (!S.IsValid()) continue;
			FRok2ScoutEntity E;
			ParseScoutEntity(S, E);
			World.Scouts.Add(E);
		}
	}

	// ---- battle reports (P1-T4) ----
	const TArray<TSharedPtr<FJsonValue>>* ReportsArr;
	if (Obj->TryGetArrayField(TEXT("reports"), ReportsArr))
	{
		BattleReports.Empty();
		for (const auto& V : *ReportsArr)
		{
			const TSharedPtr<FJsonObject> R = V->AsObject();
			if (!R.IsValid()) continue;
			FRok2BattleReport Report;
			ParseBattleReport(R, Report);
			BattleReports.Add(Report);
		}
		OnBattleReports.Broadcast(BattleReports);
	}

	// ---- طوابير التحالف الجارية (P2-T5) — تعرض في HUD الطوابير (P2-T6) ----
	const TArray<TSharedPtr<FJsonValue>>* QueuesArr;
	if (Obj->TryGetArrayField(TEXT("queues"), QueuesArr))
	{
		City.ActiveQueues.Empty();
		for (const auto& V : *QueuesArr)
		{
			const TSharedPtr<FJsonObject> Q = V->AsObject();
			if (!Q.IsValid()) continue;
			FRok2QueueEntry E;
			E.Id = Rok2Json::Str(Q, TEXT("id"));
			E.Type = Rok2Json::Str(Q, TEXT("type"));
			const TSharedPtr<FJsonObject>* DataObj;
			if (Q->TryGetObjectField(TEXT("data"), DataObj) && DataObj->IsValid())
			{
				FString BId, TId;
				if ((*DataObj)->TryGetStringField(TEXT("buildingId"), BId)) E.RefId = BId;
				else if ((*DataObj)->TryGetStringField(TEXT("techId"), TId)) E.RefId = TId;
				E.Level = (int32)Rok2Json::Num((*DataObj), TEXT("level"));
			}
			E.StartMs = (int64)Rok2Json::Num(Q, TEXT("startMs"));
			E.EndMs = (int64)Rok2Json::Num(Q, TEXT("etaMs"));
			City.ActiveQueues.Add(E);
		}
		OnCityLoaded.Broadcast(City);
	}

	// ---- حالة المناطق (P2-T4) — لمؤقّت المناطق في الـ HUD (P2-T6) ----
	const TArray<TSharedPtr<FJsonValue>>* ZonesArr;
	if (Obj->TryGetArrayField(TEXT("zones"), ZonesArr))
	{
		World.Zones.Empty();
		for (const auto& V : *ZonesArr)
		{
			const TSharedPtr<FJsonObject> Z = V->AsObject();
			if (!Z.IsValid()) continue;
			FRok2ZoneStatus E;
			E.ZoneId = (int32)Rok2Json::Num(Z, TEXT("zoneId"));
			E.RegionId = Rok2Json::Str(Z, TEXT("regionId"));
			E.bUnlocked = Rok2Json::Bool(Z, TEXT("unlocked"));
			E.UnlockDay = (int32)Rok2Json::Num(Z, TEXT("unlockDay"));
			World.Zones.Add(E);
		}
		OnZonesUpdated.Broadcast(World.Zones);
	}

	// ---- P6-T6: سجل الدردشة الحية ----
	const TArray<TSharedPtr<FJsonValue>>* ChatArr;
	if (Obj->TryGetArrayField(TEXT("chatHistory"), ChatArr))
	{
		ChatHistory.Empty();
		for (const auto& V : *ChatArr)
		{
			const TSharedPtr<FJsonObject> M = V->AsObject();
			if (!M.IsValid()) continue;
			FRok2ChatMessage Msg;
			Msg.Id = Rok2Json::Str(M, TEXT("id"));
			Msg.Channel = Rok2Json::Str(M, TEXT("channel"));
			Msg.PlayerId = Rok2Json::Str(M, TEXT("playerId"));
			Msg.PlayerName = Rok2Json::Str(M, TEXT("playerName"));
			Msg.Civ = Rok2Json::Str(M, TEXT("civ"));
			Msg.Text = Rok2Json::Str(M, TEXT("text"));
			Msg.TimestampMs = (int64)Rok2Json::Num(M, TEXT("timestampMs"));
			ChatHistory.Add(Msg);
		}
	}

	OnWorldSnapshot.Broadcast(World);
}

// ---------------------------------------------------------------------------
// Marches (P1-T3) — parsing + upsert من أحداث الـ WS
// ---------------------------------------------------------------------------
void URok2Api::ParseMarchEntity(const TSharedPtr<FJsonObject>& M, FRok2MarchEntity& E) const
{
	E.Id = Rok2Json::Str(M, TEXT("id"));
	E.OwnerPlayerId = Rok2Json::Str(M, TEXT("ownerPlayerId"));
	E.AllianceId = Rok2Json::Str(M, TEXT("allianceId"));
	E.FromX = Rok2Json::Num(M, TEXT("fromX"));
	E.FromY = Rok2Json::Num(M, TEXT("fromY"));
	E.ToX = Rok2Json::Num(M, TEXT("toX"));
	E.ToY = Rok2Json::Num(M, TEXT("toY"));
	E.StartMs = (int64)Rok2Json::Num(M, TEXT("startMs"));
	E.EtaMs = (int64)Rok2Json::Num(M, TEXT("etaMs"));
	E.State = Rok2Json::Str(M, TEXT("state"));
	E.TargetType = Rok2Json::Str(M, TEXT("targetType"));
	E.TargetId = Rok2Json::Str(M, TEXT("targetId"));

	E.Troops.Empty();
	const TSharedPtr<FJsonObject>* TroopsObj;
	if (M->TryGetObjectField(TEXT("troops"), TroopsObj) && TroopsObj->IsValid())
	{
		for (const auto& KV : (*TroopsObj)->Values)
		{
			E.Troops.Add(FString(KV.Key), (int32)KV.Value->AsNumber());
		}
	}

	// نوع الحمولة لمسيرات الجمع: الخادم يملأ m.payload.kind (food/wood/stone/gold)
	// — يقود صوت GatherComplete. كان E.Kind يُقرأ من بنية لا تحتوي الحقل أصلاً.
	const TSharedPtr<FJsonObject>* PayloadObj;
	if (M->TryGetObjectField(TEXT("payload"), PayloadObj) && PayloadObj->IsValid())
	{
		// الرالي لا يملك kind جمع؛ علامته السلطوية هي rallyId، فتُستبعد من إعادة التوجيه.
		E.Kind = !Rok2Json::Str(*PayloadObj, TEXT("rallyId")).IsEmpty()
			? TEXT("rally")
			: Rok2Json::Str(*PayloadObj, TEXT("kind"));
	}
}

void URok2Api::ParseScoutEntity(const TSharedPtr<FJsonObject>& S, FRok2ScoutEntity& E) const
{
	E.Id = Rok2Json::Str(S, TEXT("id"));
	E.OwnerPlayerId = Rok2Json::Str(S, TEXT("ownerPlayerId"));
	E.FromX = Rok2Json::Num(S, TEXT("fromX"));
	E.FromY = Rok2Json::Num(S, TEXT("fromY"));
	E.ToX = Rok2Json::Num(S, TEXT("toX"));
	E.ToY = Rok2Json::Num(S, TEXT("toY"));
	E.StartMs = (int64)Rok2Json::Num(S, TEXT("startMs"));
	E.EtaMs = (int64)Rok2Json::Num(S, TEXT("etaMs"));
	E.State = Rok2Json::Str(S, TEXT("state"));
}

void URok2Api::ParseAllianceRally(const TSharedPtr<FJsonObject>& R, FRok2AllianceRally& E) const
{
	E.Id = Rok2Json::Str(R, TEXT("id"));
	E.LeaderPlayerId = Rok2Json::Str(R, TEXT("leaderPlayerId"));
	E.TargetType = Rok2Json::Str(R, TEXT("targetType"));
	E.TargetId = Rok2Json::Str(R, TEXT("targetId"));
	E.Status = Rok2Json::Str(R, TEXT("status"));
	E.StartMs = (int64)Rok2Json::Num(R, TEXT("startMs"));
	E.LaunchMs = (int64)Rok2Json::Num(R, TEXT("launchMs"));
	E.MarchId = Rok2Json::Str(R, TEXT("marchId"));
	E.Participants = (int32)Rok2Json::Num(R, TEXT("participants"));
	E.bIsJoined = R->HasTypedField<EJson::Boolean>(TEXT("isJoined")) && R->GetBoolField(TEXT("isJoined"));
}

void URok2Api::UpsertMarch(const FRok2MarchEntity& E)
{
	for (int32 i = 0; i < World.Marches.Num(); ++i)
	{
		if (World.Marches[i].Id == E.Id)
		{
			// المسيرة العائدة تتحول لحركة جديدة من الهدف للمدينة — نحدّثها في مكانها
			if (E.State == TEXT("returned") || E.State == TEXT("cancelled") || E.State == TEXT("arrived"))
			{
				World.Marches.RemoveAt(i);
			}
			else
			{
				World.Marches[i] = E;
			}
			OnWorldSnapshot.Broadcast(World);
			return;
		}
	}
	if (E.State != TEXT("returned") && E.State != TEXT("cancelled") && E.State != TEXT("arrived"))
	{
		World.Marches.Add(E);
		OnWorldSnapshot.Broadcast(World);
	}
}

// ---------------------------------------------------------------------------
// Battle reports (P1-T4) — parsing + استقبال لحظي من الـ WS
// ---------------------------------------------------------------------------
void URok2Api::ParseTroopMap(const TSharedPtr<FJsonObject>& Obj, TArray<FRok2TroopLoss>& Out)
{
	Out.Empty();
	if (!Obj.IsValid()) return;
	for (const auto& KV : Obj->Values)
	{
		FRok2TroopLoss E;
		E.UnitId = FString(KV.Key);
		E.Count = (int32)KV.Value->AsNumber();
		if (E.Count > 0) Out.Add(E);
	}
	Out.Sort([](const FRok2TroopLoss& A, const FRok2TroopLoss& B) { return A.UnitId < B.UnitId; });
}

void URok2Api::ParseBattleReport(const TSharedPtr<FJsonObject>& Obj, FRok2BattleReport& Out) const
{
	Out = FRok2BattleReport();
	Out.Id = Rok2Json::Str(Obj, TEXT("id"));
	Out.CreatedAt = (int64)Rok2Json::Num(Obj, TEXT("createdAt"));
	Out.Kind = Rok2Json::Str(Obj, TEXT("kind"));
	Out.AttackerPlayerId = Rok2Json::Str(Obj, TEXT("attackerPlayerId"));
	Out.AttackerAllianceId = Rok2Json::Str(Obj, TEXT("attackerAllianceId"));
	Obj->TryGetStringField(TEXT("passId"), Out.PassId);

	const TSharedPtr<FJsonObject>* ResultObj;
	if (Obj->TryGetObjectField(TEXT("result"), ResultObj) && ResultObj->IsValid())
	{
		const TSharedPtr<FJsonObject>& R = *ResultObj;
		Out.Winner = Rok2Json::Str(R, TEXT("winner"));

		const TSharedPtr<FJsonObject>* F;
		if (R->TryGetObjectField(TEXT("attackerLosses"), F)) ParseTroopMap(*F, Out.Attacker.Losses);
		if (R->TryGetObjectField(TEXT("defenderLosses"), F)) ParseTroopMap(*F, Out.Defender.Losses);
		if (R->TryGetObjectField(TEXT("attackerRemaining"), F)) ParseTroopMap(*F, Out.Attacker.Remaining);
		if (R->TryGetObjectField(TEXT("defenderRemaining"), F)) ParseTroopMap(*F, Out.Defender.Remaining);

		const TSharedPtr<FJsonObject>* Split;
		if (R->TryGetObjectField(TEXT("attackerSplit"), Split) && Split->IsValid())
		{
			const TSharedPtr<FJsonObject>* S;
			if ((*Split)->TryGetObjectField(TEXT("dead"), S)) ParseTroopMap(*S, Out.Attacker.Dead);
			if ((*Split)->TryGetObjectField(TEXT("severely"), S)) ParseTroopMap(*S, Out.Attacker.Severely);
			if ((*Split)->TryGetObjectField(TEXT("slightly"), S)) ParseTroopMap(*S, Out.Attacker.Slightly);
		}
		if (R->TryGetObjectField(TEXT("defenderSplit"), Split) && Split->IsValid())
		{
			const TSharedPtr<FJsonObject>* S;
			if ((*Split)->TryGetObjectField(TEXT("dead"), S)) ParseTroopMap(*S, Out.Defender.Dead);
			if ((*Split)->TryGetObjectField(TEXT("severely"), S)) ParseTroopMap(*S, Out.Defender.Severely);
			if ((*Split)->TryGetObjectField(TEXT("slightly"), S)) ParseTroopMap(*S, Out.Defender.Slightly);
		}

		const TSharedPtr<FJsonObject>* PowerObj;
		if (R->TryGetObjectField(TEXT("powerBefore"), PowerObj) && PowerObj->IsValid())
		{
			Out.Attacker.PowerBefore = (int32)Rok2Json::Num((*PowerObj), TEXT("attacker"));
			Out.Defender.PowerBefore = (int32)Rok2Json::Num((*PowerObj), TEXT("defender"));
		}
	}

	const TSharedPtr<FJsonObject>* RallyObj = nullptr;
	if (Obj->TryGetObjectField(TEXT("rally"), RallyObj) && RallyObj && RallyObj->IsValid())
	{
		Out.RallyId = Rok2Json::Str(*RallyObj, TEXT("rallyId"));
		Out.RallyAllianceId = Rok2Json::Str(*RallyObj, TEXT("allianceId"));
		Out.RallyLeaderPlayerId = Rok2Json::Str(*RallyObj, TEXT("leaderPlayerId"));
		const TArray<TSharedPtr<FJsonValue>>* ParticipantArr = nullptr;
		if ((*RallyObj)->TryGetArrayField(TEXT("participants"), ParticipantArr) && ParticipantArr)
		{
			for (const TSharedPtr<FJsonValue>& Value : *ParticipantArr)
			{
				const TSharedPtr<FJsonObject> P = Value.IsValid() ? Value->AsObject() : nullptr;
				if (!P.IsValid()) continue;
				FRok2RallyReportParticipant Participant;
				Participant.PlayerId = Rok2Json::Str(P, TEXT("playerId"));
				const TSharedPtr<FJsonObject>* Field = nullptr;
				if (P->TryGetObjectField(TEXT("committed"), Field)) ParseTroopMap(*Field, Participant.Committed);
				if (P->TryGetObjectField(TEXT("remaining"), Field)) ParseTroopMap(*Field, Participant.Remaining);
				if (P->TryGetObjectField(TEXT("losses"), Field)) ParseTroopMap(*Field, Participant.Losses);
				if (P->TryGetObjectField(TEXT("dead"), Field)) ParseTroopMap(*Field, Participant.Dead);
				if (P->TryGetObjectField(TEXT("severely"), Field)) ParseTroopMap(*Field, Participant.Severely);
				if (P->TryGetObjectField(TEXT("slightly"), Field)) ParseTroopMap(*Field, Participant.Slightly);
				const TSharedPtr<FJsonObject>* Hospital = nullptr;
				if (P->TryGetObjectField(TEXT("hospital"), Hospital) && Hospital && Hospital->IsValid())
				{
					if ((*Hospital)->TryGetObjectField(TEXT("admitted"), Field)) ParseTroopMap(*Field, Participant.HospitalAdmitted);
					if ((*Hospital)->TryGetObjectField(TEXT("died"), Field)) ParseTroopMap(*Field, Participant.HospitalDied);
					Participant.HospitalCapacity = (int32)Rok2Json::Num(*Hospital, TEXT("capacity"));
				}
				Out.RallyParticipants.Add(MoveTemp(Participant));
			}
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* RewardsArr = nullptr;
	if (Obj->TryGetArrayField(TEXT("rewards"), RewardsArr) && RewardsArr)
	{
		for (const TSharedPtr<FJsonValue>& Value : *RewardsArr)
		{
			const TSharedPtr<FJsonObject> RewardObj = Value.IsValid() ? Value->AsObject() : nullptr;
			if (!RewardObj.IsValid()) continue;
			FRok2BattleReward Reward;
			Reward.Kind = Rok2Json::Str(RewardObj, TEXT("kind"));
			Reward.Amount = (int32)Rok2Json::Num(RewardObj, TEXT("amount"));
			Out.Rewards.Add(MoveTemp(Reward));
		}
	}
}

void URok2Api::AddBattleReport(const FRok2BattleReport& R)
{
	// استبدل بنفس الـ id إن وُجد ثم أدرج في المقدمة
	BattleReports.RemoveAll([&R](const FRok2BattleReport& X) { return X.Id == R.Id; });
	BattleReports.Insert(R, 0);
	if (BattleReports.Num() > 25) BattleReports.SetNum(25);
	OnBattleReports.Broadcast(BattleReports);
}

void URok2Api::LoadCity()
{
	Get(TEXT("/v1/city"), [this](const TSharedPtr<FJsonObject>& Obj)
	{
		ParseCity(Obj);
	});
}

void URok2Api::SaveCityLayout(const TArray<FRok2CityLayoutPlacement>& Placements)
{
	if (!HasPlayer() || !IsLoggedIn())
	{
		PushNotification(TEXT("toast"), TEXT("تعذّر حفظ التخطيط"), TEXT("سجّل الدخول أولاً لمزامنة القلعة"), 5.f);
		return;
	}

	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> JsonPlacements;
	for (const FRok2CityLayoutPlacement& Placement : Placements)
	{
		TSharedPtr<FJsonObject> Entry = MakeShared<FJsonObject>();
		Entry->SetStringField(TEXT("buildingId"), Placement.BuildingId);
		Entry->SetNumberField(TEXT("q"), Placement.Q);
		Entry->SetNumberField(TEXT("r"), Placement.R);
		Entry->SetNumberField(TEXT("rotationSteps"), Placement.RotationSteps);
		Entry->SetStringField(TEXT("facade"), Placement.Facade.IsEmpty() ? TEXT("standard") : Placement.Facade);
		JsonPlacements.Add(MakeShared<FJsonValueObject>(Entry));
	}
	Body->SetArrayField(TEXT("placements"), JsonPlacements);

	FString BodyStr;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyStr);
	FJsonSerializer::Serialize(Body.ToSharedRef(), Writer);

	TWeakObjectPtr<URok2Api> WeakThis(this);
	Post(TEXT("/v1/city/layout"), BodyStr, true, [WeakThis](const TSharedPtr<FJsonObject>&)
	{
		if (!WeakThis.IsValid()) return;
		URok2Api* Self = WeakThis.Get();
		Self->PushNotification(TEXT("toast"), TEXT("تم حفظ تخطيط القلعة"), TEXT("تزامنت المواقع والواجهات مع حسابك"), 5.f);
		Self->LoadCity();
	}, [WeakThis](const FString& Error)
	{
		if (!WeakThis.IsValid()) return;
		URok2Api* Self = WeakThis.Get();
		Self->PushNotification(TEXT("toast"), TEXT("تعذّر حفظ التخطيط"), Error.IsEmpty() ? TEXT("رفض الخادم التخطيط؛ أعيدت مزامنة المدينة") : Error, 6.f);
		Self->LoadCity();
	});
}

void URok2Api::FetchCommanders()
{
	// /v1/commanders يتطلب لاعباً مؤسساً. هذا الحارس يمنع طلباً 401 أثناء شاشة اختيار الحضارة.
	if (!HasPlayer() || !IsLoggedIn()) return;

	TWeakObjectPtr<URok2Api> WeakThis(this);
	Get(TEXT("/v1/commanders"), [WeakThis](const TSharedPtr<FJsonObject>& Obj)
	{
		if (!WeakThis.IsValid()) return;
		URok2Api* Self = WeakThis.Get();
		Self->Commanders.Empty();

		const TArray<TSharedPtr<FJsonValue>>* CommandersArr;
		if (Obj->TryGetArrayField(TEXT("commanders"), CommandersArr))
		{
			for (const TSharedPtr<FJsonValue>& Value : *CommandersArr)
			{
				const TSharedPtr<FJsonObject> CommanderObj = Value->AsObject();
				if (!CommanderObj.IsValid()) continue;

				FRok2Commander Commander;
				Commander.Id = Rok2Json::Str(CommanderObj, TEXT("commanderId"));
				Commander.Name = Rok2Json::Str(CommanderObj, TEXT("name"));
				Commander.Rarity = Rok2Json::Str(CommanderObj, TEXT("rarity"));
				Commander.Nation = Rok2Json::Str(CommanderObj, TEXT("nation"));
				Commander.Level = (int32)Rok2Json::Num(CommanderObj, TEXT("level"), 1);
				Commander.Xp = (int32)Rok2Json::Num(CommanderObj, TEXT("xp"));
				Commander.XpToNext = (int32)Rok2Json::Num(CommanderObj, TEXT("xpToNext"), 1000);
				Commander.Tomes = (int32)Rok2Json::Num(CommanderObj, TEXT("tomes"));

				const TArray<TSharedPtr<FJsonValue>>* SkillsArr;
				if (CommanderObj->TryGetArrayField(TEXT("skills"), SkillsArr))
				{
					for (const TSharedPtr<FJsonValue>& SkillValue : *SkillsArr)
					{
						const TSharedPtr<FJsonObject> SkillObj = SkillValue->AsObject();
						if (SkillObj.IsValid())
						{
							Commander.SkillLevels.Add((int32)Rok2Json::Num(SkillObj, TEXT("level"), 1));
						}
					}
				}

				if (!Commander.Id.IsEmpty()) Self->Commanders.Add(MoveTemp(Commander));
			}
		}

		Self->OnCommandersLoaded.Broadcast();
	});
}

void URok2Api::LevelUpCommander(const FString& CommanderId, int32 Tomes)
{
	if (CommanderId.IsEmpty() || Tomes <= 0) return;
	const FString Body = FString::Printf(TEXT("{\"commanderId\":\"%s\",\"tomes\":%d}"), *CommanderId, Tomes);
	TWeakObjectPtr<URok2Api> WeakThis(this);
	Post(TEXT("/v1/commander/levelup"), Body, true, [WeakThis](const TSharedPtr<FJsonObject>&)
	{
		if (!WeakThis.IsValid()) return;
		WeakThis->EmitToast(TEXT("تمت ترقية القائد"));
		WeakThis->FetchCommanders();
	});
}

void URok2Api::UpgradeCommanderSkill(const FString& CommanderId, int32 SkillSlot)
{
	if (CommanderId.IsEmpty() || SkillSlot < 1 || SkillSlot > 3) return;
	const FString Body = FString::Printf(TEXT("{\"commanderId\":\"%s\",\"skillSlot\":%d}"), *CommanderId, SkillSlot);
	TWeakObjectPtr<URok2Api> WeakThis(this);
	Post(TEXT("/v1/commander/skill"), Body, true, [WeakThis](const TSharedPtr<FJsonObject>&)
	{
		if (!WeakThis.IsValid()) return;
		WeakThis->EmitToast(TEXT("تمت ترقية المهارة"));
		WeakThis->FetchCommanders();
	});
}

void URok2Api::UpgradeBuilding(const FString& BuildingId)
{
	FString Body = FString::Printf(TEXT("{\"buildingId\":\"%s\"}"), *BuildingId);
	Post(TEXT("/v1/city/upgrade"), Body, true, [this](const TSharedPtr<FJsonObject>& Obj)
	{
		ParseCity(Obj);
		EmitToast(TEXT("تمت الترقية"));
		if (URok2AudioManager* Audio = URok2AudioManager::Get())
		{
			Audio->PlaySfx(ERok2AudioType::Upgrade);
		}
	});
}

	void URok2Api::SpeedupQueue(const FString& QueueId)
	{
		FinishQueueWithGems(QueueId);
	}

	void URok2Api::FinishQueueWithGems(const FString& QueueId)
	{
		if (QueueId.IsEmpty()) return;
		const FString Body = FString::Printf(TEXT("{\"queueId\":\"%s\",\"finishWithGems\":true}"), *QueueId);
		TWeakObjectPtr<URok2Api> WeakThis(this);
		Post(TEXT("/v1/shop/use-speedup"), Body, true, [WeakThis](const TSharedPtr<FJsonObject>&)
		{
			if (!WeakThis.IsValid()) return;
			URok2Api* Self = WeakThis.Get();
			Self->EmitToast(TEXT("اكتمل الطابور بالجواهر"));
			Self->LoadCity();
		});
	}

	void URok2Api::UseSpeedupItem(const FString& QueueId, const FString& ItemId)
	{
		if (QueueId.IsEmpty() || ItemId.IsEmpty()) return;
		const FString Body = FString::Printf(TEXT("{\"queueId\":\"%s\",\"itemId\":\"%s\"}"), *QueueId, *ItemId);
		TWeakObjectPtr<URok2Api> WeakThis(this);
		Post(TEXT("/v1/shop/use-speedup"), Body, true, [WeakThis](const TSharedPtr<FJsonObject>&)
		{
			if (!WeakThis.IsValid()) return;
			URok2Api* Self = WeakThis.Get();
			Self->EmitToast(TEXT("تم استخدام عنصر التسريع"));
			Self->LoadCity();
		});
	}

	void URok2Api::CollectCityProduction()
	{
		TWeakObjectPtr<URok2Api> WeakThis(this);
		Post(TEXT("/v1/city/collect"), TEXT("{}"), true, [WeakThis](const TSharedPtr<FJsonObject>& Obj)
		{
			if (!WeakThis.IsValid()) return;
			URok2Api* Self = WeakThis.Get();
			Self->ParseCity(Obj);
			Self->EmitToast(TEXT("تم تحصيل إنتاج المدينة"));
		});
	}

void URok2Api::Train(const FString& UnitId, int32 Count)
{
	FString Body = FString::Printf(TEXT("{\"unit\":\"%s\",\"count\":%d}"), *UnitId, Count);
	Post(TEXT("/v1/city/train"), Body, true, [this](const TSharedPtr<FJsonObject>& Obj)
	{
		ParseCity(Obj);
		EmitToast(TEXT("اكتمل التدريب"));
	});
}

void URok2Api::HealWounded(const TMap<FString, int32>& TroopsMap)
{
	// P4-T4: شفاء الجرحى (P2-T2 backend) + صوت HealComplete عند نجاح الطلب
	FString TroopsJson;
	{
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&TroopsJson);
		Writer->WriteObjectStart();
		for (const TPair<FString, int32>& Pair : TroopsMap)
		{
			if (Pair.Value > 0) Writer->WriteValue(Pair.Key, Pair.Value);
		}
		Writer->WriteObjectEnd();
	}
	const FString Body = FString::Printf(TEXT("{\"troops\":%s}"), *TroopsJson);
	Post(TEXT("/v1/city/heal"), Body, true, [this](const TSharedPtr<FJsonObject>& Obj)
	{
		ParseCity(Obj);
		if (URok2AudioManager* Audio = URok2AudioManager::Get())
		{
			Audio->PlaySfx(ERok2AudioType::HealComplete);
		}
		EmitToast(TEXT("بدأ شفاء الجرحى"));
	});
}

void URok2Api::CreateAlliance(const FString& Name, const FString& Tag)
{
	FString Body = FString::Printf(TEXT("{\"name\":\"%s\",\"tag\":\"%s\"}"), *Name, *Tag);
	Post(TEXT("/v1/alliance/create"), Body, true, [this](const TSharedPtr<FJsonObject>& Obj)
	{
		EmitToast(TEXT("تم إنشاء التحالف"));
		LoadCity();
	});
}

void URok2Api::BuildAllianceStructure(const FString& StructureKind, double X, double Y)
{
	if (StructureKind.IsEmpty())
	{
		EmitToast(TEXT("اختر نوع المنشأة أولاً"));
		return;
	}

	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("kind"), StructureKind);
	Body->SetNumberField(TEXT("x"), X);
	Body->SetNumberField(TEXT("y"), Y);

	FString BodyStr;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyStr);
	FJsonSerializer::Serialize(Body.ToSharedRef(), Writer);

	Post(TEXT("/v1/alliance/structure/build"), BodyStr, true, [this](const TSharedPtr<FJsonObject>& Obj)
	{
		EmitToast(TEXT("تم إنشاء منشأة التحالف"));
		RefreshWorld();
	});
}

void URok2Api::RefreshWorld()
{
	Get(TEXT("/v1/world/snapshot"), [this](const TSharedPtr<FJsonObject>& Obj)
	{
		ParseWorld(Obj);
	});
}

void URok2Api::FetchBattleReports()
{
	if (!HasPlayer() || !IsLoggedIn()) return;
	TWeakObjectPtr<URok2Api> WeakThis(this);
	Get(TEXT("/v1/combat/reports"), [WeakThis](const TSharedPtr<FJsonObject>& Obj)
	{
		if (!WeakThis.IsValid()) return;
		URok2Api* Self = WeakThis.Get();
		Self->BattleReports.Empty();
		const TArray<TSharedPtr<FJsonValue>>* ReportsArr = nullptr;
		if (Obj->TryGetArrayField(TEXT("reports"), ReportsArr) && ReportsArr)
		{
			for (const TSharedPtr<FJsonValue>& Value : *ReportsArr)
			{
				const TSharedPtr<FJsonObject> ReportObj = Value.IsValid() ? Value->AsObject() : nullptr;
				if (!ReportObj.IsValid()) continue;
				FRok2BattleReport Report;
				Self->ParseBattleReport(ReportObj, Report);
				if (!Report.Id.IsEmpty()) Self->BattleReports.Add(MoveTemp(Report));
			}
		}
		Self->OnBattleReports.Broadcast(Self->BattleReports);
	});
}

void URok2Api::AttackPass(const FString& PassId, const TMap<FString, int32>& TroopsMap)
{
	DispatchMarch(TEXT("pass"), PassId, TroopsMap, TEXT(""), TEXT(""));
}

void URok2Api::MarchTo(const FString& TargetType, const FString& TargetId, const TMap<FString, int32>& TroopsMap)
{
	DispatchMarch(TargetType, TargetId, TroopsMap, TEXT(""), TEXT(""));
}

void URok2Api::DispatchMarch(const FString& TargetType, const FString& TargetId, const TMap<FString, int32>& TroopsMap, const FString& PrimaryCommander, const FString& SecondaryCommander)
{
	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("targetType"), TargetType);
	Body->SetStringField(TEXT("targetId"), TargetId);
	// الخادم يقبل قائداً أساسياً واحداً باسم primaryCommanderId؛ لا نرسل حقلاً لا يقرأه.
	if (!PrimaryCommander.IsEmpty()) Body->SetStringField(TEXT("primaryCommanderId"), PrimaryCommander);
	if (!SecondaryCommander.IsEmpty())
	{
		UE_LOG(LogRok2, Warning, TEXT("Secondary commander is not supported by the current server contract and was not sent."));
	}

	TSharedPtr<FJsonObject> TroopsObj = MakeShared<FJsonObject>();
	for (const auto& KV : TroopsMap)
	{
		TroopsObj->SetNumberField(KV.Key, KV.Value);
	}
	Body->SetObjectField(TEXT("troops"), TroopsObj);

	FString BodyStr;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyStr);
	FJsonSerializer::Serialize(Body.ToSharedRef(), Writer);

	Post(TEXT("/v1/world/march"), BodyStr, true, [this](const TSharedPtr<FJsonObject>& Obj)
	{
		EmitToast(TEXT("انطلقت المسيرة"));
		ForceTick();
		RefreshWorld();
		LoadCity();
	});
}

void URok2Api::RedirectMarch(const FString& MarchId, const FString& TargetType, const FString& TargetId, double ToX, double ToY)
{
	if (MarchId.IsEmpty() || TargetType.IsEmpty())
	{
		EmitToast(TEXT("اختر مسيرة متحركة وهدفاً صالحاً"));
		return;
	}

	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("targetType"), TargetType);
	Body->SetStringField(TEXT("targetId"), TargetId);
	Body->SetNumberField(TEXT("toX"), ToX);
	Body->SetNumberField(TEXT("toY"), ToY);
	if (TargetType == TEXT("pass")) Body->SetStringField(TEXT("passId"), TargetId);
	if (TargetType == TEXT("core_objective")) Body->SetStringField(TEXT("coreObjectiveId"), TargetId);

	FString BodyStr;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyStr);
	FJsonSerializer::Serialize(Body.ToSharedRef(), Writer);

	Post(FString::Printf(TEXT("/v1/world/march/%s/redirect"), *FGenericPlatformHttp::UrlEncode(MarchId)), BodyStr, true,
		[this](const TSharedPtr<FJsonObject>& Obj)
		{
			PushNotification(TEXT("toast"), TEXT("تم اعتماد إعادة التوجيه"), TEXT("تتحرك المسيرة الآن نحو الهدف الجديد"), 6.f);
			RefreshWorld();
			LoadCity();
		},
		[this](const FString& Err)
		{
			const bool bNoLongerMoving = Err.Contains(TEXT("march_not_moving")) ||
				Err.Contains(TEXT("march_already_arrived")) || Err.Contains(TEXT("march_in_combat"));
			PushNotification(TEXT("toast"), TEXT("تعذّر تحويل المسيرة"),
				bNoLongerMoving ? TEXT("المسيرة لم تعد في الحركة؛ جرى تحديث الخريطة") : Err, 6.f);
			RefreshWorld();
		});
}

void URok2Api::SendScout(double ToX, double ToY)
{
	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetNumberField(TEXT("toX"), ToX);
	Body->SetNumberField(TEXT("toY"), ToY);

	FString BodyStr;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyStr);
	FJsonSerializer::Serialize(Body.ToSharedRef(), Writer);

	Post(TEXT("/v1/world/scout"), BodyStr, true, [this](const TSharedPtr<FJsonObject>& Obj)
	{
		EmitToast(TEXT("انطلقت الكشافة"));
		ForceTick();
		RefreshWorld();
	});
}

void URok2Api::LaunchAllianceRally(const FString& TargetType, const FString& TargetId, const TMap<FString, int32>& TroopsMap, const FString& PrimaryCommanderId)
{
	if ((TargetType != TEXT("pass") && TargetType != TEXT("throne")) || TargetId.IsEmpty())
	{
		EmitToast(TEXT("الرالي متاح للممرات والعرش فقط"));
		return;
	}

	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("targetType"), TargetType);
	Body->SetStringField(TEXT("targetId"), TargetId);
	if (!PrimaryCommanderId.IsEmpty()) Body->SetStringField(TEXT("primaryCommanderId"), PrimaryCommanderId);
	TSharedPtr<FJsonObject> TroopsObj = MakeShared<FJsonObject>();
	for (const TPair<FString, int32>& KV : TroopsMap)
	{
		if (KV.Value > 0) TroopsObj->SetNumberField(KV.Key, KV.Value);
	}
	if (TroopsObj->Values.Num() == 0)
	{
		EmitToast(TEXT("اختر قواتاً للرالي أولاً"));
		return;
	}
	Body->SetObjectField(TEXT("troops"), TroopsObj);

	FString BodyStr;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyStr);
	FJsonSerializer::Serialize(Body.ToSharedRef(), Writer);
	Post(TEXT("/v1/alliance/rally"), BodyStr, true, [this](const TSharedPtr<FJsonObject>& Obj)
	{
		EmitToast(TEXT("بدأ تجميع رالي التحالف"));
		FetchAllianceRallies();
		LoadCity();
	});
}

void URok2Api::JoinAllianceRally(const FString& RallyId, const TMap<FString, int32>& TroopsMap)
{
	if (RallyId.IsEmpty()) return;
	TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("rallyId"), RallyId);
	TSharedPtr<FJsonObject> TroopsObj = MakeShared<FJsonObject>();
	for (const TPair<FString, int32>& KV : TroopsMap)
	{
		if (KV.Value > 0) TroopsObj->SetNumberField(KV.Key, KV.Value);
	}
	if (TroopsObj->Values.Num() == 0)
	{
		EmitToast(TEXT("اختر قواتاً للانضمام أولاً"));
		return;
	}
	Body->SetObjectField(TEXT("troops"), TroopsObj);

	FString BodyStr;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyStr);
	FJsonSerializer::Serialize(Body.ToSharedRef(), Writer);
	Post(TEXT("/v1/alliance/rally/join"), BodyStr, true, [this](const TSharedPtr<FJsonObject>& Obj)
	{
		EmitToast(TEXT("انضممت إلى الرالي"));
		FetchAllianceRallies();
		LoadCity();
	});
}

void URok2Api::FetchAllianceRallies()
{
	if (Player.AllianceId.IsEmpty())
	{
		AllianceRallies.Empty();
		OnAllianceRalliesUpdated.Broadcast(AllianceRallies);
		return;
	}
	Get(TEXT("/v1/alliance/rallies"), [this](const TSharedPtr<FJsonObject>& Obj)
	{
		AllianceRallies.Empty();
		const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
		if (Obj->TryGetArrayField(TEXT("rallies"), Rows) && Rows)
		{
			for (const TSharedPtr<FJsonValue>& Value : *Rows)
			{
				const TSharedPtr<FJsonObject> RallyObj = Value.IsValid() ? Value->AsObject() : nullptr;
				if (!RallyObj.IsValid()) continue;
				FRok2AllianceRally Rally;
				ParseAllianceRally(RallyObj, Rally);
				AllianceRallies.Add(MoveTemp(Rally));
			}
		}
		OnAllianceRalliesUpdated.Broadcast(AllianceRallies);
	});
}

// P6-T6: إرسال رسالة دردشة عبر WebSocket
void URok2Api::SendChat(const FString& Channel, const FString& Text)
{
	if (!WebSocket.IsValid() || !bWsConnected) return;
	TSharedPtr<FJsonObject> Msg = MakeShared<FJsonObject>();
	Msg->SetStringField(TEXT("type"), TEXT("chat_send"));
	Msg->SetStringField(TEXT("channel"), Channel);
	Msg->SetStringField(TEXT("text"), Text);
	FString Str;
	const TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Str);
	FJsonSerializer::Serialize(Msg.ToSharedRef(), W);
	WebSocket->Send(Str);
}

void URok2Api::AllianceHelp()
{
	Post(TEXT("/v1/alliance/help"), TEXT("{}"), true, [this](const TSharedPtr<FJsonObject>& Obj)
	{
		EmitToast(TEXT("تم تقديم المساعدة للحلفاء"));
		LoadCity();
	});
}

// ---------------------------------------------------------------------------
// نقاط الأدمن — أدوات تطوير فقط.
//
// كان العميل يحمل المفتاح "rok2-dev-admin" نصاً صريحاً، وهو نفس المفتاح
// المنشور في wrangler.jsonc. أي شخص يفكّ الـ APK يجده. لا يُشحن مفتاح
// إداري داخل عميل لعبة أبداً.
//
// الآن: المفتاح فارغ افتراضياً، والدوال تُصفَّر في بناء الإصدار. لاستخدامها
// أثناء التطوير اضبط GameMode->AdminKey يدوياً بمفتاح خادمك.
// ---------------------------------------------------------------------------
void URok2Api::ForceTick()
{
#if UE_BUILD_SHIPPING
	UE_LOG(LogRok2, Warning, TEXT("ForceTick unavailable in shipping builds."));
#else
	if (AdminKey.IsEmpty())
	{
		UE_LOG(LogRok2, Warning, TEXT("ForceTick skipped: no admin key configured."));
		return;
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = FHttpModule::Get().CreateRequest();
	Req->SetURL(BuildUrl(TEXT("/v1/admin/tick")));
	Req->SetVerb(TEXT("POST"));
	Req->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Req->SetHeader(TEXT("x-admin-key"), AdminKey);
	Req->SetTimeout(HttpTimeoutSeconds);
	Req->SetContentAsString(TEXT("{\"force\":true}"));
	Req->ProcessRequest();
#endif
}

void URok2Api::SetSeasonDay(int32 Day)
{
#if UE_BUILD_SHIPPING
	UE_LOG(LogRok2, Warning, TEXT("SetSeasonDay unavailable in shipping builds."));
#else
	if (AdminKey.IsEmpty())
	{
		UE_LOG(LogRok2, Warning, TEXT("SetSeasonDay skipped: no admin key configured."));
		return;
	}

	FString Body = FString::Printf(TEXT("{\"day\":%d}"), Day);
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = FHttpModule::Get().CreateRequest();
	Req->SetURL(BuildUrl(TEXT("/v1/admin/set-time")));
	Req->SetVerb(TEXT("POST"));
	Req->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Req->SetHeader(TEXT("x-admin-key"), AdminKey);
	Req->SetTimeout(HttpTimeoutSeconds);
	Req->SetContentAsString(Body);
	Req->ProcessRequest();
#endif
}

// ---------------------------------------------------------------------------
// WebSocket مع إعادة اتصال تلقائي (backoff أسّي) — P1-T2
// ---------------------------------------------------------------------------
void URok2Api::ConnectWebSocket()
{
	bWsDesired = true;
	if (bWsConnected && WebSocket.IsValid()) return;

	FString WsUrl = BaseUrl;
	WsUrl.ReplaceInline(TEXT("https://"), TEXT("wss://"));
	WsUrl.ReplaceInline(TEXT("http://"), TEXT("ws://"));
	WsUrl += TEXT("/v1/world/ws");

	WebSocket = FModuleManager::LoadModuleChecked<FWebSocketsModule>(TEXT("WebSockets")).CreateWebSocket(WsUrl);

	TWeakObjectPtr<URok2Api> WeakThis(this);

	WebSocket->OnConnected().AddLambda([WeakThis]()
	{
		if (!WeakThis.IsValid()) return;
		URok2Api* Self = WeakThis.Get();
		Self->bWsConnected = true;
		Self->WsReconnectDelay = 2.f; // reset backoff on success
		Self->WsReconnectTimer = 0.f;
		Self->SetOnline(true, TEXT("اتصال حي"));
		Self->EmitToast(TEXT("اتصال حي"));
		// hello
		TSharedPtr<FJsonObject> Msg = MakeShared<FJsonObject>();
		Msg->SetStringField(TEXT("type"), TEXT("hello"));
		Msg->SetStringField(TEXT("playerId"), Self->Player.Id);
		FString Str;
		const TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Str);
		FJsonSerializer::Serialize(Msg.ToSharedRef(), W);
		Self->WebSocket->Send(Str);
	});

	WebSocket->OnMessage().AddLambda([WeakThis](const FString& Message)
	{
		if (!WeakThis.IsValid()) return;
		URok2Api* Self = WeakThis.Get();
		TSharedPtr<FJsonObject> Obj;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);
		if (!FJsonSerializer::Deserialize(Reader, Obj) || !Obj.IsValid()) return;
		FString Type;
		if (!Obj->TryGetStringField(TEXT("type"), Type)) return;

		if (Type == TEXT("snapshot") || Obj->HasField(TEXT("cities")))
		{
			Self->ParseWorld(Obj);
		}
		else if (Type == TEXT("march_created") || Type == TEXT("march_returning") || Type == TEXT("march_redirected"))
		{
			const TSharedPtr<FJsonObject>* MarchObj;
			if (Obj->TryGetObjectField(TEXT("march"), MarchObj) && MarchObj->IsValid())
			{
				FRok2MarchEntity E;
				Self->ParseMarchEntity(*MarchObj, E);
				Self->UpsertMarch(E);
if (Type == TEXT("march_created") && E.OwnerPlayerId == Self->Player.Id)
					{
						Self->EmitToast(TEXT("انطلقت المسيرة"));
					}
						else if (Type == TEXT("march_redirected") && E.OwnerPlayerId == Self->Player.Id)
						{
							// التنبيه الفوري يُعرض من استجابة الأمر؛ يبقى حدث WS لتحديث المسيرة على بقية الجلسات.
						}
						else if (Type == TEXT("march_returning") && E.OwnerPlayerId == Self->Player.Id)
						{
							Self->PushNotification(TEXT("toast"), TEXT("المسيرة عائدة"),
								TEXT("لا يمكن إعادة توجيه المسيرة أثناء العودة"), 6.f);
						}
					// P4-T4: عودة مسيرة جمع بموارد — صوت حصاد للاعب صاحب المسيرة
				if (Type == TEXT("march_returning") && E.OwnerPlayerId == Self->Player.Id &&
					(E.Kind.Contains(TEXT("gather")) || E.Kind.Contains(TEXT("node"))))
				{
					if (URok2AudioManager* Audio = URok2AudioManager::Get())
					{
						Audio->PlaySfx(ERok2AudioType::GatherComplete);
					}
				}
			}
		}
			else if (Type == TEXT("march_arrived"))
			{
				const TSharedPtr<FJsonObject>* MarchObj;
				if (Obj->TryGetObjectField(TEXT("march"), MarchObj) && MarchObj->IsValid())
				{
					FRok2MarchEntity E;
					Self->ParseMarchEntity(*MarchObj, E);
					if (E.OwnerPlayerId == Self->Player.Id)
					{
						Self->PushNotification(TEXT("toast"), TEXT("وصلت المسيرة"),
							FString::Printf(TEXT("وصلت مسيرتك إلى %s"), *E.TargetId), 7.f);
						Self->RefreshWorld();
					}
				}
			}
			else if (Type == TEXT("march_update"))
			{
			// تحديث تقدم خفيف — نحدّث الـ ETA فقط إن تغيّر (الحركة الفعلية تُحسب من startMs/etaMs)
			const TArray<TSharedPtr<FJsonValue>>* Arr;
			if (Obj->TryGetArrayField(TEXT("marches"), Arr))
			{
				for (const auto& V : *Arr)
				{
					const TSharedPtr<FJsonObject> MU = V->AsObject();
					if (!MU.IsValid()) continue;
					const FString MId = Rok2Json::Str(MU, TEXT("id"));
					for (FRok2MarchEntity& M : Self->World.Marches)
					{
						if (M.Id == MId)
						{
							M.EtaMs = (int64)Rok2Json::Num(MU, TEXT("etaMs"));
							break;
						}
					}
				}
			}
		}
		else if (Type == TEXT("city_upsert"))
		{
			// تحديث مدينة على الخريطة/الموارد (مثلاً ترقية قاعة المدينة) — مزامنة فورية (P1-T5)
			Self->LoadCity();
			Self->RefreshWorld();
		}
		else if (Type == TEXT("pass_owner_changed"))
		{
			Self->EmitToast(TEXT("تغير مالك ممر!"));
			Self->RefreshWorld();
		}
		else if (Type == TEXT("battle_report"))
		{
			Self->EmitToast(TEXT("تقرير قتال جديد"));
			const TSharedPtr<FJsonObject>* ReportObj;
			if (Obj->TryGetObjectField(TEXT("report"), ReportObj) && ReportObj->IsValid())
			{
				FRok2BattleReport Report;
				Self->ParseBattleReport(*ReportObj, Report);
				Self->AddBattleReport(Report);
				// P5-T6: صوت نصر/هزيمة + P4-T3: تبديل لموسيقى القتال
				if (URok2AudioManager* Audio = URok2AudioManager::Get())
				{
					Audio->EnterBattleMode();
					const bool bVictory = (Report.Winner == TEXT("attacker") && Report.AttackerPlayerId == Self->Player.Id) ||
						(Report.Winner == TEXT("defender") && Report.AttackerPlayerId != Self->Player.Id);
					Audio->PlaySfx(bVictory ? ERok2AudioType::BattleVictory : ERok2AudioType::BattleDefeat);
				}
				Self->PushNotification(TEXT("combat"), TEXT("تقرير قتال"),
					FString::Printf(TEXT("%s — %s"), *Report.Kind, *Report.Winner), 8.f);
			}
			else
			{
				Self->RefreshWorld();
			}
		}
		else if (Type == TEXT("zone_unlocked"))
		{
			// فتح منطقة جديدة (P2-T4) — إشعار بارز في الـ HUD (P2-T6)
			const FString RegionId = Rok2Json::Str(Obj, TEXT("regionId"));
			const int32 ZoneId = (int32)Rok2Json::Num(Obj, TEXT("zoneId"));
			Self->PushNotification(TEXT("zone"), TEXT("انفتحت منطقة جديدة"),
				FString::Printf(TEXT("Zone %d — %s أصبحت متاحة الآن"), ZoneId, *RegionId), 10.f);
			if (URok2AudioManager* Audio = URok2AudioManager::Get())
			{
				Audio->PlaySfx(ERok2AudioType::ZoneUnlock); // P4-T4
			}
			Self->RefreshWorld();
		}
		else if (Type == TEXT("tech_researched"))
		{
			// اكتمال بحث (P2-T3) — إشعار + مزامنة
			const FString TechId = Rok2Json::Str(Obj, TEXT("techId"));
			const int32 Level = (int32)Rok2Json::Num(Obj, TEXT("level"));
			Self->PushNotification(TEXT("toast"), TEXT("اكتمل البحث"),
				FString::Printf(TEXT("%s → مستوى %d"), *TechId, Level), 6.f);
			if (URok2AudioManager* Audio = URok2AudioManager::Get())
			{
				Audio->PlaySfx(ERok2AudioType::ResearchComplete); // P4-T4
			}
			Self->LoadCity();
		}
		else if (Type == TEXT("rally_launched"))
		{
			// انطلاق حملة rally (P2-T5)
			const FString TargetId = Rok2Json::Str(Obj, TEXT("targetId"));
			Self->PushNotification(TEXT("rally"), TEXT("انطلقت حملة التحالف"),
				FString::Printf(TEXT("rally على %s"), *TargetId), 8.f);
			if (URok2AudioManager* Audio = URok2AudioManager::Get())
			{
				Audio->PlaySfx(ERok2AudioType::RallyLaunch); // P4-T4
			}
			Self->RefreshWorld();
		}
		else if (Type == TEXT("season_day"))
		{
			const int32 Day = (int32)Rok2Json::Num(Obj, TEXT("day"));
			Self->World.SeasonDay = Day;
			Self->PushNotification(TEXT("zone"), TEXT("يوم جديد في الموسم"),
				FString::Printf(TEXT("اليوم %d"), Day), 5.f);
		}
		else if (Type == TEXT("scout_arrived"))
		{
			// وصول كشافة (P5-T5) — إشعار + كشف منطقة
			const FString ScoutId = Rok2Json::Str(Obj, TEXT("scoutId"));
			const double ToX = Rok2Json::Num(Obj, TEXT("toX"));
			const double ToY = Rok2Json::Num(Obj, TEXT("toY"));
			Self->PushNotification(TEXT("toast"), TEXT("عادت الكشافة"),
				FString::Printf(TEXT("كشفت المنطقة حول (%.0f, %.0f)"), ToX, ToY), 6.f);
		}
		// P6-T6: رسالة دردشة جديدة
		else if (Type == TEXT("chat_message"))
		{
			const TSharedPtr<FJsonObject>& MsgObj = Obj->GetObjectField(TEXT("message"));
			if (MsgObj.IsValid())
			{
				FRok2ChatMessage ChatMsg;
				ChatMsg.Id = Rok2Json::Str(MsgObj, TEXT("id"));
				ChatMsg.Channel = Rok2Json::Str(MsgObj, TEXT("channel"));
				ChatMsg.PlayerId = Rok2Json::Str(MsgObj, TEXT("playerId"));
				ChatMsg.PlayerName = Rok2Json::Str(MsgObj, TEXT("playerName"));
				ChatMsg.Civ = Rok2Json::Str(MsgObj, TEXT("civ"));
				ChatMsg.Text = Rok2Json::Str(MsgObj, TEXT("text"));
				ChatMsg.TimestampMs = (int64)Rok2Json::Num(MsgObj, TEXT("timestampMs"));
				Self->PushChatMessage(ChatMsg);
			}
		}
		// P6-T6: سجل الدردشة (عند الاتصال الأولي)
		else if (Type == TEXT("chat_history"))
		{
			const TArray<TSharedPtr<FJsonValue>>* Arr;
			if (Obj->TryGetArrayField(TEXT("messages"), Arr))
			{
				for (const auto& Val : *Arr)
				{
					const TSharedPtr<FJsonObject>& MsgObj = Val->AsObject();
					if (!MsgObj.IsValid()) continue;
					FRok2ChatMessage ChatMsg;
					ChatMsg.Id = Rok2Json::Str(MsgObj, TEXT("id"));
					ChatMsg.Channel = Rok2Json::Str(MsgObj, TEXT("channel"));
					ChatMsg.PlayerId = Rok2Json::Str(MsgObj, TEXT("playerId"));
					ChatMsg.PlayerName = Rok2Json::Str(MsgObj, TEXT("playerName"));
					ChatMsg.Civ = Rok2Json::Str(MsgObj, TEXT("civ"));
					ChatMsg.Text = Rok2Json::Str(MsgObj, TEXT("text"));
					ChatMsg.TimestampMs = (int64)Rok2Json::Num(MsgObj, TEXT("timestampMs"));
					Self->ChatHistory.Add(ChatMsg);
				}
				if (Self->ChatHistory.Num() > 100) Self->ChatHistory.RemoveAt(0, Self->ChatHistory.Num() - 100);
			}
		}
	});

	WebSocket->OnConnectionError().AddLambda([WeakThis](const FString& Err)
	{
		if (!WeakThis.IsValid()) return;
		URok2Api* Self = WeakThis.Get();
		UE_LOG(LogRok2, Error, TEXT("WS error: %s"), *Err);
		Self->bWsConnected = false;
		Self->SetOnline(false, TEXT("خطأ في الاتصال الحي — إعادة المحاولة تلقائياً..."));
		// سيُعاد الاتصال من PumpEvents بعد WsReconnectDelay
	});

	WebSocket->OnClosed().AddLambda([WeakThis](int32 Code, const FString& Reason, bool bWasClean)
	{
		if (!WeakThis.IsValid()) return;
		URok2Api* Self = WeakThis.Get();
		Self->bWsConnected = false;
		if (Self->bWsDesired)
		{
			Self->SetOnline(false, FString::Printf(TEXT("انقطع الاتصال الحي — إعادة خلال %.0f ث"), Self->WsReconnectDelay));
		}
	});

	WebSocket->Connect();
}

void URok2Api::DisconnectWebSocket()
{
	bWsDesired = false; // لا إعادة اتصال بعد الفصل اليدوي
	if (WebSocket.IsValid())
	{
		WebSocket->Close();
		WebSocket.Reset();
	}
	bWsConnected = false;
}

// ---------------------------------------------------------------------------
// HUD الموحد (P2-T6): سجل إشعارات مركزي يغذي بطاقات الإشعارات وعدّاد الجرس
// ---------------------------------------------------------------------------
void URok2Api::PushNotification(const FString& Kind, const FString& Title, const FString& Body, float TtlSeconds)
{
	FRok2HudNotification N;
	N.Id = FString::Printf(TEXT("ntf_%d"), ++NotificationSeq);
	N.Kind = Kind;
	N.Title = Title;
	N.Body = Body;
	N.TtlSeconds = TtlSeconds;
	N.CreatedAtUtcMs = FDateTime::UtcNow().ToUnixTimestamp() * 1000;
	Notifications.Insert(N, 0);
	if (Notifications.Num() > 20) Notifications.SetNum(20);
	UnreadNotifications++;
	OnHudNotification.Broadcast(N);
}

// P6-T6: إضافة رسالة دردشة ويبثها
void URok2Api::PushChatMessage(const FRok2ChatMessage& Msg)
{
	ChatHistory.Add(Msg);
	if (ChatHistory.Num() > 100) ChatHistory.RemoveAt(0);
	UnreadChatCount++;
	OnChatMessage.Broadcast(Msg);
}

void URok2Api::PumpEvents(float DeltaSeconds)
{
	// 1) إعادة اتصال WebSocket التلقائية بـ backoff أسّي
	if (bWsDesired && !bWsConnected)
	{
		WsReconnectTimer += DeltaSeconds;
		if (WsReconnectTimer >= WsReconnectDelay)
		{
			WsReconnectTimer = 0.f;
			UE_LOG(LogRok2, Log, TEXT("WS reconnect attempt (delay was %.0fs)"), WsReconnectDelay);
			ConnectWebSocket();
			// backoff للمحاولة القادمة في حال الفشل
			WsReconnectDelay = FMath::Min(WsReconnectDelay * 2.f, WsReconnectMaxDelay);
		}
	}

	// 2) Periodic world refresh if WS not connected (fallback polling)
	if (!bWsConnected)
	{
		WorldPollTimer += DeltaSeconds;
		if (WorldPollTimer >= 4.0f)
		{
			WorldPollTimer = 0.f;
			RefreshWorld();
		}
	}
	// 3) WS متصل: مزامنة المدينة (موارد + طوابير) كل CitySyncIntervalSeconds بدون polling يدوي (P1-T5)
	else if (HasPlayer())
	{
		CitySyncTimer += DeltaSeconds;
		if (CitySyncTimer >= CitySyncIntervalSeconds)
		{
			CitySyncTimer = 0.f;
			LoadCity();
		}
	}
}
