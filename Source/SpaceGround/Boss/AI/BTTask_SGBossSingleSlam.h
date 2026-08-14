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
	// BT가 이 Task에 도착하면 Single Slam 시작 요청
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	// BT가 Task를 중간에 취소하면 진행 중인 공격도 정리
	virtual EBTNodeResult::Type AbortTask(
		UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	// 성공, 실패, 취소와 상관없이 마지막에 실행되는 정리 지점
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

private:
	// 쿨다운이 끝난 뒤 기다리던 공격 시작
	void TryStartPendingAttack();
	// 이벤트, 타이머, 임시 참조 정리
	void CleanupTaskState(bool bCancelAttack);

	// CombatComponent가 공격 종료를 알리면 BT에 결과 전달
	UFUNCTION()
	void HandleAttackFinished(ESGBossAttackType AttackType, bool bSucceeded);

	// Blackboard에서 공격 대상을 가져올 Key
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(Transient)
	TObjectPtr<USGBossCombatComponent> ActiveCombatComponent;

	TWeakObjectPtr<UBehaviorTreeComponent> ActiveOwnerComp;
	TWeakObjectPtr<AActor> PendingTarget;
	FTimerHandle CooldownWaitTimerHandle;
};
