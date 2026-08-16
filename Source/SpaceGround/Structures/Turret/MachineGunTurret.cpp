#include "MachineGunTurret.h"

#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"


AMachineGunTurret::AMachineGunTurret()
{
	/*
	 * Tick 설정은 TurretBase가 담당한다.
	 * 부모 생성자에서 기본 Tick은 비활성화되며,
	 * 작동 가능 + 유효한 타깃 + 탄약 보유 시에만 켜진다.
	 */
}


void AMachineGunTurret::FireAtTarget(
	AActor* Target
)
{
	/*
	 * 정상 경로에서는 TurretBase::TryFire()가 이미 검사하지만,
	 * 전력 상태가 발사 직전에 바뀌는 경우도 안전하게 차단한다.
	 */
	if (!CanOperate() || !IsValid(Target))
	{
		return;
	}

	USceneComponent* Muzzle = GetMuzzlePoint();
	UWorld* World = GetWorld();

	if (!IsValid(Muzzle) || !World)
	{
		return;
	}


	const FVector TraceStart =
		Muzzle->GetComponentLocation();

	const FVector TraceDirection =
		Muzzle
			->GetForwardVector()
			.GetSafeNormal();

	if (TraceDirection.IsNearlyZero())
	{
		return;
	}

	const FVector TraceEnd =
		TraceStart +
		TraceDirection *
		GetAttackRange();


	FHitResult HitResult;

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(MachineGunTurretFire),
		false,
		this
	);

	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = false;


	const bool bHit =
		World->LineTraceSingleByChannel(
			HitResult,
			TraceStart,
			TraceEnd,
			ECC_Visibility,
			QueryParams
		);


	if (bHit && IsValid(HitResult.GetActor()))
	{
		UGameplayStatics::ApplyPointDamage(
			HitResult.GetActor(),
			GetTurretDamage(),
			TraceDirection,
			HitResult,
			nullptr,
			this,
			nullptr
		);
	}


	if (bDrawDebugFire)
	{
		DrawDebugLine(
			World,
			TraceStart,
			bHit
				? HitResult.ImpactPoint
				: TraceEnd,
			bHit
				? FColor::Red
				: FColor::Green,
			false,
			DebugLineDuration,
			0,
			2.0f
		);

		if (bHit)
		{
			DrawDebugPoint(
				World,
				HitResult.ImpactPoint,
				10.0f,
				FColor::Yellow,
				false,
				DebugLineDuration
			);
		}
	}


	BP_OnMachineGunFired(
		TraceStart,
		TraceEnd,
		bHit,
		HitResult
	);


	UE_LOG(
		LogTemp,
		Verbose,
		TEXT(
			"[MachineGunTurret] 발사. "
			"Target=%s Hit=%s HitActor=%s Damage=%.1f"
		),
		*GetNameSafe(Target),
		bHit
			? TEXT("True")
			: TEXT("False"),
		*GetNameSafe(HitResult.GetActor()),
		GetTurretDamage()
	);
}
