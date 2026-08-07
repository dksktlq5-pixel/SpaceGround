// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "RoundManagerComponent/RoundManagerComponent.h"
#include "SGGameMode.generated.h"

/**
* GameMode — "규칙 심판, 서버 혼자만 가짐", "권한 있는 판단"
* 준비(30초) → 관찰(5초) → 전투 페이즈 전환을 결정하는 상태머신 (타이머 자체는 여기서 돌리되, 남은 시간 "표시"는 GameState로 넘겨야 함)

* 보스 스폰, 승리/패배 판정
* 플레이어 접속 시(PostLogin) 스폰 위치 배정 — 멀티라 몇 명이 들어오든 처리해야 함
* 기본 Pawn/PlayerController/GameState/PlayerState 클래스 지정
* 멀티 고려 포인트: 여기서 계산한 결과를 UI에 보여주려면 반드시 GameState 변수에 옮겨 담고 그걸 복제. 
* GameMode 변수를 클라에서 직접 읽으려는 실수 많이 함
 */
UCLASS()
class SPACEGROUND_API ASGGameMode : public AGameMode
{
	GENERATED_BODY()
	
private:
	ASGGameMode();
	
	virtual void BeginPlay() override;
	
	UPROPERTY(VisibleAnywhere)
	URoundManagerComponent* RoundManagerComponent;
	
public:
	void HandleRoundChanged(int32 NewRoundIndex);
	
};
