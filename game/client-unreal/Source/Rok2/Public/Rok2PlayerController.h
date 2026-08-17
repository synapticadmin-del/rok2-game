// Copyright ROK2. Player controller driving isometric camera + input.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "GameFramework/PlayerController.h"
#include "Rok2PlayerController.generated.h"

class URok2Api;

UCLASS()
class ARok2PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ARok2PlayerController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	URok2Api* GetApi() const { return Api; }

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void FocusOnPlayerCity();

	/**
	 * P18-T5: زر الرجوع. يُغلق **طبقة واحدة** لكل ضغطة بترتيب المنفذ (اللوحات
	 * قبل الـHUD)، وعند خلو الشاشة يطلب تأكيد الخروج.
	 *
	 * عام وBlueprintCallable لأنه يُربط من ثلاثة مصادر: مفتاح Escape على
	 * الحاسوب، `EKeys::Android_Back` على الجهاز، وأي زر رجوع في الواجهة.
	 */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void HandleBackRequested();

protected:
	UPROPERTY(Transient)
	URok2Api* Api;

	/**
	 * لوحة تأكيد الخروج (P18-T5) — تُنشأ عند أول ضغطة رجوع على شاشة خالية
	 * وتُعاد للعرض بعدها. الاحتفاظ بالمؤشر يمنع تراكم نسخة لكل ضغطة.
	 */
	UPROPERTY(Transient)
	class URok2ExitConfirmWidget* ExitConfirmWidget;

	UPROPERTY(Transient)
	class ARok2IsometricCamera* IsoCamera;

	UPROPERTY(EditDefaultsOnly, Category = "Rok2")
	TSubclassOf<class ARok2IsometricCamera> CameraClass;

	FVector2D PanInput = FVector2D::ZeroVector;
	float ZoomInput = 0.f;

	void OnPanX(float V) { PanInput.X = V; }
	void OnPanY(float V) { PanInput.Y = V; }
	void OnZoom(float V) { ZoomInput = V; }
	void OnTap();
	void OnEscape();

	/**
	 * P18-T5: زر الرجوع على أندرويد.
	 *
	 * لماذا ربط مباشر لا `ActionMappings`: `FAndroidPlatformInput::GetKeyMap`
	 * يسجّل `AKEYCODE_BACK` مرتين — مرة كـ`Escape` (خريطة المحارف) ومرة
	 * كـ`Android_Back`. و`FInputKeyManager::GetKeyFromCodes` يقدّم الخريطة
	 * الافتراضية، فأي من الاسمين قد يصل حسب مسار الحدث. الربط بالمفتاحين معاً
	 * يجعل السلوك واحداً بلا اعتماد على أي المسارين فاز، ويمنع ضغطة مزدوجة عبر
	 * حارس الإطار الواحد في `HandleBackRequested`.
	 */
	void OnAndroidBack();

	// -----------------------------------------------------------------------
	// إدخال اللمس (أندرويد)
	//
	// المشروع كان يعتمد على BindAxis/BindAction فقط — وهي بلا معنى على
	// الهاتف: لا عجلة فأرة ولا مؤشر. بدونها الخريطة غير قابلة للتحريك أو
	// التكبير أو الاختيار على الجهاز.
	//   إصبع واحد يسحب  -> تحريك الكاميرا
	//   إصبعان يتباعدان -> تكبير/تصغير
	//   نقرة قصيرة      -> اختيار هدف
	// -----------------------------------------------------------------------
	void OnTouchBegin(ETouchIndex::Type FingerIndex, FVector Location);
	void OnTouchMoved(ETouchIndex::Type FingerIndex, FVector Location);
	void OnTouchEnd(ETouchIndex::Type FingerIndex, FVector Location);

	/** منطق الاختيار مشترك بين الفأرة واللمس. */
	void HandleTapAtScreenPos(const FVector2D& ScreenPos);

	FVector2D Touch0Pos = FVector2D::ZeroVector;
	FVector2D Touch1Pos = FVector2D::ZeroVector;
	bool bTouch0Active = false;
	bool bTouch1Active = false;
	bool bPinching = false;
	bool bTouchMovedTooFarForTap = false;
	float LastPinchDistance = 0.f;
	float TouchStartSeconds = 0.f;

	/** أقصى إزاحة بالبكسل تُحتسب معها اللمسة نقرة لا سحباً. */
	static constexpr float TapMoveThresholdPx = 24.f;

	/** أقصى مدة بالثواني تُحتسب معها اللمسة نقرة. */
	static constexpr float TapMaxDurationSeconds = 0.6f;

	/**
	 * زمن آخر ضغطة رجوع مُعالَجة. `AKEYCODE_BACK` يصل مرتين على أندرويد
	 * (كـ`Escape` وكـ`Android_Back`) فبدون هذا الحارس كانت الضغطة الواحدة تغلق
	 * طبقتين، أو تفتح تأكيد الخروج وتُلغيه في اللحظة نفسها.
	 */
	float LastBackHandledSeconds = -1.f;

	/** نافذة تجاهل الضغطة المكرّرة — أقصر من أي ضغطة بشرية متتابعة. */
	static constexpr float BackDebounceSeconds = 0.15f;
};
