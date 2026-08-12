// Fill out your copyright notice in the Description page of Project Settings.


#include "SGGameState.h"

#include "Net/UnrealNetwork.h"
#include "SpaceGround/Framework/GameInstance/SGGameInstance.h"

ASGGameState::ASGGameState()
{
	
}

void ASGGameState::BeginPlay()
{
	Super::BeginPlay();
	
	Set_Round_DamageAmounts_Zero();
}

void ASGGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ASGGameState, CurrentRoundIndex_GameState);
	DOREPLIFETIME(ASGGameState, GameStateTypes);
}

void ASGGameState::OnRep_CurrentRoundIndex_GameState()
{
	// todo : UI 갱신
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, 5.f, FColor::Yellow,
			FString::Printf(TEXT("Round: %d"), CurrentRoundIndex_GameState));
	}
}

void ASGGameState::OnRep_GameStateTypes()
{
	// todo : 페이즈 별 행동 추가하기
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(2, 5.f, FColor::Cyan,
			FString::Printf(TEXT("Phase: %s"), *UEnum::GetValueAsString(GameStateTypes)));
	}
}

void ASGGameState::Add_Round_DamageAmount(EDamageElement element, float amount)
{
	Round_DamageAmounts.FindOrAdd(element) += amount;
}

void ASGGameState::Set_Round_DamageAmounts_Zero()
{
	for (auto& Pair : Round_DamageAmounts)
	{
		Pair.Value = 0.f;
	}
}

