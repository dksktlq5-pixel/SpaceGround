#include "MachineGunTurret.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"


AMachineGunTurret::AMachineGunTurret()
{
	PrimaryActorTick.bCanEverTick = true;
}


void AMachineGunTurret::FireAtTarget(
	AActor* Target
)
{
	if (!IsValid(Target))
	{
		return;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}


	const FVector TraceStart =
		GetMuzzlePoint()->GetComponentLocation();

	const FVector TraceDirection =
		GetMuzzlePoint()
			->GetForwardVector()
			.GetSafeNormal();

	const FVector TraceEnd =
		TraceStart +
		TraceDirection *
		GetAttackRange();


	FHitResult HitResult;

	FCollisionQueryParams QueryParams;
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