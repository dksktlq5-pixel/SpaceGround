// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "SpaceGround/CommonData/SGDamageTypes.h"
#include "SGGameState.generated.h"

/**
* GameState — "다같이 보는 전광판"
* 서버가 계산하고, 모든 클라이언트한테 복제되는 공유 데이터 
* "화면에 다 같이 떠야 하는 것"은 다 여기.

* 현재 페이즈(준비/관찰/전투) + 남은 시간 → 다같이 보는 카운트다운 UI
* 보스 HP, 현재 드러난 취약 속성, 이번 판 속성별 누적 데미지(데미지 표시 UI 기획한 부분)
* 접속한 플레이어 목록(팀 전체 상태 파악용)
 */
UCLASS()
class SPACEGROUND_API ASGGameState : public AGameState
{
	GENERATED_BODY()
	
private:
	ASGGameState();
	
	virtual void BeginPlay() override;
	
public:
	int32 CurrentRoundIndex_GameState = 0;

public:
#pragma region Count Round_DamageAmount
	// todo : 보스 피격 시 속성별로 누적 데미지 저장(보스쪽에서 호출해야 함)
	
	UPROPERTY(BlueprintReadOnly)
	TMap<EDamageElement, float> Round_DamageAmounts;
	
	void Add_Round_DamageAmount(EDamageElement element, float amount);
	
	UFUNCTION(BlueprintCallable) // 서버 RPC?
	void Add_Round_DamageStats(); // 게임 인스턴스의 데미지 맵에 이번 라운드 데미지를 누적시키는 함수
	
	void Set_Round_DamageAmounts_Zero();
	
#pragma endregion
};
