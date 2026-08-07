
#pragma once

#include "CoreMinimal.h"
#include "SGDamageTypes.generated.h"

UENUM(BlueprintType)
enum class EDamageElement : uint8 // 인덱스 타입. unsigned integer(0~255) 8비트 (int로 쓰면 불필요하게 4바이트 사용)
{
	normal,
	fire,
	ice,
	electric,
	acid,
	explosion
};