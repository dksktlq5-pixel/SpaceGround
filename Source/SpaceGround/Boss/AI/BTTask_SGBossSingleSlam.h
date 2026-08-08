#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_SGBossSingleSlam.generated.h"

class USGBossCombatComponent;

/** Single Slam을 요청하고 전투 컴포넌트의 완료 신호까지 대기한다. */
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

private:
	UFUNCTION()
	void HandleAttackFinished(bool bSucceeded);

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(Transient)
	TObjectPtr<USGBossCombatComponent> ActiveCombatComponent;

	TWeakObjectPtr<UBehaviorTreeComponent> ActiveOwnerComp;
};
