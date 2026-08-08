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

private:
	void TryAcquirePlayerTarget();

	static const FName TargetActorKeyName;

	FTimerHandle TargetAcquisitionTimerHandle;
};
