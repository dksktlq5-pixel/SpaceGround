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
	// 플레이어 탐색은 타이머로 처리하므로 Controller Tick은 필요 없음
	PrimaryActorTick.bCanEverTick = false;
}

// 보스를 조종하기 시작하면 BT 실행 후 플레이어 탐색 시작
void ASGBossAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 다른 Pawn에 잘못 연결된 경우 바로 중단
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

	// BP_SGBoss에 지정한 Behavior Tree 가져옴
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

	// Player Pawn이 아직 생성되지 않았을 수 있어서 타이머로 재시도
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

// Blackboard에 타깃을 넣기 전까지 플레이어 탐색
void ASGBossAIController::TryAcquirePlayerTarget()
{
	APawn* PlayerPawn =
		UGameplayStatics::GetPlayerPawn(this, 0);

	// 최대 5초까지만 재시도하고 실패하면 타이머 중지
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

	// Move To와 공격 Task가 함께 사용할 TargetActor 설정
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

// 등록된 플레이어 탐색 타이머 정리
void ASGBossAIController::StopTargetAcquisition()
{
	GetWorldTimerManager().ClearTimer(TargetAcquisitionTimerHandle);
}
