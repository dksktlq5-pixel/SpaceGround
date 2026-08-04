#pragma once

#include "CoreMinimal.h"
#include "../Structures/Base/StructureTypes.h"
#include "SGResourceTypes.generated.h"

/*
 * 플레이어가 보유한 자원 하나의 수량을 표현한다.
 *
 * FSGResourceCost
 * - 구조물 설치에 필요한 비용
 *
 * FSGResourceAmount
 * - 플레이어가 현재 보유한 자원 수량
 */
USTRUCT(BlueprintType)
struct FSGResourceAmount
{
    GENERATED_BODY()

public:
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Resource"
    )
    ESGResourceType ResourceType = ESGResourceType::Scrap;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Resource",
        meta = (ClampMin = "0")
    )
    int32 Amount = 0;

    FSGResourceAmount() = default;

    FSGResourceAmount(
        const ESGResourceType InResourceType,
        const int32 InAmount
    )
        : ResourceType(InResourceType)
        , Amount(FMath::Max(0, InAmount))
    {
    }
};