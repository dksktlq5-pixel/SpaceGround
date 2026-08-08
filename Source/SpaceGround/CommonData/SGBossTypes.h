#pragma once

#include "CoreMinimal.h"
#include "SGBossTypes.generated.h"

UENUM(BlueprintType)
enum class ESGBossAttackState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Tracking UMETA(DisplayName = "Tracking"),
	DirectionLocked UMETA(DisplayName = "Direction Locked"),
	Hitting UMETA(DisplayName = "Hitting"),
	Recovering UMETA(DisplayName = "Recovering")
};
