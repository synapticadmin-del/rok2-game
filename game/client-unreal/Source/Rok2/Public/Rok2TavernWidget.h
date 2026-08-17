// Copyright ROK2. شاشة الحانة والصناديق (P19-T4) — header.
//
// قبل هذا البند: 24 أصلاً بصرياً في `Content/Art/Tavern` (ثلاثة صناديق، ستة
// مفاتيح، أربع منحوتات، أربع مواد، ستة مخططات) **بلا أي مستهلك** —
// `URok2ArtAssets::LoadTavernIcon` معرّفة ولا مستدعٍ لها. وصوتان
// (`chest_open.wav`, `wheel_spin.wav`) بلا نوع في `ERok2AudioType` أصلاً.
// و`HandleBuildingAction` يبثّ توستاً صادقاً عند "chests" انتظاراً لهذه الشاشة.
//
// كل الأرقام من الخادم: أوزان الرميات وسقف الفتح في الساعة والمفتاح اليومي
// المجاني من `data/tavern.json`، والرصيد من `tavern_state` في الشارد. لا
// احتمال ولا سعر في العميل.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rok2DismissibleLayer.h"
#include "Rok2Types.h"
#include "Rok2TavernWidget.generated.h"

class URok2Api;
class UButton;
class UImage;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;
class URok2TavernWidget;

/** وسيط زر صندوق — يحمل معرّفه حتى تصل الضغطة (UFUNCTION بلا معاملات). */
UCLASS()
class URok2TavernBoxProxy : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY() FString BoxId;

	UPROPERTY(Transient)
	URok2TavernWidget* Owner = nullptr;

	UFUNCTION()
	void HandleClick();
};

UCLASS(BlueprintType, Blueprintable)
class ROK2_API URok2TavernWidget : public UUserWidget, public IRok2DismissibleLayer
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Rok2|Tavern")
	void Setup(URok2Api* InApi);

	/** يُنادى من وسيط الزر: فتح صندوق. */
	void RequestOpenBox(const FString& BoxId);

	virtual void DismissLayer() override { OnCloseClicked(); }

protected:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UPROPERTY(Transient)
	URok2Api* Api;

	UPROPERTY(Transient) UVerticalBox* BoxesList;
	UPROPERTY(Transient) UVerticalBox* ResultsList;
	UPROPERTY(Transient) UTextBlock* HourlyText;
	UPROPERTY(Transient) UTextBlock* DailyKeyText;
	UPROPERTY(Transient) UButton* DailyKeyButton;

	UPROPERTY(Transient)
	TArray<URok2TavernBoxProxy*> Proxies;

	/**
	 * عدد الرميات المعروضة عند آخر إعادة بناء.
	 *
	 * صوت فتح الصندوق يجب أن يُشغَّل عند **وصول رميات جديدة** لا عند كل تحديث
	 * حالة: `OnTavernUpdated` يُبثّ من `FetchTavernState` كذلك، فبلا هذه
	 * المقارنة يُسمع الصوت كلما فُتحت الشاشة.
	 */
	int32 LastRollCount = 0;

	UFUNCTION() void OnCloseClicked();
	UFUNCTION() void OnDailyKeyClicked();

	/** تُنادى عند وصول حالة الحانة من الخادم. */
	UFUNCTION() void OnTavernUpdated(const FRok2TavernState& State);

	void RebuildBoxes();
	void RebuildResults();
	void UpdateStatus();

	/**
	 * بطاقة صندوق: صورته المستوردة + رصيد مفتاحه + زر فتح معطّل عند غيابه.
	 * `LoadTavernIcon` هي الدالة التي لم يكن لها مستدعٍ.
	 */
	void BuildBoxCard(const FString& BoxId, const FString& BoxName, const FString& KeyId, int32 KeysHeld);

	/** اسم الرمية بالعربية — من فئتها لا من معرّفها اللاتيني. */
	FString RollKindName(const FString& Kind) const;

	/** أيقونة الرمية من حزمة الحانة (منحوتة/مواد/مخطط). */
	FString RollKindIcon(const FString& Kind) const;
};
