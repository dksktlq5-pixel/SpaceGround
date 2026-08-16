#pragma once

#include "CoreMinimal.h"
#include "../Base/StructureBase.h"
#include "PowerCore.generated.h"

class AStructureBase;

/**
 * 킬존의 전력을 공급하는 핵심 구조물.
 *
 * 담당 기능:
 * - DataTable에서 공급 전력과 반경 로드
 * - 같은 소유자가 설치한 전력 소비 구조물 탐색
 * - 우선순위에 따른 전력 분배
 * - 여러 파워코어 사이의 중복 연결 방지
 * - 연결 구조물 파괴 시 전력망 재계산
 * - 발전기 파괴 시 연결 구조물 전력 차단 및 재분배
 */
UCLASS()
class SPACEGROUND_API APowerCore : public AStructureBase
{
	GENERATED_BODY()

public:
	APowerCore();

protected:
	virtual void BeginPlay() override;

	/**
	 * 발전기가 파괴되기 전에
	 * 연결 구조물의 전력을 차단한다.
	 */
	virtual void HandleStructureDestroyed(
		AActor* DamageCauser
	) override;


public:
	/**
	 * 같은 소유자의 전체 전력망을 다시 계산한다.
	 *
	 * 같은 플레이어가 여러 파워코어를 설치했더라도
	 * 한 번의 호출로 모든 파워코어의 연결 상태를
	 * 다시 분배한다.
	 */
	UFUNCTION(
		BlueprintCallable,
		CallInEditor,
		Category = "Power Core"
	)
	void RecalculatePowerGrid();

	/**
	 * 현재 파워코어에 연결된 구조물의
	 * 전력을 모두 차단한다.
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Power Core"
	)
	void ShutdownPowerGrid();

	/**
	 * 같은 소유자가 설치한 모든 비코어 구조물을
	 * 전력 소비 여부와 관계없이 비활성화한다.
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Power Core"
	)
	void ShutdownAllOwnedStructures();

	/**
	 * 해당 위치가 현재 파워코어의
	 * 전력 공급 범위 안인지 검사한다.
	 *
	 * 파워코어가 파괴되었거나 작동할 수 없는 상태라면
	 * 범위 안이어도 false를 반환한다.
	 */
	UFUNCTION(
		BlueprintPure,
		Category = "Power Core"
	)
	bool IsInsidePowerRange(
		const FVector& WorldLocation
	) const;

	/**
	 * 현재 발전기의 최대 공급 전력.
	 */
	UFUNCTION(
		BlueprintPure,
		Category = "Power Core"
	)
	float GetPowerSupply() const
	{
		return PowerSupply;
	}

	/**
	 * 현재 사용 중인 전력.
	 */
	UFUNCTION(
		BlueprintPure,
		Category = "Power Core"
	)
	float GetUsedPower() const
	{
		return UsedPower;
	}

	/**
	 * 현재 남아 있는 전력.
	 */
	UFUNCTION(
		BlueprintPure,
		Category = "Power Core"
	)
	float GetRemainingPower() const
	{
		return FMath::Max(
			0.0f,
			PowerSupply - UsedPower
		);
	}

	/**
	 * 전력 공급 반경.
	 *
	 * Unreal Unit 기준으로 반환한다.
	 * 1800 = 18m
	 */
	UFUNCTION(
		BlueprintPure,
		Category = "Power Core"
	)
	float GetPowerRadius() const
	{
		return PowerRadius;
	}

	/**
	 * 현재 연결된 유효 구조물 수.
	 */
	UFUNCTION(
		BlueprintPure,
		Category = "Power Core"
	)
	int32 GetConnectedStructureCount() const;


protected:
	/**
	 * DataTable의 PowerData를
	 * 발전기 런타임 값에 적용한다.
	 */
	bool LoadPowerCoreData();

	/** 같은 프레임의 여러 요청을 다음 Tick 한 번으로 합친다. */
	void RequestPowerGridRecalculation();

	/**
	 * 연결된 구조물이 파괴되었을 때 호출된다.
	 *
	 * 다음 프레임에 같은 소유자의
	 * 전체 전력망을 다시 계산한다.
	 */
	UFUNCTION()
	void HandleConnectedStructureDestroyed(
		AActor* DestroyedActor
	);

	/**
	 * 디버그용 전력 반경 표시.
	 */
	void DrawPowerRadiusDebug() const;


protected:
	/**
	 * 발전기가 공급할 수 있는 최대 전력.
	 *
	 * DataTable PowerSupply에서 초기화된다.
	 */
	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Power Core"
	)
	float PowerSupply = 0.0f;

	/**
	 * 현재 연결된 구조물이 사용 중인 전력.
	 */
	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Power Core"
	)
	float UsedPower = 0.0f;

	/**
	 * 전력 공급 반경.
	 *
	 * DataTable PowerRadius에서 초기화된다.
	 */
	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Power Core"
	)
	float PowerRadius = 0.0f;

	/**
	 * 현재 발전기에서 전력을 공급받는 구조물.
	 */
	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Power Core"
	)
	TArray<TObjectPtr<AStructureBase>> ConnectedStructures;

	/**
	 * 게임 실행 중 전력 범위를 표시할지 여부.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Power Core|Debug"
	)
	bool bDrawDebugPowerRadius = false;

	/**
	 * 디버그 구체가 유지되는 시간.
	 *
	 * 0 이하이면 한 프레임만 표시된다.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Power Core|Debug",
		meta = (ClampMin = "0.0")
	)
	float DebugDrawDuration = 0.0f;

	/** BeginPlay/파괴 이벤트의 중복 전력망 계산을 합치는 Timer. */
	FTimerHandle PowerGridRecalculationTimerHandle;
};
