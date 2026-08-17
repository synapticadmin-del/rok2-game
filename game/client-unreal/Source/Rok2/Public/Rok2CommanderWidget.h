// Copyright ROK2. Commander screen widget (P5-T4).
//
// شاشة القادة الكاملة بأسلوب RoK: قائمة بورتريهات + تفاصيل قائد مختار
// (بورتريه كبير، نجوم، خبرة، مهارات، مواهب، معدات).
// المرجع: 07-game-design/ui-ux-design-system.md §5 + rok-features-audit.md §2.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rok2DismissibleLayer.h"
#include "Rok2Types.h"
#include "Rok2CommanderWidget.generated.h"

class URok2Api;
class UTextBlock;
class UImage;
class UButton;
class UProgressBar;
class UScrollBox;
class UVerticalBox;
class UHorizontalBox;
class UCanvasPanel;
class URok2CommanderCardHandler;

/** مهارة قائد واحدة (attack/defense/passive) — من commanders.json. */
USTRUCT(BlueprintType)
struct FRok2CommanderSkillData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Rok2")
	FString Id;

	UPROPERTY(BlueprintReadOnly, Category = "Rok2")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Rok2")
	FString Type; // "attack" / "defense" / "passive"

	UPROPERTY(BlueprintReadOnly, Category = "Rok2")
	FString Description;

	UPROPERTY(BlueprintReadOnly, Category = "Rok2")
	int32 MaxLevel = 5;

	UPROPERTY(BlueprintReadOnly, Category = "Rok2")
	int32 CurrentLevel = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Rok2")
	FString StatName;

	UPROPERTY(BlueprintReadOnly, Category = "Rok2")
	float PerLevel = 0.f;
};

/** بيانات قائد كاملة (أساسية + مهارات) — من commanders.json. */
USTRUCT(BlueprintType)
struct FRok2CommanderDetailData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Rok2")
	FString Id;

	UPROPERTY(BlueprintReadOnly, Category = "Rok2")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Rok2")
	FString Rarity;

	UPROPERTY(BlueprintReadOnly, Category = "Rok2")
	FString Nation;

	UPROPERTY(BlueprintReadOnly, Category = "Rok2")
	TArray<FString> Tags;

	UPROPERTY(BlueprintReadOnly, Category = "Rok2")
	int32 Level = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Rok2")
	int32 Stars = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Rok2")
	int32 Xp = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Rok2")
	int32 XpToNext = 1000;

	UPROPERTY(BlueprintReadOnly, Category = "Rok2")
	int32 Attack = 50;

	UPROPERTY(BlueprintReadOnly, Category = "Rok2")
	int32 Defense = 50;

	UPROPERTY(BlueprintReadOnly, Category = "Rok2")
	int32 Utility = 50;

	UPROPERTY(BlueprintReadOnly, Category = "Rok2")
	TArray<FRok2CommanderSkillData> Skills;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCommanderSelected, const FString&, CommanderId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAssignCommander, const FString&, CommanderId);

UCLASS()
class ROK2_API URok2CommanderWidget : public UUserWidget, public IRok2DismissibleLayer
{
	GENERATED_BODY()

public:
	/** تهيئة الشاشة بمرجع الـ API (يُستدعى بعد الإنشاء). */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void SetupWithApi(URok2Api* InApi);

	/**
	 * P18-T5: إغلاق شاشة القادة.
	 *
	 * كانت هذه الشاشة تُضاف للمنفذ في `HandleCommandersAction` **ولا تُزال
	 * أبداً**: لا زر إغلاق في ترويستها ولا حجاب يُلمس ولا مسار في المشروع
	 * يزيلها. فمن يفتح القادة يبقى عليها إلى نهاية الجلسة، والمدينة والخريطة
	 * تحتها لا تُلمس. الرجوع وزر الإغلاق يمرّان من هنا.
	 */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void CloseSelf();

	virtual void DismissLayer() override { CloseSelf(); }

	/** يحدّث القائمة من Api->GetCommanders() + commanders.json. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void RefreshCommanderList();

	/** يختار قائداً ويملأ لوحة التفاصيل. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void SelectCommander(const FString& CommanderId);

	/** يُطلق عند اختيار قائد من القائمة. */
	UPROPERTY(BlueprintAssignable, Category = "Rok2")
	FOnCommanderSelected OnCommanderSelected;

	/** يُطلق عند الضغط على "تعيين في مسيرة". */
	UPROPERTY(BlueprintAssignable, Category = "Rok2")
	FOnAssignCommander OnAssignCommander;

protected:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	// ---- البناء الديناميكي للواجهة ----
	void BuildUI();

	/** يبني بطاقة قائد في القائمة (بورتريه + اسم + ندرة + نجوم). */
	UWidget* BuildCommanderCard(const FRok2Commander& Cmd);

	/** يملأ لوحة التفاصيل لقائد مختار. */
	void PopulateDetailPanel(const FRok2CommanderDetailData& Detail);

	/** يبني صف مهارة في لوحة التفاصيل. */
	UWidget* BuildSkillRow(const FRok2CommanderSkillData& Skill, int32 SlotIndex);

	/** يبني شجرة مواهب stub (3 فروع ملونة). */
	UWidget* BuildTalentTreeStub();

	/** يبني خانات المعدات stub (4 خانات + إكسسوار). */
	UWidget* BuildEquipmentSlots();

	/** لون الندرة (أخضر/أزرق/بنفسجي/برتقالي). */
	static FLinearColor RarityColor(const FString& Rarity);

	/** يحوّل نص الندرة إلى عدد نجوم افتراضي. */
	static int32 StarsForRarity(const FString& Rarity);

	/** يقرأ بيانات قائد مفصلة من commanders.json (من القرص أو fallback). */
	FRok2CommanderDetailData LoadCommanderDetail(const FString& CommanderId);

	/** يبني placeholder portrait (مربع ملوّن بحرف الاسم الأول). */
	UWidget* BuildPortraitPlaceholder(const FString& CommanderName, const FString& Nation, float Size);

	UFUNCTION()
	void OnAssignClicked();


	UFUNCTION()
	void OnLevelUpClicked();

	UFUNCTION()
	void OnSkillUpgradeClicked();

	/** زر إغلاق الترويسة (P18-T5) — يمرّ بـ`CloseSelf`. */
	UFUNCTION()
	void OnCloseClicked();

	UPROPERTY(Transient)
	URok2Api* Api;

	/** معرف القائد المختار حالياً. */
	FString SelectedCommanderId;

	// ---- مراجع الواجهة ----
	UPROPERTY(Transient)
	UScrollBox* CommanderListScroll;

	UPROPERTY(Transient)
	UVerticalBox* CommanderListBox;

	UPROPERTY(Transient)
	UVerticalBox* DetailPanel;

	UPROPERTY(Transient)
	UTextBlock* DetailNameText;

	UPROPERTY(Transient)
	UTextBlock* DetailRarityText;

	UPROPERTY(Transient)
	UTextBlock* DetailNationText;

	UPROPERTY(Transient)
	UTextBlock* DetailLevelText;

	UPROPERTY(Transient)
	UTextBlock* DetailStarsText;

	UPROPERTY(Transient)
	UProgressBar* DetailXpBar;

	// P7-T7: نص نسبة الخبرة الصريح أسفل الشريط (لا اعتماد على الشريط وحده)
	UPROPERTY(Transient)
	UTextBlock* DetailXpText;

	UPROPERTY(Transient)
	UTextBlock* DetailStatsText;

	UPROPERTY(Transient)
	UVerticalBox* SkillsBox;

	UPROPERTY(Transient)
	UVerticalBox* TalentsBox;

	UPROPERTY(Transient)
	UHorizontalBox* EquipmentBox;

	UPROPERTY(Transient)
	UImage* DetailPortraitImage;

	/**
	 * P24-T5: لوحة الجلد الأسطوري خلف ترويسة التفاصيل. الصور الست في
	 * Content/Art/CommanderSkins (35MB) كانت مولّدة بلا أي قارئ في المشروع.
	 * تظهر للقادة الأسطوريين وحدهم وتُطوى لغيرهم.
	 */
	UPROPERTY(Transient)
	UImage* DetailSkinImage;

	/** معالج مستقل لكل بطاقة ديناميكية؛ يحتفظ بمعرّف القائد حتى يصل حدث الضغط. */
	UPROPERTY(Transient)
	TArray<URok2CommanderCardHandler*> CommanderCardHandlers;

	/** خريطة معرف القائد → بياناته المفصلة (تُملأ من commanders.json). */
	TMap<FString, FRok2CommanderDetailData> CommanderDetails;

	/** هل تم تحميل commanders.json؟ */
	bool bDetailsLoaded = false;

	void LoadCommanderDetailsFromJson();
};

/** يربط زر بطاقة ديناميكية بالقائد الذي أنشأه؛ مطابق لنمط معالجات طوابير المدينة. */
UCLASS()
class ROK2_API URok2CommanderCardHandler : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString CommanderId;

	UPROPERTY()
	URok2CommanderWidget* Widget;

	UFUNCTION()
	void OnClick();
};
