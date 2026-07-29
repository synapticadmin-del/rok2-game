// Copyright ROK2. Unified procedural UI icon library (P6-T1).
//
// مكتبة رموز UI الموحدة — رسم إجرائي كامل (بدون أصول خارجية وبدون إيموجي).
// كل أيقونة تُرسم على UTexture2D (32×32 افتراضياً) بأسلوب «مخطوطة ملكية»
// من ui-ux-design-system.md §1: خطوط واضحة بسماكة 2px، لون عاجي #F5E9D0
// على خلفية شفافة، مع لمعة ذهبية #C9A227 عند الطلب.
//
// الاستخدام:
//   FSlateBrush B = URok2IconLibrary::GetBrush(TEXT("build"), 32.f);
//   Image->SetBrush(B);
// أو للنصوص المركبة (أيقونة + رقم):
//   UWidget* W = URok2IconLibrary::MakeIconLabel(Tree, TEXT("food"), TEXT("1.2K"), Color);
//
// الأيقونات المدعومة (Id): انظر IconIdList في الـ cpp — ~40 رمزاً يغطي
// الموارد والأزرار والحالات والمباني والقتال والمواسم.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Styling/SlateBrush.h"
#include "Rok2IconLibrary.generated.h"

class UTexture2D;
class UWidget;
class UWidgetTree;

/** أحجام الأيقونات المعيارية من وثيقة UI (24px للنصوص، 32px للأزرار) */
enum class ERok2IconSize : uint8
{
	Small  = 24,	// داخل النصوص والقوائم
	Medium = 32,	// الأزرار القياسية
	Large  = 48		// الأزرار الدائرية الكبيرة (زر البناء)
};

UCLASS(BlueprintType)
class ROK2_API URok2IconLibrary : public UObject
{
	GENERATED_BODY()

public:
	/** نسخة مشتركة (AddToRoot) — تبني الخبأ عند أول طلب */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Icons")
	static URok2IconLibrary* Get();

	/** هل يوجد رسم لهذا المعرّف؟ */
	UFUNCTION(BlueprintPure, Category = "Rok2|Icons")
	bool HasIcon(const FString& IconId) const;

	/** قائمة كل المعرّفات المدعومة (للاختبار والتوثيق) */
	UFUNCTION(BlueprintPure, Category = "Rok2|Icons")
	TArray<FString> GetIconIds() const;

	/**
	 * يعيد FSlateBrush لأيقونة بحجم معياري (Small 24 / Medium 32 / Large 48).
	 * Tint اختياري: الأيقونة تُرسم بالعاجي افتراضياً ويُصبغ عبر الـ brush.
	 * عند معرّف غير معروف يعيد brush فارغة مشروبة باللون المطلوب (لا يُكسر UI).
	 */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Icons")
	FSlateBrush GetBrush(const FString& IconId, float Size = 32.f, FLinearColor Tint = FLinearColor(0.96f, 0.91f, 0.81f, 1.f));

	/**
	 * يبني UImage بأيقونة بحجم محدد — للأزرار والبطاقات.
	 * يعيد nullptr إن كان Tree غير صالح (لا يُكسر بناء الواجهة).
	 */
	static UWidget* MakeIconImage(UWidgetTree* Tree, const FString& IconId, float Size, FLinearColor Tint);

	/**
	 * يبني صفاً أفقياً: أيقونة (Small) + نص — بديل «الإيموجي + نص» في الشريط العلوي
	 * والقوائم. الأيقونة تُصبغ بلون النص نفسه ليظل الرمز والقيمة عائلة بصرية واحدة.
	 */
	static UWidget* MakeIconLabel(UWidgetTree* Tree, const FString& IconId, const FText& Label, FLinearColor Color, int32 FontSize);

	/** تمرير مركزي من URok2ArtAssets (P6-T1: تُخدم الأيقونات من ArtAssets) */
	static FSlateBrush BrushFromArtAssets(const FString& IconId, float Size, FLinearColor Tint);

protected:
	/** خبأ الأيقونات المرسومة: مفتاح = id@size */
	UPROPERTY(Transient)
	TMap<FString, UTexture2D*> Cache;

	bool bCatalogBuilt = false;
	void BuildCatalog();

	/** مجموعة المعرّفات المعروفة — تُملأ في BuildCatalog */
	UPROPERTY(Transient)
	TArray<FString> KnownIds;

	/** يرسم أيقونة على texture جديدة ويخبئها */
	UTexture2D* RenderIcon(const FString& IconId, int32 Size);
};
