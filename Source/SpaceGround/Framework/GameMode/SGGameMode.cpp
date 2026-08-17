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

#pragma region CheckGameResult
bool ASGGameMode::IsBossDefeated() const
{
	// todo : 보스 체력이 0 이하일 때 true
	return false;
}

bool ASGGameMode::AreAllPlayersDead() const
{
	// todo : 게임스테이트에 생존 플레이어 카운트를 추가 후 해당 변수 0 이하 시 트루 or
	// todo : PlayerState에 생존 여부(예: bIsAlive) 프로퍼티 추가되면 아래 로직으로 교체
	
	// if (ASGGameState* GS = GetGameState<ASGGameState>())
	// {
	//     for (APlayerState* PS : GS->PlayerArray)
	//     {
	//         if (ASGPlayerState* SGPS = Cast<ASGPlayerState>(PS))
	//         {
	//             if (SGPS->bIsAlive) { return false; }
	//         }
	//     }
	//     return true;
	// }
	return false;
}

void ASGGameMode::CheckGameResult()
{
	// 게임오버일 때 return 
	if (bIsGameEnded) return;
	
	if (RoundManagerComponent 
		&& RoundManagerComponent->GetCurrentRoundIndex() >= 5
		&& IsBossDefeated())
	{
		bIsGameEnded = true;
		// todo : 승리 처리(게임스테이트에 반영, 클라 UI 표시)
		UE_LOG(LogTemp, Warning, TEXT("Victory!"));
		return;
	}
	
	if (AreAllPlayersDead())
	{
		bIsGameEnded = true;
		// todo : 패배 처리(게임스테이트에 반영, 클라 UI 표시) + 다시하기,메인화면으로 등 버튼(HUD)
		UE_LOG(LogTemp, Warning, TEXT("Defeat..."));
		return;
	}
}
#pragma endregion