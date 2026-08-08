#include "BTTask_SGBossSingleSlam.h"

#include "SpaceGround/Boss/SGBossCharacter.h"
#include "SpaceGround/Boss/Components/SGBossCombatComponent.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_SGBossSingleSlam::UBTTask_SGBossSingleSlam()
{
	NodeName = TEXT("Single Slam");
	bCreateNodeInstance = true;
	TargetActorKey.AddObjectFilter(
		this,
		GET_MEMBER_NAME_CHECKED(UBTTask_SGBossSingleSlam, TargetActorKey),
		AActor::StaticClass()
	);
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
	CombatComponent->OnBossAttackFinished.AddDynamic(
		this, &UBTTask_SGBossSingleSlam::HandleAttackFinished);

	if (!CombatComponent->StartSingleSlam(TargetActor))
	{
		CombatComponent->OnBossAttackFinished.RemoveDynamic(
			this, &UBTTask_SGBossSingleSlam::HandleAttackFinished);
		ActiveCombatComponent = nullptr;
		ActiveOwnerComp.Reset();
		return EBTNodeResult::Failed;
	}

	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UBTTask_SGBossSingleSlam::AbortTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (IsValid(ActiveCombatComponent))
	{
		ActiveCombatComponent->OnBossAttackFinished.RemoveDynamic(
			this, &UBTTask_SGBossSingleSlam::HandleAttackFinished);
		ActiveCombatComponent->CancelCurrentAttack();
	}

	ActiveCombatComponent = nullptr;
	ActiveOwnerComp.Reset();
	return EBTNodeResult::Aborted;
}

void UBTTask_SGBossSingleSlam::HandleAttackFinished(const bool bSucceeded)
{
	if (IsValid(ActiveCombatComponent))
	{
		ActiveCombatComponent->OnBossAttackFinished.RemoveDynamic(
			this, &UBTTask_SGBossSingleSlam::HandleAttackFinished);
	}

	UBehaviorTreeComponent* OwnerComp = ActiveOwnerComp.Get();
	ActiveCombatComponent = nullptr;
	ActiveOwnerComp.Reset();

	if (IsValid(OwnerComp))
	{
		FinishLatentTask(*OwnerComp,
			bSucceeded ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);
	}
}
