// Fill out your copyright notice in the Description page of Project Settings.


#include "SGGameMode.h"

#include "SpaceGround/Framework/GameInstance/SGGameInstance.h"
#include "SpaceGround/Framework/GameState/SGGameState.h"

ASGGameMode::ASGGameMode()
{
	RoundManagerComponent = CreateDefaultSubobject<URoundManagerComponent>(TEXT("RoundManagerComponent"));
	
	if (RoundManagerComponent)
	{
		RoundManagerComponent->OnRoundChanged.AddUObject(this,
			&ASGGameMode::HandleRoundChanged);
		// 델리게이트 객체에 접근.이벤트 발생 시 콜백 함수 바인딩(이 객체에서 실행, 실행할 함수 주소) -> 구독!!!!
		
		RoundManagerComponent->OnGamePhaseChanged.AddUObject(this,
			&ASGGameMode::HandleGamePhaseChanged);
	}	
}

void ASGGameMode::BeginPlay()
{
	Super::BeginPlay();
}

void ASGGameMode::HandleRoundChanged(int32 NewRoundIndex) // 라운드 델리게이트의 파라미터와 동일한 int32 타입
{
	
	if (ASGGameState* GS = GetGameState<ASGGameState>())
	{
		GS->CurrentRoundIndex_GameState = NewRoundIndex;
		
		if (USGGameInstance* GI = GetGameInstance<USGGameInstance>())
		{
			GI->Add_RoundDamageAmount(GS->Round_DamageAmounts);
		}
		
		GS->Set_Round_DamageAmounts_Zero();
	}
}

void ASGGameMode::HandleGamePhaseChanged(EGamePhase NewGamePhase)
{
	if (ASGGameState* GS = GetGameState<ASGGameState>())
	{
		GS->GamePhase = NewGamePhase;
	}
}

void ASGGameMode::CheckGameResult()
{
	// 게임오버일 때 return 
}
