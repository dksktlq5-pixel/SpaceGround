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

	TargetAcquisitionAttempts = 0;
	GetWorldTimerManager().SetTimer(
		TargetAcquisitionTimerHandle,
		this,
		&ASGBossAIController::TryAcquirePlayerTarget,
		TargetAcquisitionInterval,
		true,
		0.0f
	);
}

void ASGBossAIController::OnUnPossess()
{
	StopTargetAcquisition();
	Super::OnUnPossess();
}

void ASGBossAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopTargetAcquisition();
	Super::EndPlay(EndPlayReason);
}

void ASGBossAIController::TryAcquirePlayerTarget()
{
	APawn* PlayerPawn =
		UGameplayStatics::GetPlayerPawn(this, 0);

	if (!IsValid(PlayerPawn) || !Blackboard)
	{
		++TargetAcquisitionAttempts;
		if (TargetAcquisitionAttempts >= MaxTargetAcquisitionAttempts)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[BossAI] Failed to acquire Player Pawn or Blackboard within %.1f seconds."),
				TargetAcquisitionInterval * MaxTargetAcquisitionAttempts);
			StopTargetAcquisition();
		}
		return;
	}

	if (Blackboard->GetKeyID(TargetActorKeyName) == FBlackboard::InvalidKey)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[BossAI] Blackboard key '%s' does not exist."),
			*TargetActorKeyName.ToString());
		StopTargetAcquisition();
		return;
	}

	Blackboard->SetValueAsObject(
		TargetActorKeyName,
		PlayerPawn
	);
	StopTargetAcquisition();

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[BossAI] Target acquired. Boss=%s Target=%s"),
		*GetNameSafe(GetPawn()),
		*GetNameSafe(PlayerPawn)
	);
}

void ASGBossAIController::StopTargetAcquisition()
{
	GetWorldTimerManager().ClearTimer(TargetAcquisitionTimerHandle);
}
