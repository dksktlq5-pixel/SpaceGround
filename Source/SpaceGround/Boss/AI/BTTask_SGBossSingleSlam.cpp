#include "BTTask_SGBossSingleSlam.h"

#include "SpaceGround/Boss/SGBossCharacter.h"
#include "SpaceGround/Boss/Components/SGBossCombatComponent.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

UBTTask_SGBossSingleSlam::UBTTask_SGBossSingleSlam()
{
	NodeName = TEXT("Single Slam");
	bCreateNodeInstance = true;
	bNotifyTaskFinished = true;
	TargetActorKey.AddObjectFilter(this,
		GET_MEMBER_NAME_CHECKED(UBTTask_SGBossSingleSlam, TargetActorKey),
		AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_SGBossSingleSlam::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	ASGBossCharacter* BossCharacter = AIController
		? Cast<ASGBossCharacter>(AIController->GetPawn()) : nullptr;
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	AActor* TargetActor = Blackboard
		? Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName)) : nullptr;

	if (!IsValid(BossCharacter) || !IsValid(TargetActor))
	{
		return EBTNodeResult::Failed;
	}

	USGBossCombatComponent* CombatComponent = BossCharacter->GetBossCombatComponent();
	if (!IsValid(CombatComponent))
	{
		return EBTNodeResult::Failed;
	}

	ActiveCombatComponent = CombatComponent;
	ActiveOwnerComp = &OwnerComp;
	PendingTarget = TargetActor;
	CombatComponent->OnBossAttackFinished.AddUniqueDynamic(
		this, &UBTTask_SGBossSingleSlam::HandleAttackFinished);

	const float RemainingCooldown = CombatComponent->GetRemainingCooldown(
		ESGBossAttackType::SingleSlam);
	if (RemainingCooldown > 0.0f)
	{
		BossCharacter->GetWorldTimerManager().SetTimer(
			CooldownWaitTimerHandle, this,
			&UBTTask_SGBossSingleSlam::TryStartPendingAttack,
			RemainingCooldown, false);
		return EBTNodeResult::InProgress;
	}

	if (!CombatComponent->StartSingleSlam(TargetActor))
	{
		CleanupTaskState(false);
		return EBTNodeResult::Failed;
	}

	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UBTTask_SGBossSingleSlam::AbortTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	CleanupTaskState(true);
	return EBTNodeResult::Aborted;
}

void UBTTask_SGBossSingleSlam::OnTaskFinished(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	const EBTNodeResult::Type TaskResult)
{
	CleanupTaskState(false);
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

void UBTTask_SGBossSingleSlam::TryStartPendingAttack()
{
	UBehaviorTreeComponent* OwnerComp = ActiveOwnerComp.Get();
	AActor* TargetActor = PendingTarget.Get();
	if (!IsValid(OwnerComp) || !IsValid(ActiveCombatComponent) || !IsValid(TargetActor))
	{
		if (IsValid(OwnerComp))
		{
			FinishLatentTask(*OwnerComp, EBTNodeResult::Failed);
		}
		else
		{
			CleanupTaskState(false);
		}
		return;
	}

	if (!ActiveCombatComponent->StartSingleSlam(TargetActor))
	{
		FinishLatentTask(*OwnerComp, EBTNodeResult::Failed);
	}
}

void UBTTask_SGBossSingleSlam::CleanupTaskState(const bool bCancelAttack)
{
	if (IsValid(ActiveCombatComponent))
	{
		ActiveCombatComponent->OnBossAttackFinished.RemoveDynamic(
			this, &UBTTask_SGBossSingleSlam::HandleAttackFinished);
		if (bCancelAttack)
		{
			ActiveCombatComponent->CancelCurrentAttack();
		}
	}

	if (UBehaviorTreeComponent* OwnerComp = ActiveOwnerComp.Get())
	{
		if (UWorld* World = OwnerComp->GetWorld())
		{
			World->GetTimerManager().ClearTimer(CooldownWaitTimerHandle);
		}
	}

	ActiveCombatComponent = nullptr;
	ActiveOwnerComp.Reset();
	PendingTarget.Reset();
}

void UBTTask_SGBossSingleSlam::HandleAttackFinished(
	const ESGBossAttackType AttackType,
	const bool bSucceeded)
{
	if (AttackType != ESGBossAttackType::SingleSlam)
	{
		return;
	}

	UBehaviorTreeComponent* OwnerComp = ActiveOwnerComp.Get();
	if (IsValid(OwnerComp))
	{
		FinishLatentTask(*OwnerComp,
			bSucceeded ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);
	}
	else
	{
		CleanupTaskState(false);
	}
}
