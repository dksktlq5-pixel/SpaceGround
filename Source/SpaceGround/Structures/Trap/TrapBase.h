#pragma once

#include "CoreMinimal.h"
#include "../Base/StructureBase.h"
#include "TrapBase.generated.h"

class UBoxComponent;
class UPrimitiveComponent;

/*
 * 모든 함정의 공통 부모 클래스
 *
 * MVP 기능
 * - DataTable 함정 데이터 로드
 * - Box Overlap 감지
 * - Trigger 횟수 관리
 * - 자식 클래스별 효과 실행
 * - 최대 발동 횟수 소진 시 파괴
 */
UCLASS(Abstract)
class SPACEGROUND_API ATrapBase : public AStructureBase
{
	GENERATED_BODY()

public:
	ATrapBase();

protected:
	virtual void BeginPlay() override;

	virtual void HandleStructureDestroyed(
		AActor* DamageCauser
	) override;

	virtual void OnOperatingStateChanged(
		bool bCanOperate
	) override;


public:
	UFUNCTION(
		BlueprintPure,
		Category = "Trap|State"
	)
	bool CanTrigger() const;

	UFUNCTION(
		BlueprintPure,
		Category = "Trap|State"
	)
	int32 GetCurrentTriggerCount() const
	{
		return CurrentTriggerCount;
	}

	UFUNCTION(
		BlueprintPure,
		Category = "Trap|Data"
	)
	float GetTrapDamage() const
	{
		return TrapDamage;
	}

	UFUNCTION(
		BlueprintPure,
		Category = "Trap|Data"
	)
	float GetEffectDuration() const
	{
		return EffectDuration;
	}

	UFUNCTION(
		BlueprintPure,
		Category = "Trap|Data"
	)
	float GetSlowRate() const
	{
		return SlowRate;
	}

	UFUNCTION(
		BlueprintPure,
		Category = "Trap|Components"
	)
	UBoxComponent* GetTriggerBox() const
	{
		return TriggerBox;
	}


protected:
	bool LoadTrapData();

	virtual bool IsValidTrapTarget(
		AActor* OtherActor
	) const;

	virtual void ActivateTrap(
		AActor* TargetActor
	);

	void FinishTrapActivation();


	UFUNCTION()
	void OnTriggerBoxBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);


	UFUNCTION(
		BlueprintImplementableEvent,
		Category = "Trap|Events",
		DisplayName = "On Trap Activated"
	)
	void BP_OnTrapActivated(AActor* TargetActor);


protected:
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Trap|Components"
	)
	TObjectPtr<UBoxComponent> TriggerBox;


	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Trap|Data"
	)
	float TrapDamage = 0.0f;

	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Trap|Data"
	)
	ESGCrowdControlType CrowdControlType =
		ESGCrowdControlType::None;

	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Trap|Data"
	)
	float EffectDuration = 0.0f;

	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Trap|Data"
	)
	float SlowRate = 0.0f;

	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Trap|Data"
	)
	int32 MaxTriggerCount = 1;

	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Trap|State"
	)
	int32 CurrentTriggerCount = 0;


	/*
	 * 이 태그가 있는 Actor만 함정에 반응한다.
	 *
	 * 테스트 대상이나 보스 Actor에
	 * TrapTarget 태그를 추가하면 된다.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Trap|Target"
	)
	FName RequiredTargetTag = TEXT("TrapTarget");

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Trap|Activation",
		meta = (ClampMin = "0.0")
	)
	float DestroyDelayAfterFinalTrigger = 0.15f;


private:
	bool bActivationInProgress = false;
};
