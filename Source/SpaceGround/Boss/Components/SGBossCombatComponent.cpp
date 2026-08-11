#include "SGBossCombatComponent.h"

#include "AIController.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

USGBossCombatComponent::USGBossCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void USGBossCombatComponent::TickComponent(const float DeltaTime,
	const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (AttackState == ESGBossAttackState::Idle)
	{
		SetComponentTickEnabled(false);
		return;
	}

	if (!IsValid(CurrentTarget))
	{
		FinishCurrentAttack(false);
		return;
	}

	const FSGAttackCommonData* CommonData = FindCommonAttackData(CurrentAttackType);
	if (!CommonData)
	{
		FinishCurrentAttack(false);
		return;
	}

	AttackElapsedTime += DeltaTime;

	if (CurrentAttackType == ESGBossAttackType::SingleSlam)
	{
		const float LockTime = FMath::Max(0.0f,
			SingleSlamData.FallbackWindupTime
			- SingleSlamData.FallbackDirectionLockLeadTime);

		if (AttackState == ESGBossAttackState::Tracking)
		{
			UpdateTargetTracking(DeltaTime);
			if (AttackState != ESGBossAttackState::Idle && AttackElapsedTime >= LockTime)
			{
				NotifyLockAttackDirection();
			}
		}

		if (AttackState != ESGBossAttackState::Idle && !bHitExecuted
			&& AttackElapsedTime >= SingleSlamData.FallbackWindupTime)
		{
			NotifyExecuteCurrentAttackHit();
		}

		const float EndTime = SingleSlamData.FallbackWindupTime
			+ FMath::Max(0.0f, CommonData->RecoveryTime);
		if (AttackState != ESGBossAttackState::Idle && AttackElapsedTime >= EndTime)
		{
			FinishCurrentAttack(true);
		}
	}
}

bool USGBossCombatComponent::StartAttack(
	const ESGBossAttackType AttackType,
	AActor* TargetActor)
{
	if (!CanStartAttack(AttackType, TargetActor) || !ValidateAttackData(AttackType))
	{
		return false;
	}

	AActor* OwnerActor = GetOwner();
	if (const APawn* OwnerPawn = Cast<APawn>(OwnerActor))
	{
		if (AAIController* AIController = Cast<AAIController>(OwnerPawn->GetController()))
		{
			AIController->StopMovement();
		}
	}

	CurrentAttackType = AttackType;
	CurrentTarget = TargetActor;
	AttackElapsedTime = 0.0f;
	bHitExecuted = false;
	AttackState = ESGBossAttackState::Tracking;
	LockedAttackDirection = OwnerActor->GetActorForwardVector().GetSafeNormal2D();
	if (LockedAttackDirection.IsNearlyZero())
	{
		LockedAttackDirection = FVector::ForwardVector;
	}

	SetComponentTickEnabled(true);
	BP_OnAttackStarted(AttackType);

	UE_LOG(LogTemp, Log, TEXT("[BossCombat] Attack started. Type=%d Target=%s"),
		static_cast<int32>(AttackType), *GetNameSafe(TargetActor));
	return true;
}

bool USGBossCombatComponent::StartSingleSlam(AActor* TargetActor)
{
	return StartAttack(ESGBossAttackType::SingleSlam, TargetActor);
}

void USGBossCombatComponent::CancelCurrentAttack()
{
	if (AttackState != ESGBossAttackState::Idle)
	{
		FinishCurrentAttack(false);
	}
}

void USGBossCombatComponent::NotifyLockAttackDirection()
{
	if (AttackState == ESGBossAttackState::Tracking)
	{
		LockAttackDirection();
	}
}

void USGBossCombatComponent::NotifyExecuteCurrentAttackHit()
{
	if (AttackState == ESGBossAttackState::Idle || bHitExecuted)
	{
		return;
	}

	switch (CurrentAttackType)
	{
	case ESGBossAttackType::SingleSlam:
		ExecuteSingleSlamHit();
		break;

	default:
		UE_LOG(LogTemp, Warning, TEXT("[BossCombat] No hit implementation for AttackType=%d."),
			static_cast<int32>(CurrentAttackType));
		FinishCurrentAttack(false);
		break;
	}
}

bool USGBossCombatComponent::CanStartAttack(
	const ESGBossAttackType AttackType,
	AActor* TargetActor) const
{
	const AActor* OwnerActor = GetOwner();
	const FSGAttackCommonData* CommonData = FindCommonAttackData(AttackType);
	if (AttackState != ESGBossAttackState::Idle || !IsValid(OwnerActor)
		|| !IsValid(TargetActor) || !CommonData)
	{
		return false;
	}

	if (GetRemainingCooldown(AttackType) > 0.0f)
	{
		return false;
	}

	const float Distance = FVector::Dist2D(
		OwnerActor->GetActorLocation(), TargetActor->GetActorLocation());
	const float MinimumRange = FMath::Max(0.0f, CommonData->MinimumRange);
	const float MaximumRange = FMath::Max(MinimumRange, CommonData->ActivationRange);
	return Distance >= MinimumRange && Distance <= MaximumRange;
}

float USGBossCombatComponent::GetRemainingCooldown(
	const ESGBossAttackType AttackType) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0f;
	}

	const float* NextAllowedTime = NextAttackAllowedTimes.Find(AttackType);
	return NextAllowedTime
		? FMath::Max(0.0f, *NextAllowedTime - World->GetTimeSeconds())
		: 0.0f;
}

const FSGAttackCommonData* USGBossCombatComponent::FindCommonAttackData(
	const ESGBossAttackType AttackType) const
{
	switch (AttackType)
	{
	case ESGBossAttackType::SingleSlam:
		return &SingleSlamData.Common;

	default:
		return nullptr;
	}
}

bool USGBossCombatComponent::ValidateAttackData(
	const ESGBossAttackType AttackType) const
{
	const FSGAttackCommonData* CommonData = FindCommonAttackData(AttackType);
	if (!CommonData || CommonData->ActivationRange < 0.0f
		|| CommonData->MinimumRange < 0.0f || CommonData->Damage < 0.0f)
	{
		UE_LOG(LogTemp, Error, TEXT("[BossCombat] Invalid common attack data. Type=%d."),
			static_cast<int32>(AttackType));
		return false;
	}

	if (AttackType == ESGBossAttackType::SingleSlam
		&& (SingleSlamData.HitRadius <= 0.0f
			|| SingleSlamData.FallbackWindupTime <= 0.0f))
	{
		UE_LOG(LogTemp, Error, TEXT("[BossCombat] Invalid Single Slam data."));
		return false;
	}

	return true;
}

void USGBossCombatComponent::UpdateTargetTracking(const float DeltaTime)
{
	AActor* OwnerActor = GetOwner();
	const FSGAttackCommonData* CommonData = FindCommonAttackData(CurrentAttackType);
	if (!IsValid(OwnerActor) || !IsValid(CurrentTarget) || !CommonData)
	{
		FinishCurrentAttack(false);
		return;
	}

	FVector Direction = CurrentTarget->GetActorLocation() - OwnerActor->GetActorLocation();
	Direction.Z = 0.0f;
	if (Direction.IsNearlyZero())
	{
		return;
	}

	const FRotator NewRotation = FMath::RInterpConstantTo(
		OwnerActor->GetActorRotation(), Direction.Rotation(), DeltaTime,
		FMath::Max(0.0f, CommonData->TrackingRotationSpeed));
	OwnerActor->SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
}

void USGBossCombatComponent::LockAttackDirection()
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		FinishCurrentAttack(false);
		return;
	}

	LockedAttackDirection = OwnerActor->GetActorForwardVector().GetSafeNormal2D();
	if (LockedAttackDirection.IsNearlyZero())
	{
		LockedAttackDirection = FVector::ForwardVector;
	}

	AttackState = ESGBossAttackState::DirectionLocked;
	BP_OnAttackDirectionLocked(CurrentAttackType);
	UE_LOG(LogTemp, Log, TEXT("[BossCombat] Attack direction locked. Type=%d."),
		static_cast<int32>(CurrentAttackType));
}

void USGBossCombatComponent::ExecuteSingleSlamHit()
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	if (!IsValid(OwnerActor) || !World || !IsValid(CurrentTarget))
	{
		FinishCurrentAttack(false);
		return;
	}

	bHitExecuted = true;
	AttackState = ESGBossAttackState::Hitting;

	const FVector HitCenter = OwnerActor->GetActorLocation()
		+ LockedAttackDirection * FMath::Max(0.0f, SingleSlamData.ForwardOffset);
	const float HitRadius = FMath::Max(1.0f, SingleSlamData.HitRadius);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SGBossSingleSlam), false, OwnerActor);
	TArray<FOverlapResult> Results;
	World->OverlapMultiByObjectType(Results, HitCenter, FQuat::Identity,
		ObjectQueryParams, FCollisionShape::MakeSphere(HitRadius), QueryParams);

	bool bTargetHit = false;
	for (const FOverlapResult& Result : Results)
	{
		if (Result.GetActor() != CurrentTarget)
		{
			continue;
		}

		const APawn* OwnerPawn = Cast<APawn>(OwnerActor);
		UGameplayStatics::ApplyDamage(CurrentTarget, SingleSlamData.Common.Damage,
			OwnerPawn ? OwnerPawn->GetController() : nullptr, OwnerActor, nullptr);
		bTargetHit = true;
		UE_LOG(LogTemp, Log, TEXT("[BossCombat] Single Slam hit %s for %.1f damage."),
			*GetNameSafe(CurrentTarget), SingleSlamData.Common.Damage);
		break;
	}

	if (bDrawDebugAttack)
	{
		DrawDebugSphere(World, HitCenter, HitRadius, 24,
			bTargetHit ? FColor::Green : FColor::Red, false, 1.5f, 0, 4.0f);
	}

	BP_OnAttackHit(CurrentAttackType);
	AttackState = ESGBossAttackState::Recovering;
}

void USGBossCombatComponent::FinishCurrentAttack(const bool bSucceeded)
{
	const ESGBossAttackType FinishedAttackType = CurrentAttackType;
	const FSGAttackCommonData* CommonData = FindCommonAttackData(FinishedAttackType);
	if (bSucceeded && CommonData)
	{
		if (const UWorld* World = GetWorld())
		{
			NextAttackAllowedTimes.FindOrAdd(FinishedAttackType) =
				World->GetTimeSeconds() + FMath::Max(0.0f, CommonData->Cooldown);
		}
	}

	SetComponentTickEnabled(false);
	CurrentTarget = nullptr;
	AttackElapsedTime = 0.0f;
	bHitExecuted = false;
	AttackState = ESGBossAttackState::Idle;
	CurrentAttackType = ESGBossAttackType::None;

	OnBossAttackFinished.Broadcast(FinishedAttackType, bSucceeded);
}
