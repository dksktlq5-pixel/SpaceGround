#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"

#include "StructureTypes.h"

#include "StructureDefinition.generated.h"

class AActor;
class UTexture2D;

/**
 * 모든 구조물의 DataTable Row.
 *
 * DT_StructureData에서 구조물별 클래스, 설치 규칙,
 * 자원 비용, 전력 및 전투 데이터를 관리한다.
 */
USTRUCT(BlueprintType)
struct FSGStructureDefinition : public FTableRowBase
{
    GENERATED_BODY()

public:
    // ─────────────────────────────────────────────
    // Identity

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

public:
    // ─────────────────────────────────────────────
    // Class

    /**
     * 실제 설치할 구조물 클래스.
     *
     * StructureBase 또는 그 자식 Blueprint를 지정한다.
     */
    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Class"
    )
    TSubclassOf<AActor> StructureClass;

    /**
     * 건설 모드에서 표시할 홀로그램 프리뷰 클래스.
     */
    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Class"
    )
    TSubclassOf<AActor> PreviewClass;

public:
    // ─────────────────────────────────────────────
    // Visual

    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Visual"
    )
    TObjectPtr<UTexture2D> Icon = nullptr;

public:
    // ─────────────────────────────────────────────
    // Health

    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Health",
        meta = (ClampMin = "1.0")
    )
    float MaxHP = 1000.0f;

public:
    // ─────────────────────────────────────────────
    // Build

    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Build",
        meta = (ClampMin = "0.0")
    )
    float BuildTime = 1.0f;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Build",
        meta = (ClampMin = "1")
    )
    int32 MaxInstallCount = 1;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Build"
    )
    bool bCanBuildDuringCombat = false;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Build"
    )
    TArray<FSGResourceCost> BuildCosts;

public:
    // ─────────────────────────────────────────────
    // Placement

    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Placement",
        meta = (
            ClampMin = "0.0",
            ClampMax = "89.0",
            Units = "Degrees"
        )
    )
    float MaxAllowedSlope = 10.0f;

    /**
     * 설치 중첩 검사에 사용하는 Box Half Extent.
     */
    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Placement",
        meta = (ClampMin = "1.0")
    )
    FVector PlacementExtent =
        FVector(50.0f, 50.0f, 50.0f);

    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Placement"
    )
    float GroundOffset = 0.0f;
    
    /**
 * 구조물의 바닥 전체가 지면에 지지되어야 하는지 여부.
 *
 * true이면 중앙과 네 모서리 지면 검사를 수행한다.
 */
    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Placement"
    )
    bool bRequireFullGroundSupport = true;

    /**
     * 구조물 바닥 모서리에서 아래로 검사하는 깊이.
     *
     * 구조물이 절벽 가장자리나 공중에 걸쳐 있는지 검사한다.
     */
    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Placement",
        meta = (
            ClampMin = "1.0",
            Units = "cm"
        )
    )
    float SupportTraceDepth = 50.0f;

    /**
     * 모서리 지면 검사 시작 높이.
     *
     * 바닥과 정확히 같은 위치에서 Trace를 시작하면
     * 충돌 정밀도 때문에 검사가 실패할 수 있어 조금 위에서 시작한다.
     */
    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Placement",
        meta = (
            ClampMin = "1.0",
            Units = "cm"
        )
    )
    float SupportTraceStartHeight = 20.0f;

    /**
     * 기존 구조물과 확보할 추가 간격.
     *
     * PlacementExtent에 이 값을 더해 중첩 검사를 수행한다.
     */
    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Placement",
        meta = (
            ClampMin = "0.0",
            Units = "cm"
        )
    )
    float StructureSpacing = 10.0f;

public:
    // ─────────────────────────────────────────────
    // Power

    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Power"
    )
    FSGPowerData PowerData;

public:
    // ─────────────────────────────────────────────
    // Combat

    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Combat"
    )
    FSGTurretData TurretData;

public:
    // ─────────────────────────────────────────────
    // Trap

    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Trap"
    )
    FSGTrapData TrapData;
};