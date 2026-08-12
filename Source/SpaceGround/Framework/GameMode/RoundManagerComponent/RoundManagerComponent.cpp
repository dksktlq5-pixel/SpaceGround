// Fill out your copyright notice in the Description page of Project Settings.

#include "RoundManagerComponent.h"

#include "SpaceGround/Framework/GameState/SGGameState.h"

URoundManagerComponent::URoundManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
}

void URoundManagerComponent::BeginPlay()
{
	Super::BeginPlay();
	
	BeginExploration();
}

void URoundManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                           FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void URoundManagerComponent::StartRound()
{
	CurrentRoundIndex++;
	OnRoundChanged.Broadcast(CurrentRoundIndex); // 구독자들에게 알림 
}

void URoundManagerComponent::EndRound()
{
	// todo : 전투 종료 처리 - 잔여 몬스터 정리/재배치 등 (일반 몹 시스템 추가 후 구현)
	// 데미지 맵 병합은 StartRound()의 델리게이트 체인에서 이미 처리됨
}

void URoundManagerComponent::BeginExploration()
{
	StartRound();
	OnGameStateTypesChanged.Broadcast(EGameStateTypes::exploration);

	GetWorld()->GetTimerManager().SetTimer(
		GameStateTypesTimerHandle, this, &URoundManagerComponent::BeginPreparation, ExplorationTime, false);
}

void URoundManagerComponent::BeginPreparation()
{
	OnGameStateTypesChanged.Broadcast(EGameStateTypes::preparation);

	GetWorld()->GetTimerManager().SetTimer(
		GameStateTypesTimerHandle, this, &URoundManagerComponent::BeginObservation, PreparationTime, false);
}

void URoundManagerComponent::BeginObservation()
{
	OnGameStateTypesChanged.Broadcast(EGameStateTypes::observation);

	GetWorld()->GetTimerManager().SetTimer(
		GameStateTypesTimerHandle, this, &URoundManagerComponent::BeginCombat, ObservationTime, false);
}

void URoundManagerComponent::BeginCombat()
{
	OnGameStateTypesChanged.Broadcast(EGameStateTypes::combat);
	// 타이머 없음
}

void URoundManagerComponent::EndCombat()
{
	EndRound();
	BeginExploration();
}
