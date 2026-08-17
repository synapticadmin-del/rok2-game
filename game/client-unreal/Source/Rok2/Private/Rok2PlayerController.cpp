// Copyright ROK2. Player controller impl.

#include "Rok2PlayerController.h"
#include "Rok2CityBuilder.h"
#include "Rok2ExitConfirmWidget.h"
#include "Rok2GameMode.h"
#include "Rok2IsometricCamera.h"
#include "Rok2Api.h"
#include "Rok2Types.h"
#include "Rok2UiStack.h"
#include "Rok2ViewManager.h"
#include "EngineUtils.h"
#include "Rok2MarchPanel.h"
#include "Rok2WorldRenderer.h"
#include "Kismet/GameplayStatics.h"
#include "Rok2BlueprintLibrary.h"
#include "Blueprint/UserWidget.h"

ARok2PlayerController::ARok2PlayerController()
{
	// بدون هذا لا تصل أحداث اللمس إلى InputComponent على أندرويد.
	bEnableTouchEvents = true;
	bEnableTouchOverEvents = false;

#if PLATFORM_ANDROID || PLATFORM_IOS
	// لا مؤشر على الهاتف — إظهاره يرسم سهماً عالقاً في زاوية الشاشة.
	bShowMouseCursor = false;
#else
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
#endif
}

void ARok2PlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (ARok2GameMode* GM = Cast<ARok2GameMode>(GetWorld()->GetAuthGameMode()))
	{
		Api = GM->Api;
	}

	// Spawn iso camera if not in level
	if (!IsoCamera)
	{
		FVector Loc(0, 0, 1500);
		FRotator Rot(-50.f, 0.f, 0.f);
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		UClass* SpawnClass = CameraClass ? *CameraClass : ARok2IsometricCamera::StaticClass();
		IsoCamera = GetWorld()->SpawnActor<ARok2IsometricCamera>(SpawnClass, Loc, Rot, P);
		if (IsoCamera)
		{
			SetViewTargetWithBlend(IsoCamera, 0.f);
		}
	}
}

void ARok2PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindAxis(TEXT("MoveRight"), this, &ARok2PlayerController::OnPanX);
	InputComponent->BindAxis(TEXT("MoveForward"), this, &ARok2PlayerController::OnPanY);
	InputComponent->BindAxis(TEXT("Zoom"), this, &ARok2PlayerController::OnZoom);
	InputComponent->BindAction(TEXT("Tap"), IE_Pressed, this, &ARok2PlayerController::OnTap);
	InputComponent->BindAction(TEXT("Escape"), IE_Pressed, this, &ARok2PlayerController::OnEscape);

	// P18-T5: زر الرجوع على أندرويد. `FAndroidPlatformInput::GetKeyMap` يسجّل
	// `AKEYCODE_BACK` باسمين (`Escape` في خريطة المحارف و`Android_Back` في
	// خريطة المفاتيح)، فأي منهما قد يصل حسب مسار الحدث. نربط المفتاح صريحاً
	// إلى جانب `ActionMappings` كي لا يعتمد السلوك على أي المسارين فاز؛
	// والوصول المزدوج يمتصّه حارس `BackDebounceSeconds`.
	InputComponent->BindKey(EKeys::Android_Back, IE_Pressed, this, &ARok2PlayerController::OnAndroidBack);

	// اللمس — المسار الوحيد الفعّال على أندرويد.
	InputComponent->BindTouch(IE_Pressed, this, &ARok2PlayerController::OnTouchBegin);
	InputComponent->BindTouch(IE_Repeat, this, &ARok2PlayerController::OnTouchMoved);
	InputComponent->BindTouch(IE_Released, this, &ARok2PlayerController::OnTouchEnd);
}

void ARok2PlayerController::OnTouchBegin(ETouchIndex::Type FingerIndex, FVector Location)
{
	const FVector2D Pos(Location.X, Location.Y);

	if (FingerIndex == ETouchIndex::Touch1)
	{
		Touch0Pos = Pos;
		bTouch0Active = true;
		bTouchMovedTooFarForTap = false;
		bPinching = false;
		TouchStartSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	}
	else if (FingerIndex == ETouchIndex::Touch2)
	{
		Touch1Pos = Pos;
		bTouch1Active = true;
		bPinching = true;
		// قياس مرجعي أول، وإلا قفزت قيمة التكبير في الإطار الأول.
		LastPinchDistance = FVector2D::Distance(Touch0Pos, Touch1Pos);
		// إصبع ثانٍ يعني أن الإيماءة ليست نقرة.
		bTouchMovedTooFarForTap = true;
	}
}

void ARok2PlayerController::OnTouchMoved(ETouchIndex::Type FingerIndex, FVector Location)
{
	const FVector2D Pos(Location.X, Location.Y);

	if (FingerIndex == ETouchIndex::Touch1)
	{
		const FVector2D Delta = Pos - Touch0Pos;
		Touch0Pos = Pos;

		if (Delta.Size() > TapMoveThresholdPx)
		{
			bTouchMovedTooFarForTap = true;
		}

		// أثناء التكبير بإصبعين لا نحرّك الكاميرا — وإلا تصارعت الإيماءتان.
		if (!bPinching && IsoCamera)
		{
			IsoCamera->PanByScreenDelta(Delta);
		}
	}
	else if (FingerIndex == ETouchIndex::Touch2)
	{
		Touch1Pos = Pos;
	}

	if (bPinching && bTouch0Active && bTouch1Active && IsoCamera)
	{
		const float Distance = FVector2D::Distance(Touch0Pos, Touch1Pos);
		IsoCamera->ZoomByPinch(Distance - LastPinchDistance);
		LastPinchDistance = Distance;
	}
}

void ARok2PlayerController::OnTouchEnd(ETouchIndex::Type FingerIndex, FVector Location)
{
	if (FingerIndex == ETouchIndex::Touch2)
	{
		bTouch1Active = false;
		bPinching = false;
		return;
	}

	if (FingerIndex != ETouchIndex::Touch1)
	{
		return;
	}

	bTouch0Active = false;

	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	const bool bWasQuick = (Now - TouchStartSeconds) <= TapMaxDurationSeconds;

	if (!bTouchMovedTooFarForTap && bWasQuick && !bTouch1Active)
	{
		HandleTapAtScreenPos(FVector2D(Location.X, Location.Y));
	}

	bPinching = false;
}

void ARok2PlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsoCamera)
	{
		IsoCamera->AddPan(PanInput * DeltaSeconds);
		IsoCamera->AddZoom(ZoomInput * DeltaSeconds);
	}
}

void ARok2PlayerController::FocusOnPlayerCity()
{
	if (!IsoCamera || !Api) return;
	const FRok2Player& P = Api->GetPlayer();
	if (P.Id.IsEmpty()) return;
	// map world coords are in our own tile units; convert to UE cm.
	FVector Target(P.X * 100.f, P.Y * 100.f, 0.f);
	IsoCamera->FocusOn(Target);
}

void ARok2PlayerController::OnTap()
{
	// مسار الفأرة (حاسوب) — يحوّل إلى نفس منطق موضع الشاشة الذي يستخدمه اللمس.
	float MouseX = 0.f;
	float MouseY = 0.f;
	if (GetMousePosition(MouseX, MouseY))
	{
		HandleTapAtScreenPos(FVector2D(MouseX, MouseY));
	}
}

void ARok2PlayerController::HandleTapAtScreenPos(const FVector2D& ScreenPos)
{
	FHitResult HitResult;
	if (GetHitResultAtScreenPosition(ScreenPos, ECC_Visibility, true, HitResult))
	{
		if (!Api) return;

		FVector HitLoc = HitResult.ImpactPoint;
		float WorldX = HitLoc.X / 100.f;
		float WorldY = HitLoc.Y / 100.f;

		FString FoundType;
		FString FoundId;
		FString FoundName;
		float FoundX = 0;
		float FoundY = 0;
		float MinDistSq = 999999.f;

		const FRok2WorldSnapshot& W = Api->GetWorldSnapshot();

		for (const FRok2CityEntity& C : W.Cities)
		{
			if (C.PlayerId == Api->GetPlayer().Id) continue; 
			float DistSq = FVector2D::DistSquared(FVector2D(C.X, C.Y), FVector2D(WorldX, WorldY));
			if (DistSq < MinDistSq)
			{
				MinDistSq = DistSq;
				FoundType = TEXT("city");
				FoundId = C.PlayerId;
				FoundName = C.Name;
				FoundX = C.X;
				FoundY = C.Y;
			}
		}

		for (const FRok2PassEntity& P : W.Passes)
		{
			float DistSq = FVector2D::DistSquared(FVector2D(P.X, P.Y), FVector2D(WorldX, WorldY));
			if (DistSq < MinDistSq)
			{
				MinDistSq = DistSq;
				FoundType = TEXT("pass");
				FoundId = P.Id;
				FoundName = FString::Printf(TEXT("Pass Level %d"), P.Level);
				FoundX = P.X;
				FoundY = P.Y;
			}
		}

		for (const FRok2NodeEntity& N : W.Nodes)
		{
			float DistSq = FVector2D::DistSquared(FVector2D(N.X, N.Y), FVector2D(WorldX, WorldY));
			if (DistSq < MinDistSq)
			{
				MinDistSq = DistSq;
				FoundType = N.Kind == TEXT("barb") ? TEXT("barbarian") : TEXT("node");
				FoundId = N.Id;
				FoundName = FString::Printf(TEXT("%s Level %d"), *FoundType, N.Level);
				FoundX = N.X;
				FoundY = N.Y;
			}
		}

		if (MinDistSq < 100.f)
		{
			ARok2WorldRenderer* WorldRenderer = Cast<ARok2WorldRenderer>(UGameplayStatics::GetActorOfClass(GetWorld(), ARok2WorldRenderer::StaticClass()));
			if (!WorldRenderer || !WorldRenderer->CanInteractWithWorldTarget(FoundType, false))
			{
				return;
			}
			if (URok2MarchPanel* Panel = Cast<URok2MarchPanel>(URok2BlueprintLibrary::CreateRok2Widget(this, URok2MarchPanel::StaticClass())))
			{
				Panel->Api = Api;
				Panel->TargetType = FoundType;
				Panel->TargetId = FoundId;
				Panel->TargetName = FoundName;
				Panel->ToX = FoundX;
				Panel->ToY = FoundY;
				// P18-T5: كانت `AddToViewport()` بلا معامل — أي ZOrder = 0،
				// **تحت الـHUD (20)**. فأزرار الإرسال والكشافة كانت مرسومة تحت
				// شريط الموارد وعنقود الأزرار، ولوحة المسيرة خارج ترتيب الطبقات
				// الذي يقرأه زر الرجوع. ترتيب اللوحات هو 50 في هذا المشروع.
				Panel->AddToViewport(50);
			}
		}
	}
}

void ARok2PlayerController::OnEscape()
{
	HandleBackRequested();
}

void ARok2PlayerController::OnAndroidBack()
{
	HandleBackRequested();
}

// ---------------------------------------------------------------------------
// P18-T5: زر الرجوع.
//
// كان جسم `OnEscape` تعليقاً واحداً («could close UI panels») — فمفتاح Escape
// على الحاسوب وزر الرجوع على أندرويد لا يفعلان شيئاً مهما تراكمت اللوحات.
//
// السلوك: **طبقة واحدة لكل ضغطة**، بترتيب المنفذ الفعلي (بطاقة المبنى 200 ثم
// ورقة التدريب 150 ثم اللوحات 50 … فوق الـHUD 20). ثم — إن خلت الشاشة —
// وضع تحرير المدينة، ثم عرض الخريطة يعود للمدينة، وأخيراً تأكيد الخروج.
//
// الترتيب مقصود: الرجوع يُلغي آخر ما فعله اللاعب. من فتح الخريطة فلمس عقدة
// فلوحة مسيرة، يعود بثلاث ضغطات إلى مدينته لا يُقذف خارج التطبيق.
// ---------------------------------------------------------------------------
void ARok2PlayerController::HandleBackRequested()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// `AKEYCODE_BACK` يصل مرتين على أندرويد (Escape + Android_Back)؛ الضغطة
	// الواحدة يجب أن تغلق طبقة واحدة.
	const float Now = World->GetTimeSeconds();
	if (LastBackHandledSeconds >= 0.f && (Now - LastBackHandledSeconds) < BackDebounceSeconds)
	{
		return;
	}
	LastBackHandledSeconds = Now;

	// 1) أعلى لوحة مفتوحة — تُغلق بحركتها الخاصة لا بإزالة مفاجئة.
	if (URok2UiStack::DismissTopLayer(this))
	{
		return;
	}

	ARok2GameMode* GameMode = World->GetAuthGameMode<ARok2GameMode>();

	// 2) وضع تحرير المدينة حالة يجب أن يُخرج منها الرجوع: اللاعب فيه يسحب
	//    مبانٍ ولا زر «تم» بارز، فبدون هذا كان الرجوع يقفز إلى سؤال الخروج
	//    والمدينة ما تزال في وضع التحرير.
	if (GameMode && GameMode->ViewManager && GameMode->ViewManager->CityBuilder
		&& GameMode->ViewManager->CityBuilder->IsEditModeActive())
	{
		GameMode->ViewManager->CityBuilder->ToggleEditMode();
		return;
	}

	// 3) على الخريطة: الرجوع يعود إلى المدينة — «الرجوع» بمعناه المباشر.
	if (GameMode && GameMode->ViewManager && !GameMode->ViewManager->IsCityView()
		&& !GameMode->ViewManager->IsTransitioning())
	{
		GameMode->ViewManager->SwitchToCityView();
		return;
	}

	// 4) لا شيء ليُغلق: تأكيد الخروج. الودجة نفسها طبقة قابلة للتسريح، فضغطة
	//    رجوع أخرى تُلغي السؤال بدل أن تخرج.
	if (!ExitConfirmWidget)
	{
		ExitConfirmWidget = Cast<URok2ExitConfirmWidget>(
			URok2BlueprintLibrary::CreateRok2Widget(this, URok2ExitConfirmWidget::StaticClass()));
	}
	if (ExitConfirmWidget && !ExitConfirmWidget->IsInViewport())
	{
		// فوق كل شيء إلا شاشة الدخول (100): سؤالٌ يوقف اللعب لا لوحة محتوى.
		ExitConfirmWidget->AddToViewport(90);
	}
}
