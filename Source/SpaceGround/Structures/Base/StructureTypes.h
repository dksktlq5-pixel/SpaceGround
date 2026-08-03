#pragma once

#include "CoreMinimal.h"
#include "StructureTypes.generated.h"

/*
 * 구조물 분류
 */
UENUM(BlueprintType)
enum class ESGStructureCategory : uint8
{
	None		UMETA(DisplayName = "None"),

	Power		UMETA(DisplayName = "Power"),
	Attack		UMETA(DisplayName = "Attack"),
	Defense		UMETA(DisplayName = "Defense"),
	Support		UMETA(DisplayName = "Support"),
	Trap		UMETA(DisplayName = "Trap")
};


/*
 * 구조물 작동 상태
 */
UENUM(BlueprintType)
enum class ESGStructureState : uint8
{
	Inactive	UMETA(DisplayName = "Inactive"),
	Active		UMETA(DisplayName = "Active"),
	Unpowered	UMETA(DisplayName = "Unpowered"),
	Damaged		UMETA(DisplayName = "Damaged"),
	Destroyed	UMETA(DisplayName = "Destroyed")
};


/*
 * 구조물 제작에 사용되는 자원
 */
UENUM(BlueprintType)
enum class ESGResourceType : uint8
{
	Scrap				UMETA(DisplayName = "Scrap"),
	StructurePart		UMETA(DisplayName = "Structure Part"),
	Battery				UMETA(DisplayName = "Battery"),
	Circuit				UMETA(DisplayName = "Circuit"),
	ElementMaterial		UMETA(DisplayName = "Element Material"),
	Explosive			UMETA(DisplayName = "Explosive"),
	RepairGel			UMETA(DisplayName = "Repair Gel")
};


/*
 * 공격 속성
 */
UENUM(BlueprintType)
enum class ESGElementType : uint8
{
	None		UMETA(DisplayName = "None"),
	Fire		UMETA(DisplayName = "Fire"),
	Ice			UMETA(DisplayName = "Ice"),
	Electric	UMETA(DisplayName = "Electric"),
	Acid		UMETA(DisplayName = "Acid"),
	Explosive	UMETA(DisplayName = "Explosive")
};


/*
 * 함정 제어 효과
 */
UENUM(BlueprintType)
enum class ESGCrowdControlType : uint8
{
	None	UMETA(DisplayName = "None"),
	Slow	UMETA(DisplayName = "Slow"),
	Stun	UMETA(DisplayName = "Stun")
};


/*
 * 구조물 설치에 필요한 자원 하나를 표현한다.
 *
 * 예시:
 * Scrap 10
 * StructurePart 2
 */
USTRUCT(BlueprintType)
struct FSGResourceCost
{
	GENERATED_BODY()

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Resource"
	)
	ESGResourceType ResourceType = ESGResourceType::Scrap;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Resource",
		meta = (ClampMin = "0")
	)
	int32 Amount = 0;
};


/*
 * 구조물 전력 데이터
 */
USTRUCT(BlueprintType)
struct FSGPowerData
{
	GENERATED_BODY()

	/*
	 * 발전기 전력이 필요한 구조물인지 여부
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Power"
	)
	bool bRequiresPower = false;

	/*
	 * 구조물이 소비하는 전력
	 *
	 * 기관총 터렛 예시: 25 PU
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Power",
		meta = (ClampMin = "0.0")
	)
	float PowerConsumption = 0.0f;

	/*
	 * 발전기가 공급하는 전력
	 *
	 * PowerCore가 아닌 구조물은 0
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Power",
		meta = (ClampMin = "0.0")
	)
	float PowerSupply = 0.0f;

	/*
	 * 발전기의 전력 공급 반경
	 *
	 * 언리얼 기준 1m = 100cm
	 * 18m를 사용하려면 1800
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Power",
		meta = (ClampMin = "0.0")
	)
	float PowerRadius = 0.0f;

	/*
	 * 전력 부족 시 정지 우선순위
	 *
	 * 숫자가 낮을수록 먼저 정지
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Power",
		meta = (ClampMin = "0")
	)
	int32 ShutdownPriority = 1;
};


/*
 * 터렛 전투 데이터
 */
USTRUCT(BlueprintType)
struct FSGTurretData
{
	GENERATED_BODY()

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Turret",
		meta = (ClampMin = "0.0")
	)
	float Damage = 0.0f;

	/*
	 * 한 발을 발사한 뒤 다음 발까지의 시간
	 *
	 * 초당 5발이면 0.2초
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Turret",
		meta = (ClampMin = "0.01")
	)
	float FireInterval = 0.2f;

	/*
	 * 공격 가능 거리
	 *
	 * 16m = 1600cm
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Turret",
		meta = (ClampMin = "0.0")
	)
	float AttackRange = 0.0f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Turret",
		meta = (ClampMin = "0.0", ClampMax = "360.0")
	)
	float ViewAngle = 150.0f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Turret",
		meta = (ClampMin = "0")
	)
	int32 MaxAmmo = 0;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Turret"
	)
	ESGElementType ElementType = ESGElementType::None;
};


/*
 * 함정 데이터
 */
USTRUCT(BlueprintType)
struct FSGTrapData
{
	GENERATED_BODY()

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Trap",
		meta = (ClampMin = "0.0")
	)
	float Damage = 0.0f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Trap"
	)
	ESGCrowdControlType CrowdControlType =
		ESGCrowdControlType::None;

	/*
	 * 감속 또는 기절 지속시간
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Trap",
		meta = (ClampMin = "0.0")
	)
	float EffectDuration = 0.0f;

	/*
	 * 감속 비율
	 *
	 * 35% 감속이면 0.35
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Trap",
		meta = (ClampMin = "0.0", ClampMax = "1.0")
	)
	float SlowRate = 0.0f;

	/*
	 * 최대 발동 횟수
	 *
	 * 일회용 함정이면 1
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Trap",
		meta = (ClampMin = "1")
	)
	int32 MaxTriggerCount = 1;
};