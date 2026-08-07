#pragma once

#include "CoreMinimal.h"
#include "SGPlayerTypes.generated.h"

/**
 * 플레이어가 현재 수행 중인 상위 행동 상태.
 *
 * 애니메이션 상태가 아니라 게임플레이 입력 제한용 상태다.
 */
UENUM(BlueprintType)
enum class ESGPlayerActionState : uint8
{
	Normal      UMETA(DisplayName = "Normal"),
	Building    UMETA(DisplayName = "Building"),
	Interacting UMETA(DisplayName = "Interacting"),
	Traversing  UMETA(DisplayName = "Traversing"),
	HardLanding UMETA(DisplayName = "Hard Landing"),
	Disabled    UMETA(DisplayName = "Disabled"),
	Dead        UMETA(DisplayName = "Dead")
};