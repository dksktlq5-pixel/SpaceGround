#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "SpaceGround/CommonData/SGBossTypes.h"

#include "BTTask_SGBossSingleSlam.generated.h"

class AActor;
class USGBossCombatComponent;


UCLASS()
class SPACEGROUND_API UBTTask_SGBossSingleSlam : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_SGBossSingleSlam();

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual EBTNodeResult::Type AbortTask(
		UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

private:
	void TryStartPendingAttack();
	void CleanupTaskState(bool bCancelAttack);

	UFUNCTION()
	void HandleAttackFinished(ESGBossAttackType AttackType, bool bSucceeded);

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(Transient)
	TObjectPtr<USGBossCombatComponent> ActiveCombatComponent;

	TWeakObjectPtr<UBehaviorTreeComponent> ActiveOwnerComp;
	TWeakObjectPtr<AActor> PendingTarget;
	FTimerHandle CooldownWaitTimerHandle;
};
