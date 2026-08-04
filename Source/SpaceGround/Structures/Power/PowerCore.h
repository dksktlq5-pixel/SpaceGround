#pragma once

#include "CoreMinimal.h"
#include "../Base/StructureBase.h"
#include "PowerCore.generated.h"

class AStructureBase;


/*
 * 킬존의 전력을 공급하는 핵심 구조물
 *
 * 담당 기능
 * - DataTable에서 공급 전력과 반경 로드
 * - 반경 내 전력 소비 구조물 탐색
 * - 우선순위에 따른 전력 분배
 * - 발전기 파괴 시 연결 구조물 전력 차단
 */
UCLASS()
class SPACEGROUND_API APowerCore : public AStructureBase
{
	GENERATED_BODY()

public:
	APowerCore();

protected:
	virtual void BeginPlay() override;

	/*
	 * 발전기가 파괴되기 전에
	 * 연결 구조물의 전력을 모두 차단한다.
	 */
	virtual void HandleStructureDestroyed(
		AActor* DamageCauser
	) override;

public:
	/*
	 * 현재 월드의 구조물을 검색하여
	 * 전력 연결 상태를 다시 계산한다.
	 *
	 * Details 패널에서도 호출할 수 있다.
	 */
	UFUNCTION(
		BlueprintCallable,
		CallInEditor,
		Category = "Power Core"
	)
	void RecalculatePowerGrid();

	/*
	 * 연결된 구조물의 전력을 모두 차단한다.
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Power Core"
	)
	void ShutdownPowerGrid();

	/*
	 * 현재 발전기의 최대 공급 전력
	 */
	UFUNCTION(
		BlueprintPure,
		Category = "Power Core"
	)
	float GetPowerSupply() const
	{
		return PowerSupply;
	}

	/*
	 * 현재 사용 중인 전력
	 */
	UFUNCTION(
		BlueprintPure,
		Category = "Power Core"
	)
	float GetUsedPower() const
	{
		return UsedPower;
	}

	/*
	 * 현재 남아 있는 전력
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

	/*
	 * 전력 공급 반경
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

	/*
	 * 현재 연결된 유효 구조물 수
	 */
	UFUNCTION(
		BlueprintPure,
		Category = "Power Core"
	)
	int32 GetConnectedStructureCount() const;

protected:
	/*
	 * DataTable의 PowerData를
	 * 발전기 런타임 값에 적용한다.
	 */
	bool LoadPowerCoreData();

	/*
	 * 디버그용 전력 반경 표시
	 */
	void DrawPowerRadiusDebug() const;


protected:
	/*
	 * 발전기가 공급할 수 있는 최대 전력
	 *
	 * DataTable PowerSupply에서 초기화된다.
	 */
	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Power Core"
	)
	float PowerSupply = 0.0f;

	/*
	 * 현재 연결된 구조물이 사용 중인 전력
	 */
	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Power Core"
	)
	float UsedPower = 0.0f;

	/*
	 * 전력 공급 반경
	 *
	 * DataTable PowerRadius에서 초기화된다.
	 */
	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Power Core"
	)
	float PowerRadius = 0.0f;

	/*
	 * 현재 발전기에서 전력을 공급받는 구조물
	 */
	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Power Core"
	)
	TArray<TObjectPtr<AStructureBase>> ConnectedStructures;

	/*
	 * 게임 실행 중 전력 범위를 표시할지 여부
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Power Core|Debug"
	)
	bool bDrawDebugPowerRadius = true;

	/*
	 * 디버그 구체가 유지되는 시간
	 *
	 * 0 이하이면 한 프레임만 표시된다.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Power Core|Debug",
		meta = (ClampMin = "0.0")
	)
	float DebugDrawDuration = 10.0f;
};