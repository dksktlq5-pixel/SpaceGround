#include "SGBossAIController.h"

#include "SGBossCharacter.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"

#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

const FName ASGBossAIController::TargetActorKeyName(
	TEXT("TargetActor")
);

ASGBossAIController::ASGBossAIController()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ASGBossAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	const ASGBossCharacter* BossCharacter =
		Cast<ASGBossCharacter>(InPawn);

	if (!IsValid(BossCharacter))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[BossAI] Possessed Pawn is not SGBossCharacter.")
		);
		return;
	}

	UBehaviorTree* BehaviorTree =
		BossCharacter->GetBossBehaviorTree();

	if (!IsValid(BehaviorTree))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[BossAI] BossBehaviorTree is not assigned on %s."
			),
			*GetNameSafe(BossCharacter)
		);
		return;
	}

	if (!RunBehaviorTree(BehaviorTree))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[BossAI] Failed to run Behavior Tree %s."),
			*GetNameSafe(BehaviorTree)
		);
		return;
	}


	TryAcquirePlayerTarget();
}

void ASGBossAIController::TryAcquirePlayerTarget()
{
	APawn* PlayerPawn =
		UGameplayStatics::GetPlayerPawn(this, 0);

	if (!IsValid(PlayerPawn) || !Blackboard)
	{
		
		TargetAcquisitionTimerHandle =
			GetWorldTimerManager().SetTimerForNextTick(
			this,
			&ASGBossAIController::TryAcquirePlayerTarget
		);
		return;
	}

	Blackboard->SetValueAsObject(
		TargetActorKeyName,
		PlayerPawn
	);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[BossAI] Target acquired. Boss=%s Target=%s"),
		*GetNameSafe(GetPawn()),
		*GetNameSafe(PlayerPawn)
	);
}
