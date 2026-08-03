#pragma once

#include "CoreMinimal.h"
#include "TurretBase.h"
#include "MachineGunTurret.generated.h"

/*
 * 기본 기관총 터렛
 *
 * 실제 LineTrace 사격과 피해 적용을 담당한다.
 */
UCLASS()
class SPACEGROUND_API AMachineGunTurret : public ATurretBase
{
	GENERATED_BODY()

public:
	AMachineGunTurret();

protected:
	virtual void FireAtTarget(
		AActor* Target
	) override;


protected:
	UFUNCTION(
		BlueprintImplementableEvent,
		Category = "Turret|Events",
		DisplayName = "On Machine Gun Fired"
	)
	void BP_OnMachineGunFired(
		const FVector& TraceStart,
		const FVector& TraceEnd,
		bool bHit,
		const FHitResult& HitResult
	);


protected:
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Turret|Debug"
	)
	bool bDrawDebugFire = true;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Turret|Debug",
		meta = (ClampMin = "0.0")
	)
	float DebugLineDuration = 0.15f;
};