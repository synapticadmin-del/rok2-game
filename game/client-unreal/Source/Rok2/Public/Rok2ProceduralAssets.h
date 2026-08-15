// Copyright ROK2. Procedural materials generated at runtime (no external textures needed for prototype).

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
UENUM(BlueprintType)
enum class ERok2MaterialType : uint8
{
	GroundTile,
	Building,
	Pass,
	Node,
	City,
	Mountain,
	Water
};

#include "Rok2ProceduralAssets.generated.h"


class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMeshComponent;
UCLASS()
class ROK2_API URok2ProceduralAssets : public UObject
{
	GENERATED_BODY()

public:
	/** Creates or returns a shared instance of procedural materials. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	static URok2ProceduralAssets* Get();

	/** مسار مادة المشروع المضاءة — تملك البارامترات Color/Roughness/Metallic/EmissiveColor. */
	static const TCHAR* BaseMaterialPath();

	/** مسار مادة Unlit — اللون على Emissive، يظهر بلا إضاءة. */
	static const TCHAR* UnlitMaterialPath();

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* GroundTileMat;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* BuildingMat;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* PassMat;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* NodeMat;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* CityMat;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* MountainMat;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* WaterMat;

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void Init();

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	UMaterialInstanceDynamic* GetMaterial(ERok2MaterialType Type);

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	UMaterialInstanceDynamic* CreateTintedMaterial(const FLinearColor& Color);

	/** يضع على القناة المطلوبة نسخة ديناميكية من مادة المشروع ثم يعيدها.
	 *
	 *  الفرق الجوهري عن CreateAndSetMaterialInstanceDynamic: تلك الدالة تبني
	 *  النسخة فوق المادة الموجودة على الميش — ومعظم أشكالنا مستوردة من
	 *  /Engine/BasicShapes فتحمل WorldGridMaterial أو DefaultMaterial، وكلتاهما
	 *  بلا أي VectorParameter. أي SetVectorParameterValue عليهما يُهمل بصمت
	 *  فيبقى الجسم بلا لون. هنا نفرض والداً نملك بارامتراته فعلاً.
	 */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	UMaterialInstanceDynamic* MakeTintedMaterialOn(UMeshComponent* Component, int32 ElementIndex, const FLinearColor& Color);

	/** يصبغ داخل مادة الأصل الحالية بدل استبدالها — للمجسمات المستوردة التي
	 *  تحمل نسيجاً حقيقياً. يعيد nullptr إن لم تملك المادة بارامتر لون، فتبقى
	 *  كما هي بدل أن تُمحى تفاصيلها بلون مسطّح. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	UMaterialInstanceDynamic* TintExistingMaterialOn(UMeshComponent* Component, int32 ElementIndex, const FLinearColor& Color);

	/** يحوّل لون الحضارة إلى مُضاعِف آمن لأصل ذي نسيج: إيماءة لونية مع حدّ أدنى
	 *  للسطوع، فلا يطفئ لونٌ داكن (مثل أساس اليابان #111111) المجسم كله. */
	UFUNCTION(BlueprintPure, Category = "Rok2")
	static FLinearColor SoftenTintForTexturedAsset(const FLinearColor& Color);

protected:
	UPROPERTY(Transient)
	UMaterialInterface* BaseMaterial;

	UPROPERTY(Transient)
	UMaterialInterface* UnlitMaterial;

	bool bInitialized = false;

	void EnsureInit();

	UMaterialInstanceDynamic* CreateDynInternal(const FLinearColor& Color);
};
