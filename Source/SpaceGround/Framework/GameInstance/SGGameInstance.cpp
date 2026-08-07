// Fill out your copyright notice in the Description page of Project Settings.

#include "SGGameInstance.h"

void USGGameInstance::Init()
{
	Super::Init();
	
	TotalDamageAmounts.FindOrAdd()
}

void USGGameInstance::Shutdown()
{
	Super::Shutdown();
}

void USGGameInstance::Initialize_TotalDamageAmounts()
{
	for (auto& Pair : TotalDamageAmounts)
	{
		Pair.Value = 0.f;
	}
}

// todo : 라운드 종료 시 호출
void USGGameInstance::Add_DamageAmount(EDamageElement Element, float DamageAmount)
{
	TotalDamageAmounts.Add(Element) += DamageAmount;
}
