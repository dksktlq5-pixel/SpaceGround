#pragma once

#include "CoreMinimal.h"
#include "SGConstructionTypes.generated.h"

/**
 * 건설 컴포넌트의 현재 상태.
 */
UENUM(BlueprintType)
enum class ESGBuildModeState : uint8
{
	Inactive   UMETA(DisplayName = "Inactive"),
	Previewing UMETA(DisplayName = "Previewing"),
	Placing    UMETA(DisplayName = "Placing")
};

/**
 * 구조물 설치 실패 사유.
 */
UENUM(BlueprintType)
enum class ESGPlacementFailureReason : uint8
{
	None                 UMETA(DisplayName = "None"),
	NoSurface            UMETA(DisplayName = "No Surface"),
	TooSteep             UMETA(DisplayName = "Too Steep"),
	Overlap               UMETA(DisplayName = "Overlap"),
	OutOfPlacementRange  UMETA(DisplayName = "Out Of Placement Range"),
	OutsidePowerRange    UMETA(DisplayName = "Outside Power Range"),
	InsufficientPower    UMETA(DisplayName = "Insufficient Power"),
	InsufficientResources UMETA(DisplayName = "Insufficient Resources"),
	StructureLimit       UMETA(DisplayName = "Structure Limit"),
	InvalidDefinition    UMETA(DisplayName = "Invalid Definition")
};

/**
 * 한 프레임의 설치 판정 결과.
 */
USTRUCT(BlueprintType)
struct FSGPlacementResult
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Construction")
	bool bCanPlace = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Construction")
	ESGPlacementFailureReason FailureReason =
		ESGPlacementFailureReason::NoSurface;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Construction")
	FTransform PlacementTransform = FTransform::Identity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Construction")
	FHitResult GroundHit;

	void Reset()
	{
		bCanPlace = false;
		FailureReason = ESGPlacementFailureReason::NoSurface;
		PlacementTransform = FTransform::Identity;
		GroundHit = FHitResult();
	}
};