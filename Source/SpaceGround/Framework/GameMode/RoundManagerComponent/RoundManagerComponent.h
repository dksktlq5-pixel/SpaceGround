// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpaceGround/CommonData/SGGamePhase.h"
#include "RoundManagerComponent.generated.h"
/*
 * 라운드 별 차이 반영하는 클래스
 */

DECLARE_MULTICAST_DELEGATE_OneParam(FOnRoundChanged, int32 NewRoundindex);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnGamePhaseChanged, EGamePhase NewGamePhase); // 델리게이트는 매크로가 만들어준 "클래스"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SPACEGROUND_API URoundManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URoundManagerComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	
	void StartRound();
	void EndRound();
	
	FOnRoundChanged OnRoundChanged;
	
	int32 CurrentRoundIndex = 0;
	int32 GetCurrentRoundIndex() const {  return CurrentRoundIndex; };
	
#pragma region GamePhase Timer

private:
	FTimerHandle GamePhaseTimerHandle;
	
	void BeginExploration();
	void BeginPreparation();
	void BeginObservation();
	void BeginCombat();

public:
	void EndCombat();
	
	FOnGamePhaseChanged OnGamePhaseChanged;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float ExplorationTime = 600.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float PreparationTime = 30.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float ObservationTime = 5.f;

#pragma endregion
	
};
