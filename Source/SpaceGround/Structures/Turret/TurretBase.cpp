#include "TurretBase.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"


ATurretBase::ATurretBase()
{
	PrimaryActorTick.bCanEverTick = true;


	YawPivot =
		CreateDefaultSubobject<USceneComponent>(
			TEXT("YawPivot")
		);

	YawPivot->SetupAttachment(StructureMesh);


	PitchPivot =
		CreateDefaultSubobject<USceneComponent>(
			TEXT("PitchPivot")
		);

	PitchPivot->SetupAttachment(YawPivot);


	MuzzlePoint =
		CreateDefaultSubobject<USceneComponent>(
			TEXT("MuzzlePoint")
		);

	MuzzlePoint->SetupAttachment(PitchPivot);
}


void ATurretBase::BeginPlay()
{
	Super::BeginPlay();

	if (!LoadTurretData())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[TurretBase] 터렛 데이터 로드 실패. "
				"Actor=%s"
			),
			*GetNameSafe(this)
		);

		return;
	}

	if (CanOperate())
	{
		StartFireTimer();
	}
}


void ATurretBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!CanOperate())
	{
		return;
	}

	if (!HasValidTarget())
	{
		return;
	}

	UpdateTurretRotation(DeltaSeconds);
}


bool ATurretBase::LoadTurretData()
{
	const FSGStructureDefinition* Definition =
		FindStructureDefinition();

	if (!Definition)
	{
		return false;
	}


	TurretDamage =
		FMath::Max(
			0.0f,
			Definition->TurretData.Damage
		);

	BaseFireInterval =
		FMath::Max(
			0.01f,
			Definition->TurretData.FireInterval
		);

	AttackRange =
		FMath::Max(
			0.0f,
			Definition->TurretData.AttackRange
		);

	ViewAngle =
		FMath::Clamp(
			Definition->TurretData.ViewAngle,
			0.0f,
			360.0f
		);

	MaxAmmo =
		FMath::Max(
			0,
			Definition->TurretData.MaxAmmo
		);

	CurrentAmmo = MaxAmmo;

	ElementType =
		Definition->TurretData.ElementType;


	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"[TurretBase] 데이터 로드 완료. "
			"ID=%s Damage=%.1f Interval=%.2f "
			"Range=%.1f ViewAngle=%.1f Ammo=%d"
		),
		*GetStructureID().ToString(),
		TurretDamage,
		BaseFireInterval,
		AttackRange,
		ViewAngle,
		MaxAmmo
	);

	return true;
}


void ATurretBase::SetTargetActor(
	AActor* NewTarget
)
{
	if (NewTarget == this)
	{
		return;
	}

	if (TargetActor == NewTarget)
	{
		return;
	}

	TargetActor = NewTarget;

	BP_OnTargetChanged(TargetActor);


	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"[TurretBase] 타깃 변경. "
			"Turret=%s Target=%s"
		),
		*GetNameSafe(this),
		*GetNameSafe(TargetActor)
	);
}


void ATurretBase::ClearTargetActor()
{
	SetTargetActor(nullptr);
}


bool ATurretBase::HasValidTarget() const
{
	return
		IsValid(TargetActor) &&
		TargetActor != this &&
		!TargetActor->IsActorBeingDestroyed();
}


FVector ATurretBase::GetTargetLocation() const
{
	if (!HasValidTarget())
	{
		return FVector::ZeroVector;
	}

	return TargetActor->GetActorLocation();
}


bool ATurretBase::IsTargetInRange() const
{
	if (!HasValidTarget())
	{
		return false;
	}

	const float DistanceSquared =
		FVector::DistSquared(
			MuzzlePoint->GetComponentLocation(),
			GetTargetLocation()
		);

	return
		DistanceSquared <=
		FMath::Square(AttackRange);
}


bool ATurretBase::IsTargetInsideViewAngle() const
{
	if (!HasValidTarget())
	{
		return false;
	}

	if (ViewAngle >= 360.0f)
	{
		return true;
	}


	const FVector DirectionToTarget =
		(
			GetTargetLocation() -
			GetActorLocation()
		).GetSafeNormal2D();

	const FVector TurretForward =
		GetActorForwardVector().GetSafeNormal2D();

	if (
		DirectionToTarget.IsNearlyZero() ||
		TurretForward.IsNearlyZero()
	)
	{
		return false;
	}


	const float DotValue =
		FVector::DotProduct(
			TurretForward,
			DirectionToTarget
		);

	const float HalfAngle =
		FMath::DegreesToRadians(
			ViewAngle * 0.5f
		);

	const float MinimumDot =
		FMath::Cos(HalfAngle);

	return DotValue >= MinimumDot;
}


bool ATurretBase::HasLineOfSightToTarget() const
{
	if (!HasValidTarget())
	{
		return false;
	}

	const UWorld* World = GetWorld();

	if (!World)
	{
		return false;
	}


	FHitResult HitResult;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = false;


	const bool bHit =
		World->LineTraceSingleByChannel(
			HitResult,
			MuzzlePoint->GetComponentLocation(),
			GetTargetLocation(),
			ECC_Visibility,
			QueryParams
		);


	if (!bHit)
	{
		return true;
	}

	return HitResult.GetActor() == TargetActor;
}


bool ATurretBase::CanFireAtTarget() const
{
	if (!CanOperate())
	{
		return false;
	}

	if (!HasValidTarget())
	{
		return false;
	}

	if (CurrentAmmo <= 0)
	{
		return false;
	}

	if (!IsTargetInRange())
	{
		return false;
	}

	if (!IsTargetInsideViewAngle())
	{
		return false;
	}

	if (!HasLineOfSightToTarget())
	{
		return false;
	}


	const FVector DesiredDirection =
		(
			GetTargetLocation() -
			MuzzlePoint->GetComponentLocation()
		).GetSafeNormal();

	const float AimDot =
		FVector::DotProduct(
			MuzzlePoint->GetForwardVector(),
			DesiredDirection
		);

	const float MinimumAimDot =
		FMath::Cos(
			FMath::DegreesToRadians(
				FireAlignmentTolerance
			)
		);

	return AimDot >= MinimumAimDot;
}


void ATurretBase::UpdateTurretRotation(
	float DeltaSeconds
)
{
	if (!HasValidTarget())
	{
		return;
	}


	const FVector DirectionToTarget =
		GetTargetLocation() -
		MuzzlePoint->GetComponentLocation();

	if (DirectionToTarget.IsNearlyZero())
	{
		return;
	}


	const FRotator DesiredWorldRotation =
		DirectionToTarget.Rotation();


	// ─────────────────────────────────────────────
	// Yaw

	const FRotator CurrentYawWorldRotation =
		YawPivot->GetComponentRotation();

	const float NewYaw =
		FMath::FixedTurn(
			CurrentYawWorldRotation.Yaw,
			DesiredWorldRotation.Yaw,
			YawRotationSpeed * DeltaSeconds
		);

	YawPivot->SetWorldRotation(
		FRotator(
			0.0f,
			NewYaw,
			0.0f
		)
	);


	// ─────────────────────────────────────────────
	// Pitch

	const float DesiredPitch =
		FMath::Clamp(
			DesiredWorldRotation.Pitch,
			MinimumPitch,
			MaximumPitch
		);

	const FRotator CurrentPitchRotation =
		PitchPivot->GetRelativeRotation();

	const float NewPitch =
		FMath::FixedTurn(
			CurrentPitchRotation.Pitch,
			DesiredPitch,
			PitchRotationSpeed * DeltaSeconds
		);

	PitchPivot->SetRelativeRotation(
		FRotator(
			NewPitch,
			0.0f,
			0.0f
		)
	);
}


float ATurretBase::GetCurrentFireInterval() const
{
	if (GetHealthPercent() <= 0.4f)
	{
		return
			BaseFireInterval *
			FMath::Max(
				1.0f,
				DamagedFireIntervalMultiplier
			);
	}

	return BaseFireInterval;
}


void ATurretBase::StartFireTimer()
{
	if (!CanOperate())
	{
		return;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}


	const float NewInterval =
		GetCurrentFireInterval();

	if (
		World->GetTimerManager()
			.IsTimerActive(FireTimerHandle) &&
		FMath::IsNearlyEqual(
			ActiveTimerInterval,
			NewInterval
		)
	)
	{
		return;
	}


	StopFireTimer();

	ActiveTimerInterval = NewInterval;


	World->GetTimerManager().SetTimer(
		FireTimerHandle,
		this,
		&ATurretBase::TryFire,
		ActiveTimerInterval,
		true,
		ActiveTimerInterval
	);
}


void ATurretBase::StopFireTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager()
			.ClearTimer(FireTimerHandle);
	}

	ActiveTimerInterval = 0.0f;
}


void ATurretBase::RefreshFireTimer()
{
	if (!CanOperate())
	{
		StopFireTimer();
		return;
	}

	StartFireTimer();
}


void ATurretBase::TryFire()
{
	if (!CanOperate())
	{
		StopFireTimer();
		return;
	}


	const float ExpectedInterval =
		GetCurrentFireInterval();

	if (
		!FMath::IsNearlyEqual(
			ActiveTimerInterval,
			ExpectedInterval
		)
	)
	{
		RefreshFireTimer();
		return;
	}


	if (!CanFireAtTarget())
	{
		return;
	}


	FireAtTarget(TargetActor);

	CurrentAmmo =
		FMath::Max(
			0,
			CurrentAmmo - 1
		);

	BP_OnAmmoChanged(
		CurrentAmmo,
		MaxAmmo
	);


	if (CurrentAmmo <= 0)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[TurretBase] 탄약 소진. "
				"Turret=%s"
			),
			*GetNameSafe(this)
		);
	}
}


void ATurretBase::FireAtTarget(
	AActor* Target
)
{
	/*
	 * 자식 클래스에서 실제 발사를 구현한다.
	 */
}


void ATurretBase::RefillAmmo()
{
	CurrentAmmo = MaxAmmo;

	BP_OnAmmoChanged(
		CurrentAmmo,
		MaxAmmo
	);

	RefreshFireTimer();
}


void ATurretBase::AddAmmo(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	CurrentAmmo =
		FMath::Clamp(
			CurrentAmmo + Amount,
			0,
			MaxAmmo
		);

	BP_OnAmmoChanged(
		CurrentAmmo,
		MaxAmmo
	);
}


void ATurretBase::OnOperatingStateChanged(
	bool bCanOperateNow
)
{
	Super::OnOperatingStateChanged(
		bCanOperateNow
	);

	if (bCanOperateNow)
	{
		StartFireTimer();
	}
	else
	{
		StopFireTimer();
	}
}


void ATurretBase::HandleStructureDestroyed(
	AActor* DamageCauser
)
{
	StopFireTimer();
	ClearTargetActor();

	Super::HandleStructureDestroyed(
		DamageCauser
	);
}