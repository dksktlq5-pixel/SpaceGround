// Fill out your copyright notice in the Description page of Project Settings.


#include "StructureTestManager.h"


// Sets default values
AStructureTestManager::AStructureTestManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AStructureTestManager::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AStructureTestManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

