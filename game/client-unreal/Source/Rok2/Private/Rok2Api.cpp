// Copyright ROK2. Cloudflare API + WebSocket client impl.

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

void URok2Api::Get(const FString& Path, TFunction<void(const TSharedPtr<FJsonObject>&)> OnOk)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = FHttpModule::Get().CreateRequest();
	Req->SetURL(BuildUrl(Path));
	Req->SetVerb(TEXT("GET"));
	Req->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	if (!Token.IsEmpty()) Req->SetHeader(TEXT("Authorization"), AuthHeader());
	Req->OnProcessRequestComplete().BindLambda([OnOk](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
	{
		if (!bSuccess || !Resp.IsValid())
		{
			UE_LOG(LogRok2, Error, TEXT("GET failed: %s"), *Req->GetURL());
			return;
		}
		int32 Code = Resp->GetResponseCode();
		FString Body = Resp->GetContentAsString();
		TSharedPtr<FJsonObject> Obj;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
		if (!FJsonSerializer::Deserialize(Reader, Obj) || !Obj.IsValid())
		{
			UE_LOG(LogRok2, Error, TEXT("GET bad json (%d): %s"), Code, *Body.Left(200));
			return;
		}
		if (Code >= 400)
		{
			FString Err;
			Obj->TryGetStringField(TEXT("error"), Err);
			UE_LOG(LogRok2, Warning, TEXT("GET %s -> %d %s"), *Req->GetURL(), Code, *Err);
			return;
		}
		OnOk(Obj);
	});
	Req->ProcessRequest();
}

void URok2Api::Post(const FString& Path, const FString& JsonBody, bool bAuth,
	TFunction<void(const TSharedPtr<FJsonObject>&)> OnOk, TFunction<void(const FString&)> OnErr)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = FHttpModule::Get().CreateRequest();
	Req->SetURL(BuildUrl(Path));
	Req->SetVerb(TEXT("POST"));
	Req->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	if (bAuth && !Token.IsEmpty()) Req->SetHeader(TEXT("Authorization"), AuthHeader());
	Req->SetContentAsString(JsonBody);
	Req->OnProcessRequestComplete().BindLambda([OnOk, OnErr](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
	{
		FString Body = Resp.IsValid() ? Resp->GetContentAsString() : FString();
		int32 Code = Resp.IsValid() ? Resp->GetResponseCode() : 0;
		TSharedPtr<FJsonObject> Obj;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
		bool bParsed = FJsonSerializer::Deserialize(Reader, Obj);
		if (!bSuccess || !bParsed || !Obj.IsValid() || Code >= 400)
		{
			FString Err;
			if (Obj.IsValid()) Obj->TryGetStringField(TEXT("error"), Err);
			if (Err.IsEmpty()) Err = FString::Printf(TEXT("HTTP %d"), Code);
			UE_LOG(LogRok2, Warning, TEXT("POST %s -> %s"), *Req->GetURL(), *Err);
			if (OnErr) OnErr(Err);
			return;
		}
		OnOk(Obj);
	});
	Req->ProcessRequest();
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

	OnWorldSnapshot.Broadcast(World);
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
	Req->SetContentAsString(Body);
	Req->ProcessRequest();
}

void URok2Api::ConnectWebSocket()
{
	if (bWsConnected && WebSocket.IsValid()) return;

	FString WsUrl = BaseUrl;
	WsUrl.ReplaceInline(TEXT("https://"), TEXT("wss://"));
	WsUrl.ReplaceInline(TEXT("http://"), TEXT("ws://"));
	WsUrl += TEXT("/v1/world/ws");

	WebSocket = FModuleManager::LoadModuleChecked<FWebSocketsModule>(TEXT("WebSockets")).CreateWebSocket(WsUrl);

	WebSocket->OnConnected().AddLambda([this]()
	{
		bWsConnected = true;
		EmitToast(TEXT("اتصال حي ✓"));
		// hello
		TSharedPtr<FJsonObject> Msg = MakeShared<FJsonObject>();
		Msg->SetStringField(TEXT("type"), TEXT("hello"));
		Msg->SetStringField(TEXT("playerId"), Player.Id);
		FString Str;
		const TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Str);
		FJsonSerializer::Serialize(Msg.ToSharedRef(), W);
		WebSocket->Send(Str);
	});

	WebSocket->OnMessage().AddLambda([this](const FString& Message)
	{
		TSharedPtr<FJsonObject> Obj;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);
		if (!FJsonSerializer::Deserialize(Reader, Obj) || !Obj.IsValid()) return;
		FString Type;
		if (!Obj->TryGetStringField(TEXT("type"), Type)) return;

		if (Type == TEXT("snapshot") || Obj->HasField(TEXT("cities")))
		{
			ParseWorld(Obj);
		}
		else if (Type == TEXT("pass_owner_changed"))
		{
			EmitToast(TEXT("تغير مالك ممر!"));
			RefreshWorld();
		}
		else if (Type == TEXT("battle_report"))
		{
			EmitToast(TEXT("تقرير قتال جديد"));
			RefreshWorld();
		}
	});

	WebSocket->OnConnectionError().AddLambda([this](const FString& Err)
	{
		UE_LOG(LogRok2, Error, TEXT("WS error: %s"), *Err);
		EmitToast(TEXT("WS خطأ"));
	});

	WebSocket->OnClosed().AddLambda([this](int32 Code, const FString& Reason, bool bWasClean)
	{
		bWsConnected = false;
	});

	WebSocket->Connect();
}

void URok2Api::DisconnectWebSocket()
{
	if (WebSocket.IsValid())
	{
		WebSocket->Close();
		WebSocket.Reset();
	}
	bWsConnected = false;
}

void URok2Api::PumpEvents(float DeltaSeconds)
{
	// Periodic world refresh if WS not connected (fallback polling)
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
