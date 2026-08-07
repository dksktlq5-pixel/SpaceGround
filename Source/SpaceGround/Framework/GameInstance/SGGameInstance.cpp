// Fill out your copyright notice in the Description page of Project Settings.

#include "SGGameInstance.h"

void USGGameInstance::Init()
{
	Super::Init();
	
	Initialize_TotalDamageAmounts();
}

void USGGameInstance::Shutdown()
{
	Super::Shutdown();
}

void USGGameInstance::Initialize_TotalDamageAmounts()
{
	for (auto& Pair : TotalDamageAmounts)
	{
		TotalDamageAmounts.FindOrAdd(Pair.Key) = 0.f;
	}
}

// todo : 라운드 종료 시 호출
void USGGameInstance::Add_RoundDamageAmount(const TMap<EDamageElement, float>& RoundDamageAmount)
{
	for (const auto& Pair : RoundDamageAmount)
	{
		TotalDamageAmounts.FindOrAdd(Pair.Key) += Pair.Value;
	}
}

