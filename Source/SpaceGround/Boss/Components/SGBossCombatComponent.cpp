#include "SGBossCombatComponent.h"

#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

USGBossCombatComponent::USGBossCombatComponent()
{
	// 방향 추적이 필요한 공격 중에만 Tick을 켜서 사용
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

// 공격 준비 중 방향 추적, 몽타주가 없으면 테스트용 타이머도 처리
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

	if (AttackState == ESGBossAttackState::Tracking)
	{
		UpdateTargetTracking(DeltaTime);
	}

	if (bUsingMontageTiming)
	{
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

		if (AttackState == ESGBossAttackState::Recovering)
		{
			const float EndTime = SingleSlamData.FallbackWindupTime
				+ FMath::Max(0.0f, CommonData->RecoveryTime);
			if (AttackElapsedTime >= EndTime)
			{
				FinishCurrentAttack(true);
			}
		}
	}
}

// 공통 조건 확인 후 공격 상태 초기화와 몽타주 재생
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
	bDirectionLockedByNotify = false;
	bUsingMontageTiming = false;
	bFinishingAttack = false;
	AttackState = ESGBossAttackState::Tracking;
	LockedAttackDirection = OwnerActor->GetActorForwardVector().GetSafeNormal2D();
	if (LockedAttackDirection.IsNearlyZero())
	{
		LockedAttackDirection = FVector::ForwardVector;
	}

	const FSGAttackCommonData* CommonData = FindCommonAttackData(AttackType);
	if (!CommonData)
	{
		FinishCurrentAttack(false);
		return false;
	}

	bUsingMontageTiming = TryPlayAttackMontage(*CommonData);
	if (!bUsingMontageTiming && !CommonData->bAllowTimerFallback)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[BossCombat] Attack Montage is unavailable and timer fallback is disabled. Type=%d."),
			static_cast<int32>(AttackType));
		FinishCurrentAttack(false);
		return false;
	}

	SetComponentTickEnabled(true);
	BP_OnAttackStarted(AttackType);

	UE_LOG(LogTemp, Log, TEXT("[BossCombat] Attack started. Type=%d Target=%s"),
		static_cast<int32>(AttackType), *GetNameSafe(TargetActor));
	return true;
}

// Single Slam 종류를 공통 공격 시작 함수에 전달
bool USGBossCombatComponent::StartSingleSlam(AActor* TargetActor)
{
	return StartAttack(ESGBossAttackType::SingleSlam, TargetActor);
}

// 현재 공격이 진행 중이면 실패로 종료
void USGBossCombatComponent::CancelCurrentAttack()
{
	if (AttackState != ESGBossAttackState::Idle)
	{
		FinishCurrentAttack(false);
	}
}

// 방향 고정 Notify는 한 공격에서 한 번만 처리
void USGBossCombatComponent::NotifyLockAttackDirection()
{
	if (AttackState == ESGBossAttackState::Tracking && !bDirectionLockedByNotify)
	{
		bDirectionLockedByNotify = true;
		LockAttackDirection();
		if (bUsingMontageTiming)
		{
			SetComponentTickEnabled(false);
		}
	}
}

// 현재 공격 종류에 맞는 실제 타격 함수 호출
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

// 몽타주 재생 후 종료 Delegate와 멈춤 방지용 Safety Timer 등록
bool USGBossCombatComponent::TryPlayAttackMontage(
	const FSGAttackCommonData& CommonData)
{
	if (!IsValid(CommonData.Montage))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[BossCombat] No Montage assigned. Using timer fallback. Type=%d."),
			static_cast<int32>(CurrentAttackType));
		return false;
	}

	UAnimInstance* AnimInstance = GetOwnerAnimInstance();
	if (!IsValid(AnimInstance))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[BossCombat] Owner has no valid AnimInstance. Montage=%s."),
			*GetNameSafe(CommonData.Montage));
		return false;
	}

	const float MontageDuration = AnimInstance->Montage_Play(CommonData.Montage);
	if (MontageDuration <= 0.0f)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[BossCombat] Failed to play Montage %s."),
			*GetNameSafe(CommonData.Montage));
		return false;
	}

	ActiveAttackMontage = CommonData.Montage;
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &USGBossCombatComponent::HandleAttackMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, ActiveAttackMontage);

	if (UWorld* World = GetWorld())
	{
		const float SafetyTimeout = MontageDuration
			+ FMath::Max(0.0f, CommonData.RecoveryTime) + 1.0f;
		World->GetTimerManager().SetTimer(
			AttackSafetyTimerHandle,
			this,
			&USGBossCombatComponent::HandleAttackSafetyTimeout,
			SafetyTimeout,
			false);
	}

	return true;
}

// 보스 Character의 Skeletal Mesh가 사용하는 AnimInstance 반환
UAnimInstance* USGBossCombatComponent::GetOwnerAnimInstance() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerPawn);
	return OwnerCharacter && OwnerCharacter->GetMesh()
		? OwnerCharacter->GetMesh()->GetAnimInstance()
		: nullptr;
}

// 공격 중인지, 거리 안인지, 쿨다운이 끝났는지 확인
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

// 해당 공격을 다시 사용할 때까지 남은 시간 반환
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

// 공격 종류에 맞는 공통 밸런스 데이터 반환
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

// 잘못된 공격 설정 때문에 실행 중 문제가 생기기 전에 값 검사
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

// Windup 동안 플레이어를 향해 제한된 속도로 회전
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

// Notify가 호출된 순간의 전방 방향 저장
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

// 앞다리 Socket 위치에서 Sphere 판정 후 현재 타깃에게 한 번만 데미지 적용
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

	// Socket이 없을 때 사용할 기본 판정 위치부터 계산
	FVector HitCenter = OwnerActor->GetActorLocation()
		+ LockedAttackDirection * FMath::Max(0.0f, SingleSlamData.ForwardOffset);

	// 구매한 보스 Mesh에서 설정한 Socket 또는 Bone 이름 확인
	const USkeletalMeshComponent* SkeletalMesh = OwnerActor->FindComponentByClass<USkeletalMeshComponent>();
	const bool bHasValidHitSocket = SkeletalMesh
		&& !SingleSlamData.HitSocketName.IsNone()
		&& SkeletalMesh->DoesSocketExist(SingleSlamData.HitSocketName);

	if (bHasValidHitSocket)
	{
		// Socket의 현재 월드 위치에 BP에서 설정한 미세 Offset 적용
		const FTransform SocketTransform = SkeletalMesh->GetSocketTransform(
			SingleSlamData.HitSocketName, RTS_World);
		HitCenter = SocketTransform.TransformPosition(SingleSlamData.HitSocketOffset);
	}
	else
	{
		// Socket 설정이 잘못돼도 공격이 멈추지 않게 기본 위치 사용
		UE_LOG(LogTemp, Warning,
			TEXT("[BossCombat] Single Slam hit socket '%s' is invalid on %s. Using ForwardOffset fallback."),
			*SingleSlamData.HitSocketName.ToString(), *GetNameSafe(OwnerActor));
	}

	const float HitRadius = FMath::Max(1.0f, SingleSlamData.HitRadius);

	// Pawn 타입만 찾는 Sphere Overlap 실행, 판정에서 보스 자신은 제외
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SGBossSingleSlam), false, OwnerActor);
	TArray<FOverlapResult> Results;
	World->OverlapMultiByObjectType(Results, HitCenter, FQuat::Identity,
		ObjectQueryParams, FCollisionShape::MakeSphere(HitRadius), QueryParams);

	// Sphere 안에 들어온 Pawn 중 현재 공격 타깃만 확인
	bool bTargetHit = false;
	for (const FOverlapResult& Result : Results)
	{
		if (Result.GetActor() != CurrentTarget)
		{
			continue;
		}

		const APawn* OwnerPawn = Cast<APawn>(OwnerActor);
		// Unreal 기본 데미지 이벤트를 타깃에게 한 번 전달
		UGameplayStatics::ApplyDamage(CurrentTarget, SingleSlamData.Common.Damage,
			OwnerPawn ? OwnerPawn->GetController() : nullptr, OwnerActor, nullptr);
		bTargetHit = true;
		UE_LOG(LogTemp, Log, TEXT("[BossCombat] Single Slam hit %s for %.1f damage."),
			*GetNameSafe(CurrentTarget), SingleSlamData.Common.Damage);
		break;
	}

	// 명중은 초록색, 빗나감은 빨간색 Sphere로 표시
	if (bDrawDebugAttack)
	{
		DrawDebugSphere(World, HitCenter, HitRadius, 24,
			bTargetHit ? FColor::Green : FColor::Red, false, 1.5f, 0, 4.0f);
	}

	BP_OnAttackHit(CurrentAttackType);
	AttackState = ESGBossAttackState::Recovering;
}

// 몽타주 종료 후 설정된 후딜레이 시작
void USGBossCombatComponent::BeginRecovery()
{
	if (AttackState == ESGBossAttackState::Idle)
	{
		return;
	}

	AttackState = ESGBossAttackState::Recovering;
	SetComponentTickEnabled(false);

	const FSGAttackCommonData* CommonData = FindCommonAttackData(CurrentAttackType);
	const float RecoveryTime = CommonData
		? FMath::Max(0.0f, CommonData->RecoveryTime)
		: 0.0f;

	if (RecoveryTime <= 0.0f)
	{
		FinishCurrentAttack(true);
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			RecoveryTimerHandle,
			this,
			&USGBossCombatComponent::HandleRecoveryFinished,
			RecoveryTime,
			false);
	}
	else
	{
		FinishCurrentAttack(false);
	}
}

// 후딜레이가 끝나면 공격 성공 처리
void USGBossCombatComponent::HandleRecoveryFinished()
{
	FinishCurrentAttack(true);
}

// 몽타주 종료 이벤트가 오지 않아 공격 상태가 멈추는 상황 방지
void USGBossCombatComponent::HandleAttackSafetyTimeout()
{
	if (AttackState == ESGBossAttackState::Idle)
	{
		return;
	}

	UE_LOG(LogTemp, Error,
		TEXT("[BossCombat] Attack timed out and was cancelled. Type=%d Montage=%s."),
		static_cast<int32>(CurrentAttackType),
		*GetNameSafe(ActiveAttackMontage));
	FinishCurrentAttack(false);
}

// 몽타주 정상 종료, 중단, Notify 누락을 확인한 뒤 Recovery 진행
void USGBossCombatComponent::HandleAttackMontageEnded(
	UAnimMontage* Montage,
	const bool bInterrupted)
{
	if (bFinishingAttack || AttackState == ESGBossAttackState::Idle
		|| Montage != ActiveAttackMontage)
	{
		return;
	}

	ClearMontageDelegate();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AttackSafetyTimerHandle);
	}

	if (bInterrupted)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[BossCombat] Attack Montage was interrupted. Type=%d Montage=%s."),
			static_cast<int32>(CurrentAttackType), *GetNameSafe(Montage));
		FinishCurrentAttack(false);
		return;
	}

	if (!bDirectionLockedByNotify)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[BossCombat] Montage ended without LockDirection Notify. Montage=%s."),
			*GetNameSafe(Montage));
	}

	if (!bHitExecuted)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[BossCombat] Montage ended without AttackHit Notify. Montage=%s."),
			*GetNameSafe(Montage));
	}

	ActiveAttackMontage = nullptr;
	BeginRecovery();
}

// 공격에서 사용한 Recovery와 Safety Timer 정리
void USGBossCombatComponent::ClearAttackTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RecoveryTimerHandle);
		World->GetTimerManager().ClearTimer(AttackSafetyTimerHandle);
	}
}

// 이전 몽타주의 종료 이벤트가 다음 공격에 남지 않게 해제
void USGBossCombatComponent::ClearMontageDelegate()
{
	if (!IsValid(ActiveAttackMontage))
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetOwnerAnimInstance())
	{
		FOnMontageEnded EmptyEndDelegate;
		AnimInstance->Montage_SetEndDelegate(EmptyEndDelegate, ActiveAttackMontage);
	}
}

// 쿨다운 기록 후 공격 상태와 임시 참조를 모두 초기화
void USGBossCombatComponent::FinishCurrentAttack(const bool bSucceeded)
{
	if (bFinishingAttack || AttackState == ESGBossAttackState::Idle)
	{
		return;
	}

	bFinishingAttack = true;
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

	ClearAttackTimers();
	ClearMontageDelegate();
	if (IsValid(ActiveAttackMontage))
	{
		if (UAnimInstance* AnimInstance = GetOwnerAnimInstance())
		{
			if (AnimInstance->Montage_IsPlaying(ActiveAttackMontage))
			{
				AnimInstance->Montage_Stop(0.1f, ActiveAttackMontage);
			}
		}
	}

	SetComponentTickEnabled(false);
	ActiveAttackMontage = nullptr;
	CurrentTarget = nullptr;
	AttackElapsedTime = 0.0f;
	bHitExecuted = false;
	bUsingMontageTiming = false;
	bDirectionLockedByNotify = false;
	AttackState = ESGBossAttackState::Idle;
	CurrentAttackType = ESGBossAttackType::None;

	OnBossAttackFinished.Broadcast(FinishedAttackType, bSucceeded);
	bFinishingAttack = false;
}
