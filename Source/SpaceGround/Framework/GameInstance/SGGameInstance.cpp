// Fill out your copyright notice in the Description page of Project Settings.

#include "SGGameInstance.h"

void USGGameInstance::Init()
{
	Super::Init();
	
}

void USGGameInstance::Shutdown()
{
	Super::Shutdown();
}

// todo : 라운드 종료 시 호출
void USGGameInstance::Add_RoundDamageAmount(const TMap<EDamageElement, float>& RoundDamageAmount)
{
	for (const auto& Pair : RoundDamageAmount)
	{
		TotalDamageAmounts.FindOrAdd(Pair.Key) += Pair.Value;
	}
}

