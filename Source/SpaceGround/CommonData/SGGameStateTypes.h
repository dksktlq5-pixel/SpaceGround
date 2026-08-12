// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SGGameStateTypes.generated.h"

UENUM(BlueprintType)
enum class EGameStateTypes : uint8
{
	None,
	exploration, // 탐사 10분
	preparation, // 준비 30초
	observation, // 관찰 5초
	combat // 전투
};