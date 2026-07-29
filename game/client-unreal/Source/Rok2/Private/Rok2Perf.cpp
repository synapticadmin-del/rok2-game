// Copyright ROK2. Performance subsystem (P4-T7) — implementation.

#include "Rok2Perf.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/GameInstance.h"
#include "UObject/ConstructorHelpers.h"

void URok2Perf::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// سخّن الخبأ بالأشكال الأكثر استخداماً — التحميل يحدث مرة واحدة هنا بدل
	// LoadObject متفرق في كل building/refresh عبر الجلسة.
	static const TCHAR* WarmShapes[] = { TEXT("Plane"), TEXT("Cube"), TEXT("Sphere"), TEXT("Cylinder"), TEXT("Cone") };
	for (const TCHAR* S : WarmShapes)
	{
		GetEngineMesh(FString(S));
	}
}

void URok2Perf::Deinitialize()
{
	FlushPool();
	EngineMeshCache.Empty();
	Super::Deinitialize();
}

URok2Perf* URok2Perf::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject) return nullptr;
	if (UWorld* World = WorldContextObject->GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			return GI->GetSubsystem<URok2Perf>();
		}
	}
	return nullptr;
}

UStaticMesh* URok2Perf::GetEngineMesh(const FString& ShapeName)
{
	if (UStaticMesh** Found = EngineMeshCache.Find(ShapeName))
	{
		return *Found;
	}
	const FString Path = FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), *ShapeName, *ShapeName);
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Path);
	if (Mesh)
	{
		EngineMeshCache.Add(ShapeName, Mesh);
	}
	return Mesh;
}

AStaticMeshActor* URok2Perf::AcquireMarkerActor(UWorld* World)
{
	if (!World) return nullptr;

	while (Pool.Num() > 0)
	{
		AStaticMeshActor* A = Pool.Pop();
		if (IsValid(A))
		{
			A->SetActorHiddenInGame(false);
			A->SetActorTickEnabled(true);
			return A;
		}
	}

	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<AStaticMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator, P);
}

void URok2Perf::ReleaseMarkerActor(AStaticMeshActor* Actor)
{
	if (!IsValid(Actor)) return;

	if (Pool.Num() >= MaxPoolSize)
	{
		Actor->Destroy();
		return;
	}

	Actor->SetActorHiddenInGame(true);
	Actor->SetActorTickEnabled(false);
	Actor->SetActorLocation(FVector::ZeroVector);
	Actor->SetActorScale3D(FVector::OneVector);
	Actor->SetActorRotation(FRotator::ZeroRotator);
	Pool.Add(Actor);
}

void URok2Perf::FlushPool()
{
	for (AStaticMeshActor* A : Pool)
	{
		if (IsValid(A)) A->Destroy();
	}
	Pool.Empty();
}
