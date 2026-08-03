#pragma once

#include "CoreMinimal.h"
#include "TrapBase.h"
#include "ShockMine.generated.h"

/*
 * 보스에게 직접 피해와 기절 요청을 보내는 일회용 함정
 */
UCLASS()
class SPACEGROUND_API AShockMine : public ATrapBase
{
	GENERATED_BODY()

public:
	AShockMine();

protected:
	virtual void ActivateTrap(
		AActor* TargetActor
	) override;


	UFUNCTION(
		BlueprintImplementableEvent,
		Category = "Trap|Shock Mine",
		DisplayName = "On Stun Requested"
	)
	void BP_OnStunRequested(
		AActor* TargetActor,
		float StunDuration
	);
};