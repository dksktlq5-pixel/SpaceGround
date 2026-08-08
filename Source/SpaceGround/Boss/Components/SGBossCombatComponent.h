#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpaceGround/CommonData/SGBossTypes.h"
#include "SGBossCombatComponent.generated.h"

class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSGOnBossAttackFinished, bool, bSucceeded);

/** 보스 공격 상태와 공격 패턴을 담당하는 전투 컴포넌트. */
UCLASS(ClassGroup = (SpaceGround), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class SPACEGROUND_API USGBossCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USGBossCombatComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Boss|Combat")
	bool StartSingleSlam(AActor* TargetActor);

	UFUNCTION(BlueprintCallable, Category = "Boss|Combat")
	void CancelCurrentAttack();

	UFUNCTION(BlueprintPure, Category = "Boss|Combat")
	bool IsAttacking() const { return AttackState != ESGBossAttackState::Idle; }

	UPROPERTY(BlueprintAssignable, Category = "Boss|Combat")
	FSGOnBossAttackFinished OnBossAttackFinished;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Combat|Single Slam", meta = (ClampMin = "0.0"))
	float SingleSlamDamage = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Combat|Single Slam", meta = (ClampMin = "0.0", Units = "cm"))
	float SingleSlamAttackRange = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Combat|Single Slam", meta = (ClampMin = "0.0", Units = "cm"))
	float SingleSlamForwardOffset = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Combat|Single Slam", meta = (ClampMin = "1.0", Units = "cm"))
	float SingleSlamHitRadius = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Combat|Single Slam", meta = (ClampMin = "0.01", Units = "s"))
	float SingleSlamWindupTime = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Combat|Single Slam", meta = (ClampMin = "0.0", Units = "s"))
	float DirectionLockLeadTime = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Combat|Single Slam", meta = (ClampMin = "0.0", Units = "s"))
	float SingleSlamRecoveryTime = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Combat|Single Slam", meta = (ClampMin = "0.0", Units = "s"))
	float SingleSlamCooldown = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Combat|Single Slam", meta = (ClampMin = "0.0", Units = "deg/s"))
	float TrackingRotationSpeed = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Debug")
	bool bDrawDebugAttack = true;

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Combat|Animation")
	void BP_OnSingleSlamStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Combat|Animation")
	void BP_OnSingleSlamDirectionLocked();

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Combat|Animation")
	void BP_OnSingleSlamHit();

private:
	void UpdateTargetTracking(float DeltaTime);
	void LockAttackDirection();
	void ExecuteSingleSlamHit();
	void FinishCurrentAttack(bool bSucceeded);

	UPROPERTY(VisibleInstanceOnly, Category = "Boss|Combat")
	ESGBossAttackState AttackState = ESGBossAttackState::Idle;

	UPROPERTY(Transient)
	TObjectPtr<AActor> CurrentTarget;

	float AttackElapsedTime = 0.0f;
	FVector LockedAttackDirection = FVector::ForwardVector;
	bool bHitExecuted = false;
};
