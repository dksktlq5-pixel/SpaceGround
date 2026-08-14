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
	// BT Editor에서 보일 이름
	NodeName = TEXT("Single Slam");
	// Task마다 독립된 타이머와 참조를 가질 수 있게 인스턴스 생성
	bCreateNodeInstance = true;
	// Task 종료 시 OnTaskFinished 호출
	bNotifyTaskFinished = true;
	// Actor 타입 Blackboard Key만 선택 가능
	TargetActorKey.AddObjectFilter(this,
		GET_MEMBER_NAME_CHECKED(UBTTask_SGBossSingleSlam, TargetActorKey),
		AActor::StaticClass());
}

// Blackboard의 타깃을 가져와 Single Slam 시작
EBTNodeResult::Type UBTTask_SGBossSingleSlam::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	ASGBossCharacter* BossCharacter = AIController
		? Cast<ASGBossCharacter>(AIController->GetPawn()) : nullptr;
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	AActor* TargetActor = Blackboard
		? Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName)) : nullptr;

	// 보스나 타깃이 없으면 공격 시작 불가
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
	// 공격이 끝날 때까지 BT Task를 InProgress 상태로 유지
	CombatComponent->OnBossAttackFinished.AddUniqueDynamic(
		this, &UBTTask_SGBossSingleSlam::HandleAttackFinished);

	const float RemainingCooldown = CombatComponent->GetRemainingCooldown(
		ESGBossAttackType::SingleSlam);
	// 쿨다운이 남았으면 Timer로 기다렸다가 공격 시작
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

// 쿨다운이 끝난 뒤 보스와 타깃을 다시 확인하고 공격 시작
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

// Task에서 사용한 이벤트와 Timer가 다음 실행에 남지 않게 정리
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

// 공격 결과를 BT의 성공 또는 실패로 전달
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
