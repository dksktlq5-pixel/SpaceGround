// Fill out your copyright notice in the Description page of Project Settings.


#include "SGGameState.h"

#include "SpaceGround/Framework/GameInstance/SGGameInstance.h"

ASGGameState::ASGGameState()
{
	
}

void ASGGameState::BeginPlay()
{
	Super::BeginPlay();
	
	Set_Round_DamageAmounts_Zero();
}

void ASGGameState::Add_Round_DamageAmount(EDamageElement element, float amount)
{
	Round_DamageAmounts.FindOrAdd(element) += amount;
}

void ASGGameState::Add_Round_DamageStats()
{
	USGGameInstance* GI = GetGameInstance<USGGameInstance>();
	GI->
}

void ASGGameState::Set_Round_DamageAmounts_Zero()
{
	for (auto& Pair : Round_DamageAmounts)
	{
		Pair.Value = 0.f;
	}
}

