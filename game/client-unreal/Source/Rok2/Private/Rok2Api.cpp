// Copyright ROK2. Cloudflare API + WebSocket client impl.
// P1-T2: معالجة أخطاء الشبكة + إعادة الاتصال التلقائي + بث حالة الاتصال للواجهات.

#include "Rok2Api.h"
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

void URok2Api::Init(const FString& ApiBaseUrl, const FString& InKingdomId, const FString& InAdminKey)
{
	BaseUrl = ApiBaseUrl;
	KingdomId = InKingdomId;
	AdminKey = InAdminKey;
	DeviceId = TEXT("ue5_dev_") + FGuid::NewGuid().ToString();
	
	Commanders.Add({TEXT("cmd_sun_tzu"), TEXT("Sun Tzu"), TEXT("Epic"), TEXT("China"), {TEXT("Infantry"), TEXT("Garrison"), TEXT("Skill")}});
	Commanders.Add({TEXT("cmd_richard_1"), TEXT("Richard I"), TEXT("Legendary"), TEXT("Britain"), {TEXT("Infantry"), TEXT("Garrison"), TEXT("Defense")}});
	Commanders.Add({TEXT("cmd_joan"), TEXT("Joan of Arc"), TEXT("Epic"), TEXT("France"), {TEXT("Integration"), TEXT("Gathering"), TEXT("Support")}});

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

	UE_LOG(LogRok2, Log, TEXT("Rok2Api init: %s device=%s"), *BaseUrl, *DeviceId);
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

static void Rok2SendRequest(URok2Api* Self, TSharedPtr<FRok2RetryCtx> Ctx)
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
		Self->LoadCity();
	});
}

void URok2Api::ParsePlayer(const TSharedPtr<FJsonObject>& Obj)
{
	Player.Id = Obj->GetStringField(TEXT("id"));
	Player.Name = Obj->GetStringField(TEXT("name"));
	Player.Civ = Obj->GetStringField(TEXT("civ"));
	Player.AllianceId = Obj->GetStringField(TEXT("allianceId"));
	Player.RegionId = Obj->GetStringField(TEXT("regionId"));
	Player.X = Obj->GetNumberField(TEXT("x"));
	Player.Y = Obj->GetNumberField(TEXT("y"));
	Player.Power = (int32)Obj->GetNumberField(TEXT("power"));
}

void URok2Api::ParseCity(const TSharedPtr<FJsonObject>& Obj)
{
	const TSharedPtr<FJsonObject>* CityObj;
	if (Obj->TryGetObjectField(TEXT("city"), CityObj) && CityObj->IsValid())
	{
		City.HallLevel = (int32)(*CityObj)->GetNumberField(TEXT("hall_level"));
		City.Resources.Food = (*CityObj)->GetNumberField(TEXT("food"));
		City.Resources.Wood = (*CityObj)->GetNumberField(TEXT("wood"));
		City.Resources.Stone = (*CityObj)->GetNumberField(TEXT("stone"));
		City.Resources.Gold = (*CityObj)->GetNumberField(TEXT("gold"));
		City.UpdatedAt = (int64)(*CityObj)->GetNumberField(TEXT("updated_at"));
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

	// queues
	City.ActiveQueues.Empty();
	const TArray<TSharedPtr<FJsonValue>>* QueuesArr;
	if (Obj->TryGetArrayField(TEXT("active_queues"), QueuesArr))
	{
		for (const auto& V : *QueuesArr)
		{
			const TSharedPtr<FJsonObject> Q = V->AsObject();
			if (!Q.IsValid()) continue;
			FRok2QueueEntry Entry;
			Entry.Id = Q->GetStringField(TEXT("id"));
			Entry.Type = Q->GetStringField(TEXT("type"));
			Entry.RefId = Q->GetStringField(TEXT("refId"));
			Entry.Level = (int32)Q->GetNumberField(TEXT("level"));
			Entry.StartMs = (int64)Q->GetNumberField(TEXT("startMs"));
			Entry.EndMs = (int64)Q->GetNumberField(TEXT("endMs"));
			City.ActiveQueues.Add(Entry);
		}
	}

	OnCityLoaded.Broadcast(City);
}

void URok2Api::ParseWorld(const TSharedPtr<FJsonObject>& Obj)
{
	World.SeasonDay = (int32)Obj->GetNumberField(TEXT("seasonDay"));

	World.Cities.Empty();
	const TArray<TSharedPtr<FJsonValue>>* CitiesArr;
	if (Obj->TryGetArrayField(TEXT("cities"), CitiesArr))
	{
		for (const auto& V : *CitiesArr)
		{
			const TSharedPtr<FJsonObject> C = V->AsObject();
			if (!C.IsValid()) continue;
			FRok2CityEntity E;
			E.PlayerId = C->GetStringField(TEXT("playerId"));
			E.Name = C->GetStringField(TEXT("name"));
			E.AllianceId = C->GetStringField(TEXT("allianceId"));
			E.X = C->GetNumberField(TEXT("x"));
			E.Y = C->GetNumberField(TEXT("y"));
			E.HallLevel = (int32)C->GetNumberField(TEXT("hallLevel"));
			E.RegionId = C->GetStringField(TEXT("regionId"));
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
			E.Id = P->GetStringField(TEXT("id"));
			E.OwnerAllianceId = P->GetStringField(TEXT("ownerAllianceId"));
			E.CaptureProgress = P->GetNumberField(TEXT("captureProgress"));
			E.State = P->GetStringField(TEXT("state"));
			E.Level = (int32)P->GetNumberField(TEXT("level"));
			E.From = P->GetStringField(TEXT("from"));
			E.To = P->GetStringField(TEXT("to"));
			E.X = P->GetNumberField(TEXT("x"));
			E.Y = P->GetNumberField(TEXT("y"));
			E.UnlockDay = (int32)P->GetNumberField(TEXT("unlockDay"));
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
			E.Id = N->GetStringField(TEXT("id"));
			E.Kind = N->GetStringField(TEXT("kind"));
			E.Level = (int32)N->GetNumberField(TEXT("level"));
			E.X = N->GetNumberField(TEXT("x"));
			E.Y = N->GetNumberField(TEXT("y"));
			E.Remaining = N->GetNumberField(TEXT("remaining"));
			World.Nodes.Add(E);
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

	OnWorldSnapshot.Broadcast(World);
}

// ---------------------------------------------------------------------------
// Marches (P1-T3) — parsing + upsert من أحداث الـ WS
// ---------------------------------------------------------------------------
void URok2Api::ParseMarchEntity(const TSharedPtr<FJsonObject>& M, FRok2MarchEntity& E) const
{
	E.Id = M->GetStringField(TEXT("id"));
	E.OwnerPlayerId = M->GetStringField(TEXT("ownerPlayerId"));
	E.AllianceId = M->GetStringField(TEXT("allianceId"));
	E.FromX = M->GetNumberField(TEXT("fromX"));
	E.FromY = M->GetNumberField(TEXT("fromY"));
	E.ToX = M->GetNumberField(TEXT("toX"));
	E.ToY = M->GetNumberField(TEXT("toY"));
	E.StartMs = (int64)M->GetNumberField(TEXT("startMs"));
	E.EtaMs = (int64)M->GetNumberField(TEXT("etaMs"));
	E.State = M->GetStringField(TEXT("state"));
	E.TargetType = M->GetStringField(TEXT("targetType"));
	E.TargetId = M->GetStringField(TEXT("targetId"));

	E.Troops.Empty();
	const TSharedPtr<FJsonObject>* TroopsObj;
	if (M->TryGetObjectField(TEXT("troops"), TroopsObj) && TroopsObj->IsValid())
	{
		for (const auto& KV : (*TroopsObj)->Values)
		{
			E.Troops.Add(FString(KV.Key), (int32)KV.Value->AsNumber());
		}
	}
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
	Out.Id = Obj->GetStringField(TEXT("id"));
	Out.CreatedAt = (int64)Obj->GetNumberField(TEXT("createdAt"));
	Out.Kind = Obj->GetStringField(TEXT("kind"));
	Out.AttackerPlayerId = Obj->GetStringField(TEXT("attackerPlayerId"));
	Out.AttackerAllianceId = Obj->GetStringField(TEXT("attackerAllianceId"));
	Obj->TryGetStringField(TEXT("passId"), Out.PassId);

	const TSharedPtr<FJsonObject>* ResultObj;
	if (Obj->TryGetObjectField(TEXT("result"), ResultObj) && ResultObj->IsValid())
	{
		const TSharedPtr<FJsonObject>& R = *ResultObj;
		Out.Winner = R->GetStringField(TEXT("winner"));

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
			Out.Attacker.PowerBefore = (int32)(*PowerObj)->GetNumberField(TEXT("attacker"));
			Out.Defender.PowerBefore = (int32)(*PowerObj)->GetNumberField(TEXT("defender"));
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

void URok2Api::UpgradeBuilding(const FString& BuildingId)
{
	FString Body = FString::Printf(TEXT("{\"buildingId\":\"%s\"}"), *BuildingId);
	Post(TEXT("/v1/city/upgrade"), Body, true, [this](const TSharedPtr<FJsonObject>& Obj)
	{
		ParseCity(Obj);
		EmitToast(TEXT("تمت الترقية"));
	});
}

void URok2Api::SpeedupQueue(const FString& QueueId)
{
	FString Body = FString::Printf(TEXT("{\"queueId\":\"%s\"}"), *QueueId);
	Post(TEXT("/v1/city/speedup"), Body, true, [this](const TSharedPtr<FJsonObject>& Obj)
	{
		ParseCity(Obj);
		EmitToast(TEXT("تم التسريع"));
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

void URok2Api::CreateAlliance(const FString& Name, const FString& Tag)
{
	FString Body = FString::Printf(TEXT("{\"name\":\"%s\",\"tag\":\"%s\"}"), *Name, *Tag);
	Post(TEXT("/v1/alliance/create"), Body, true, [this](const TSharedPtr<FJsonObject>& Obj)
	{
		EmitToast(TEXT("تم إنشاء التحالف"));
		LoadCity();
	});
}

void URok2Api::RefreshWorld()
{
	Get(TEXT("/v1/world/snapshot"), [this](const TSharedPtr<FJsonObject>& Obj)
	{
		ParseWorld(Obj);
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
	if (!PrimaryCommander.IsEmpty()) Body->SetStringField(TEXT("primaryCommander"), PrimaryCommander);
	if (!SecondaryCommander.IsEmpty()) Body->SetStringField(TEXT("secondaryCommander"), SecondaryCommander);

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

void URok2Api::AllianceHelp()
{
	Post(TEXT("/v1/alliance/help"), TEXT("{}"), true, [this](const TSharedPtr<FJsonObject>& Obj)
	{
		EmitToast(TEXT("تم تقديم المساعدة للحلفاء"));
		LoadCity();
	});
}

void URok2Api::ForceTick()
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = FHttpModule::Get().CreateRequest();
	Req->SetURL(BuildUrl(TEXT("/v1/admin/tick")));
	Req->SetVerb(TEXT("POST"));
	Req->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Req->SetHeader(TEXT("x-admin-key"), AdminKey);
	Req->SetTimeout(HttpTimeoutSeconds);
	Req->SetContentAsString(TEXT("{\"force\":true}"));
	Req->ProcessRequest();
}

void URok2Api::SetSeasonDay(int32 Day)
{
	FString Body = FString::Printf(TEXT("{\"day\":%d}"), Day);
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = FHttpModule::Get().CreateRequest();
	Req->SetURL(BuildUrl(TEXT("/v1/admin/set-time")));
	Req->SetVerb(TEXT("POST"));
	Req->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Req->SetHeader(TEXT("x-admin-key"), AdminKey);
	Req->SetTimeout(HttpTimeoutSeconds);
	Req->SetContentAsString(Body);
	Req->ProcessRequest();
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
		Self->SetOnline(true, TEXT("اتصال حي ✓"));
		Self->EmitToast(TEXT("اتصال حي ✓"));
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
		else if (Type == TEXT("march_created") || Type == TEXT("march_returning"))
		{
			const TSharedPtr<FJsonObject>* MarchObj;
			if (Obj->TryGetObjectField(TEXT("march"), MarchObj) && MarchObj->IsValid())
			{
				FRok2MarchEntity E;
				Self->ParseMarchEntity(*MarchObj, E);
				Self->UpsertMarch(E);
				if (Type == TEXT("march_created") && E.OwnerPlayerId == Self->Player.Id)
				{
					Self->EmitToast(TEXT("انطلقت المسيرة ⚔️"));
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
					const FString MId = MU->GetStringField(TEXT("id"));
					for (FRok2MarchEntity& M : Self->World.Marches)
					{
						if (M.Id == MId)
						{
							M.EtaMs = (int64)MU->GetNumberField(TEXT("etaMs"));
							break;
						}
					}
				}
			}
		}
		else if (Type == TEXT("pass_owner_changed"))
		{
			Self->EmitToast(TEXT("تغير مالك ممر!"));
			Self->RefreshWorld();
		}
		else if (Type == TEXT("battle_report"))
		{
			Self->EmitToast(TEXT("تقرير قتال جديد ⚔️"));
			const TSharedPtr<FJsonObject>* ReportObj;
			if (Obj->TryGetObjectField(TEXT("report"), ReportObj) && ReportObj->IsValid())
			{
				FRok2BattleReport Report;
				Self->ParseBattleReport(*ReportObj, Report);
				Self->AddBattleReport(Report);
			}
			else
			{
				Self->RefreshWorld();
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
}
