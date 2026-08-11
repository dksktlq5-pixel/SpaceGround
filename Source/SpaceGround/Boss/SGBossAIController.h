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
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void TryAcquirePlayerTarget();
	void StopTargetAcquisition();

	static const FName TargetActorKeyName;

	FTimerHandle TargetAcquisitionTimerHandle;
	int32 TargetAcquisitionAttempts = 0;

	static constexpr float TargetAcquisitionInterval = 0.1f;
	static constexpr int32 MaxTargetAcquisitionAttempts = 50;
};
