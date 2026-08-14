#pragma once

#include "CoreMinimal.h"
#include "AIController.h"

#include "SGBossAIController.generated.h"


UCLASS(Blueprintable)
class SPACEGROUND_API ASGBossAIController : public AAIController
{
	GENERATED_BODY()

public:
	ASGBossAIController();

protected:
	// 보스를 조종하기 시작할 때 Behavior Tree 실행
	virtual void OnPossess(APawn* InPawn) override;
	// 보스와 연결이 끊기면 플레이어 탐색 타이머 정리
	virtual void OnUnPossess() override;
	// 게임 종료나 Actor 제거 시 타이머 정리
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// 싱글플레이 0번 플레이어를 찾아 Blackboard에 저장
	void TryAcquirePlayerTarget();
	// 플레이어 탐색 타이머 중지
	void StopTargetAcquisition();

	// Blackboard에서 사용할 플레이어 Key 이름
	static const FName TargetActorKeyName;

	FTimerHandle TargetAcquisitionTimerHandle;
	int32 TargetAcquisitionAttempts = 0;

	// 플레이어가 늦게 생성되는 상황을 고려해 0.1초마다 최대 50회 시도
	static constexpr float TargetAcquisitionInterval = 0.1f;
	static constexpr int32 MaxTargetAcquisitionAttempts = 50;
};
