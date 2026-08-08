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
	if (AttackState == ESGBossAttackState::Idle) return;

	AttackElapsedTime += DeltaTime;
	const float LockTime = FMath::Max(0.0f, SingleSlamWindupTime - DirectionLockLeadTime);

	if (AttackState == ESGBossAttackState::Tracking)
	{
		UpdateTargetTracking(DeltaTime);
		if (AttackState != ESGBossAttackState::Idle && AttackElapsedTime >= LockTime)
		{
			LockAttackDirection();
		}
	}

	if (AttackState != ESGBossAttackState::Idle && !bHitExecuted
		&& AttackElapsedTime >= SingleSlamWindupTime)
	{
		ExecuteSingleSlamHit();
	}

	const float EndTime = SingleSlamWindupTime + SingleSlamRecoveryTime + SingleSlamCooldown;
	if (AttackState != ESGBossAttackState::Idle && AttackElapsedTime >= EndTime)
	{
		FinishCurrentAttack(true);
	}
}

bool USGBossCombatComponent::StartSingleSlam(AActor* TargetActor)
{
	AActor* OwnerActor = GetOwner();
	if (AttackState != ESGBossAttackState::Idle || !IsValid(OwnerActor) || !IsValid(TargetActor))
	{
		return false;
	}

	const float Distance = FVector::Dist2D(OwnerActor->GetActorLocation(), TargetActor->GetActorLocation());
	if (Distance > SingleSlamAttackRange)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[BossCombat] Single Slam rejected. Distance=%.1f Range=%.1f"),
			Distance, SingleSlamAttackRange);
		return false;
	}

	if (const APawn* OwnerPawn = Cast<APawn>(OwnerActor))
	{
		if (AAIController* AIController = Cast<AAIController>(OwnerPawn->GetController()))
		{
			AIController->StopMovement();
		}
	}

	CurrentTarget = TargetActor;
	AttackElapsedTime = 0.0f;
	bHitExecuted = false;
	AttackState = ESGBossAttackState::Tracking;
	LockedAttackDirection = OwnerActor->GetActorForwardVector();
	SetComponentTickEnabled(true);
	BP_OnSingleSlamStarted();
	UE_LOG(LogTemp, Log, TEXT("[BossCombat] Single Slam started. Target=%s"), *GetNameSafe(TargetActor));
	return true;
}

void USGBossCombatComponent::CancelCurrentAttack()
{
	if (AttackState != ESGBossAttackState::Idle) FinishCurrentAttack(false);
}

void USGBossCombatComponent::UpdateTargetTracking(const float DeltaTime)
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor) || !IsValid(CurrentTarget))
	{
		FinishCurrentAttack(false);
		return;
	}

	FVector Direction = CurrentTarget->GetActorLocation() - OwnerActor->GetActorLocation();
	Direction.Z = 0.0f;
	if (Direction.IsNearlyZero()) return;

	const FRotator NewRotation = FMath::RInterpConstantTo(
		OwnerActor->GetActorRotation(), Direction.Rotation(), DeltaTime, TrackingRotationSpeed);
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
	AttackState = ESGBossAttackState::DirectionLocked;
	BP_OnSingleSlamDirectionLocked();
	UE_LOG(LogTemp, Log, TEXT("[BossCombat] Single Slam direction locked."));
}

void USGBossCombatComponent::ExecuteSingleSlamHit()
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	if (!IsValid(OwnerActor) || !World)
	{
		FinishCurrentAttack(false);
		return;
	}

	bHitExecuted = true;
	AttackState = ESGBossAttackState::Hitting;
	const FVector HitCenter = OwnerActor->GetActorLocation()
		+ LockedAttackDirection * SingleSlamForwardOffset;

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SGBossSingleSlam), false, OwnerActor);
	TArray<FOverlapResult> Results;
	World->OverlapMultiByObjectType(Results, HitCenter, FQuat::Identity,
		ObjectQueryParams, FCollisionShape::MakeSphere(SingleSlamHitRadius), QueryParams);

	TSet<AActor*> DamagedActors;
	for (const FOverlapResult& Result : Results)
	{
		AActor* HitActor = Result.GetActor();
		if (!IsValid(HitActor) || HitActor == OwnerActor || DamagedActors.Contains(HitActor)) continue;

		DamagedActors.Add(HitActor);
		APawn* OwnerPawn = Cast<APawn>(OwnerActor);
		UGameplayStatics::ApplyDamage(HitActor, SingleSlamDamage,
			OwnerPawn ? OwnerPawn->GetController() : nullptr, OwnerActor, nullptr);
		UE_LOG(LogTemp, Log, TEXT("[BossCombat] Single Slam hit %s for %.1f damage."),
			*GetNameSafe(HitActor), SingleSlamDamage);
	}

	if (bDrawDebugAttack)
	{
		DrawDebugSphere(World, HitCenter, SingleSlamHitRadius, 24,
			DamagedActors.IsEmpty() ? FColor::Red : FColor::Green,
			false, 1.5f, 0, 4.0f);
	}

	BP_OnSingleSlamHit();
	AttackState = ESGBossAttackState::Recovering;
}

void USGBossCombatComponent::FinishCurrentAttack(const bool bSucceeded)
{
	SetComponentTickEnabled(false);
	CurrentTarget = nullptr;
	AttackElapsedTime = 0.0f;
	bHitExecuted = false;
	AttackState = ESGBossAttackState::Idle;
	OnBossAttackFinished.Broadcast(bSucceeded);
}
