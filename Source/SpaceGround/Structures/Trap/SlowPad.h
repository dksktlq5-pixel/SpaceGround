#pragma once

#include "CoreMinimal.h"
#include "TrapBase.h"
#include "SlowPad.generated.h"

/*
 * 보스에게 직접 피해와 감속 요청을 보내는 일회용 함정
 */
UCLASS()
class SPACEGROUND_API ASlowPad : public ATrapBase
{
	GENERATED_BODY()

public:
	ASlowPad();

protected:
	virtual void ActivateTrap(
		AActor* TargetActor
	) override;


	UFUNCTION(
		BlueprintImplementableEvent,
		Category = "Trap|Slow Pad",
		DisplayName = "On Slow Requested"
	)
	void BP_OnSlowRequested(
		AActor* TargetActor,
		float InSlowRate,
		float InDuration
	);
};