#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpaceGround/CommonData/SGBossTypes.h"

#include "SGBossCombatComponent.generated.h"

class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FSGOnBossAttackFinished,
	ESGBossAttackType,
	AttackType,
	bool,
	bSucceeded
);

/** 공격 상태, 공통 타이밍과 공격별 공간 판정을 관리한다. */
UCLASS(ClassGroup = (SpaceGround), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class SPACEGROUND_API USGBossCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USGBossCombatComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Boss|Combat")
	bool StartAttack(ESGBossAttackType AttackType, AActor* TargetActor);

	UFUNCTION(BlueprintCallable, Category = "Boss|Combat")
	bool StartSingleSlam(AActor* TargetActor);

	UFUNCTION(BlueprintCallable, Category = "Boss|Combat")
	void CancelCurrentAttack();

	/** 모든 공격 Montage가 공유할 방향 고정 Notify 진입점. */
	UFUNCTION(BlueprintCallable, Category = "Boss|Combat|Notify")
	void NotifyLockAttackDirection();

	/** 모든 공격 Montage가 공유할 타격 Notify 진입점. */
	UFUNCTION(BlueprintCallable, Category = "Boss|Combat|Notify")
	void NotifyExecuteCurrentAttackHit();

	UFUNCTION(BlueprintPure, Category = "Boss|Combat")
	bool CanStartAttack(ESGBossAttackType AttackType, AActor* TargetActor) const;

	UFUNCTION(BlueprintPure, Category = "Boss|Combat")
	float GetRemainingCooldown(ESGBossAttackType AttackType) const;

	UFUNCTION(BlueprintPure, Category = "Boss|Combat")
	bool IsAttacking() const { return AttackState != ESGBossAttackState::Idle; }

	UFUNCTION(BlueprintPure, Category = "Boss|Combat")
	ESGBossAttackType GetCurrentAttackType() const { return CurrentAttackType; }

	UPROPERTY(BlueprintAssignable, Category = "Boss|Combat")
	FSGOnBossAttackFinished OnBossAttackFinished;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Combat|Attacks")
	FSGSingleSlamAttackData SingleSlamData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Debug")
	bool bDrawDebugAttack = true;

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Combat|Animation")
	void BP_OnAttackStarted(ESGBossAttackType AttackType);

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Combat|Animation")
	void BP_OnAttackDirectionLocked(ESGBossAttackType AttackType);

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Combat|Animation")
	void BP_OnAttackHit(ESGBossAttackType AttackType);

private:
	const FSGAttackCommonData* FindCommonAttackData(ESGBossAttackType AttackType) const;
	bool ValidateAttackData(ESGBossAttackType AttackType) const;
	void UpdateTargetTracking(float DeltaTime);
	void LockAttackDirection();
	void ExecuteSingleSlamHit();
	void FinishCurrentAttack(bool bSucceeded);

	UPROPERTY(VisibleInstanceOnly, Category = "Boss|Combat")
	ESGBossAttackState AttackState = ESGBossAttackState::Idle;

	UPROPERTY(VisibleInstanceOnly, Category = "Boss|Combat")
	ESGBossAttackType CurrentAttackType = ESGBossAttackType::None;

	UPROPERTY(Transient)
	TObjectPtr<AActor> CurrentTarget;

	UPROPERTY(Transient)
	TMap<ESGBossAttackType, float> NextAttackAllowedTimes;

	float AttackElapsedTime = 0.0f;
	FVector LockedAttackDirection = FVector::ForwardVector;
	bool bHitExecuted = false;
};
