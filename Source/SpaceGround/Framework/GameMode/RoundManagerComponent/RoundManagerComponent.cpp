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
	// todo : 게임 스테이트의 map을 게임 인스턴스의 map에 더하기
	
}

