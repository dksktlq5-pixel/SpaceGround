#pragma once

#include "CoreMinimal.h"
#include "TurretBase.h"
#include "MachineGunTurret.generated.h"

/*
 * 기본 기관총 터렛
 *
 * TurretBase가 전력, 타깃, 회전, 탄약과 발사 주기를 관리하고
 * 이 클래스는 실제 LineTrace 사격과 피해 적용만 담당한다.
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
	/*
	 * 여러 터렛이 동시에 사격할 때 디버그 라인이 누적되지 않도록
	 * 기본값은 false로 둔다. 필요할 때 BP에서만 활성화한다.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Turret|Debug"
	)
	bool bDrawDebugFire = false;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Turret|Debug",
		meta = (ClampMin = "0.0")
	)
	float DebugLineDuration = 0.15f;
};
