#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "StructureTypes.h"
#include "StructureDefinition.generated.h"

class AStructureBase;
class UTexture2D;


/*
 * 모든 구조물의 DataTable Row
 *
 * DT_StructureData에서 구조물별 수치를 관리한다.
 */
USTRUCT(BlueprintType)
struct FSGStructureDefinition : public FTableRowBase
{
	GENERATED_BODY()

	/*
	 * 구조물 내부 식별자
	 *
	 * 예시:
	 * PowerCore
	 * MachineGunTurret
	 * Barricade
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Identity"
	)
	FName StructureID = NAME_None;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Identity"
	)
	FText DisplayName;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Identity",
		meta = (MultiLine = true)
	)
	FText Description;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Identity"
	)
	ESGStructureCategory Category =
		ESGStructureCategory::None;

	/*
	 * DataTable Row를 통해 생성할 구조물 클래스
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Class"
	)
	TSubclassOf<AStructureBase> StructureClass;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Visual"
	)
	TObjectPtr<UTexture2D> Icon = nullptr;

	/*
	 * 구조물 최대 체력
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Health",
		meta = (ClampMin = "1.0")
	)
	float MaxHP = 1000.0f;

	/*
	 * 건설 완료까지 걸리는 시간
	 *
	 * 현재는 실제 건설 모드에서 사용하지 않지만
	 * 나중을 위해 DataTable에 저장한다.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Build",
		meta = (ClampMin = "0.0")
	)
	float BuildTime = 1.0f;

	/*
	 * 동시에 배치할 수 있는 최대 개수
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Build",
		meta = (ClampMin = "1")
	)
	int32 MaxInstallCount = 1;

	/*
	 * 현재 전투 중 설치 가능한지 여부
	 *
	 * 건설 모드 구현 시 사용한다.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Build"
	)
	bool bCanBuildDuringCombat = false;

	/*
	 * 구조물 제작 비용
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Build"
	)
	TArray<FSGResourceCost> BuildCosts;

	/*
	 * 발전기 및 전력 소비 데이터
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Power"
	)
	FSGPowerData PowerData;

	/*
	 * 터렛 구조물에서 사용하는 데이터
	 *
	 * 터렛이 아닌 구조물은 기본값으로 둔다.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Combat"
	)
	FSGTurretData TurretData;

	/*
	 * 함정 구조물에서 사용하는 데이터
	 *
	 * 함정이 아닌 구조물은 기본값으로 둔다.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Trap"
	)
	FSGTrapData TrapData;
};