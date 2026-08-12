#pragma once

#include "CoreMinimal.h"
#include "StructureTypes.generated.h"


/**
 * 구조물 분류.
 *
 * 구조물의 역할, 설치 제한, 최대 설치 개수 등을
 * 구분할 때 사용한다.
 */
UENUM(BlueprintType)
enum class ESGStructureCategory : uint8
{
	None UMETA(DisplayName = "None"),

	Power UMETA(DisplayName = "Power"),

	Attack UMETA(DisplayName = "Attack"),

	Defense UMETA(DisplayName = "Defense"),

	Support UMETA(DisplayName = "Support"),

	Trap UMETA(DisplayName = "Trap")
};


/**
 * 구조물의 현재 작동 상태.
 */
UENUM(BlueprintType)
enum class ESGStructureState : uint8
{
	Inactive UMETA(DisplayName = "Inactive"),

	Active UMETA(DisplayName = "Active"),

	Unpowered UMETA(DisplayName = "Unpowered"),

	Damaged UMETA(DisplayName = "Damaged"),

	Destroyed UMETA(DisplayName = "Destroyed")
};


/**
 * 구조물 제작 및 자원 인벤토리에 사용하는 자원 종류.
 */
UENUM(BlueprintType)
enum class ESGResourceType : uint8
{
	Scrap UMETA(DisplayName = "Scrap"),

	StructurePart UMETA(DisplayName = "Structure Part"),

	Battery UMETA(DisplayName = "Battery"),

	Circuit UMETA(DisplayName = "Circuit"),

	ElementMaterial UMETA(DisplayName = "Element Material"),

	Explosive UMETA(DisplayName = "Explosive"),

	RepairGel UMETA(DisplayName = "Repair Gel")
};


/**
 * 구조물 설치에 필요한 자원 하나를 표현한다.
 *
 * 예:
 * Scrap 10
 * StructurePart 2
 */
USTRUCT(BlueprintType)
struct FSGResourceCost
{
	GENERATED_BODY()

public:
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Resource"
	)
	ESGResourceType ResourceType =
		ESGResourceType::Scrap;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Resource",
		meta = (ClampMin = "0")
	)
	int32 Amount = 0;
};


/**
 * 공격 속성.
 *
 * 일반 공격 또는 속성 터렛에서 사용한다.
 */
UENUM(BlueprintType)
enum class ESGElementType : uint8
{
	None UMETA(DisplayName = "None"),

	Fire UMETA(DisplayName = "Fire"),

	Ice UMETA(DisplayName = "Ice"),

	Electric UMETA(DisplayName = "Electric"),

	Acid UMETA(DisplayName = "Acid"),

	Explosive UMETA(DisplayName = "Explosive")
};


/**
 * 함정이 적용하는 군중 제어 효과.
 *
 * 현재 기획에서는 둔화와 기절을
 * 함정 전용 효과로 사용한다.
 */
UENUM(BlueprintType)
enum class ESGCrowdControlType : uint8
{
	None UMETA(DisplayName = "None"),

	Slow UMETA(DisplayName = "Slow"),

	Stun UMETA(DisplayName = "Stun")
};


/**
 * 구조물 전력 데이터.
 *
 * 전력 코어의 공급량과 반경,
 * 전력형 구조물의 소비량을 하나의 구조체로 관리한다.
 */
USTRUCT(BlueprintType)
struct FSGPowerData
{
	GENERATED_BODY()

public:
	/**
	 * 구조물이 작동할 때
	 * 발전기 전력이 필요한지 여부.
	 *
	 * 예:
	 * PowerCore          false
	 * MachineGunTurret   true
	 * Barricade          false
	 * ShockMine          false
	 * SlowPad            false
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Power"
	)
	bool bRequiresPower = false;

	/**
	 * 구조물을 설치할 때
	 * 파워코어가 반드시 필요한지 여부.
	 *
	 * true인 구조물은 설치 예정 위치가
	 * 같은 소유자의 파워코어 공급 범위 안에 있고,
	 * 필요한 잔여 전력이 있어야 설치할 수 있다.
	 *
	 * 예:
	 * PowerCore          false
	 * MachineGunTurret   true
	 * Barricade          false
	 * ShockMine          false
	 * SlowPad            false
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Power"
	)
	bool bRequiresPowerCore = false;

	/**
	 * 구조물이 작동할 때 소비하는 전력.
	 *
	 * 기관총 터렛 초기값 예:
	 * 25 PU
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Power",
		meta = (ClampMin = "0.0")
	)
	float PowerConsumption = 0.0f;

	/**
	 * 발전 구조물이 공급하는 최대 전력.
	 *
	 * PowerCore 초기값 예:
	 * 100 PU
	 *
	 * 발전 구조물이 아닌 경우 0이다.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Power",
		meta = (ClampMin = "0.0")
	)
	float PowerSupply = 0.0f;

	/**
	 * 발전기의 전력 공급 반경.
	 *
	 * Unreal Engine 단위는 cm다.
	 *
	 * 18m를 사용하려면 1800으로 설정한다.
	 * 발전 구조물이 아닌 경우 0이다.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Power",
		meta = (
			ClampMin = "0.0",
			Units = "cm"
		)
	)
	float PowerRadius = 0.0f;

	/**
	 * 전력 부족 상태에서 구조물이 정지하는 우선순위.
	 *
	 * 숫자가 낮은 구조물이 먼저 정지하고,
	 * 숫자가 높은 구조물이 우선적으로 전력을 공급받는다.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Power",
		meta = (ClampMin = "0")
	)
	int32 ShutdownPriority = 1;
};


/**
 * 터렛 전투 데이터.
 *
 * 공격형 구조물에서 사용하는 공통 전투 수치다.
 * 터렛이 아닌 구조물은 기본값으로 유지한다.
 */
USTRUCT(BlueprintType)
struct FSGTurretData
{
	GENERATED_BODY()

public:
	/**
	 * 한 번 공격할 때 적용하는 기본 피해량.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Turret",
		meta = (ClampMin = "0.0")
	)
	float Damage = 0.0f;

	/**
	 * 한 번 발사한 뒤 다음 발사까지 걸리는 시간.
	 *
	 * 초당 5발:
	 * 0.2초
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Turret",
		meta = (
			ClampMin = "0.01",
			Units = "s"
		)
	)
	float FireInterval = 0.2f;

	/**
	 * 터렛의 최대 공격 거리.
	 *
	 * Unreal Engine 단위는 cm다.
	 *
	 * 16m:
	 * 1600cm
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Turret",
		meta = (
			ClampMin = "0.0",
			Units = "cm"
		)
	)
	float AttackRange = 0.0f;

	/**
	 * 터렛이 표적을 감지할 수 있는 수평 시야각.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Turret",
		meta = (
			ClampMin = "0.0",
			ClampMax = "360.0",
			Units = "Degrees"
		)
	)
	float ViewAngle = 150.0f;

	/**
	 * 터렛의 최대 탄약 수.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Turret",
		meta = (ClampMin = "0")
	)
	int32 MaxAmmo = 0;

	/**
	 * 터렛 공격에 적용되는 속성.
	 *
	 * 기관총 터렛은 None을 사용한다.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Turret"
	)
	ESGElementType ElementType =
		ESGElementType::None;
};


/**
 * 함정 데이터.
 *
 * 함정의 직접 피해, 둔화 또는 기절 효과,
 * 최대 작동 횟수를 관리한다.
 */
USTRUCT(BlueprintType)
struct FSGTrapData
{
	GENERATED_BODY()

public:
	/**
	 * 함정이 발동할 때 적용하는 직접 피해량.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Trap",
		meta = (ClampMin = "0.0")
	)
	float Damage = 0.0f;

	/**
	 * 함정이 적용하는 제어 효과 종류.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Trap"
	)
	ESGCrowdControlType CrowdControlType =
		ESGCrowdControlType::None;

	/**
	 * 둔화 또는 기절 효과의 지속시간.
	 *
	 * 예:
	 * 둔화 패드 3초
	 * 충격 지뢰 2.5초
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Trap",
		meta = (
			ClampMin = "0.0",
			Units = "s"
		)
	)
	float EffectDuration = 0.0f;

	/**
	 * 이동 및 행동 속도 감소 비율.
	 *
	 * 35% 감소:
	 * 0.35
	 *
	 * 기절 함정에서는 0으로 둔다.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Trap",
		meta = (
			ClampMin = "0.0",
			ClampMax = "1.0"
		)
	)
	float SlowRate = 0.0f;

	/**
	 * 함정이 작동할 수 있는 최대 횟수.
	 *
	 * 일회용 함정:
	 * 1
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Trap",
		meta = (ClampMin = "1")
	)
	int32 MaxTriggerCount = 1;
};