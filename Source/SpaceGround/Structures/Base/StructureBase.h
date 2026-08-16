#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "StructureDefinition.h"
#include "StructureTypes.h"

#include "StructureBase.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UDataTable;


/*
 * 모든 구조물의 공통 부모 클래스
 *
 * 담당 기능
 * - DataTable 초기화
 * - HP 관리
 * - 피해 및 수리
 * - 전력 상태
 * - 활성 상태
 * - 파괴 처리
 */
UCLASS()
class SPACEGROUND_API AStructureBase : public AActor
{
	GENERATED_BODY()

public:
	AStructureBase();

protected:
	virtual void BeginPlay() override;

public:
	/*
	 * 언리얼 기본 피해 시스템 진입점
	 *
	 * UGameplayStatics::ApplyDamage() 또는
	 * Actor->TakeDamage() 호출 시 실행된다.
	 */
	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser
	) override;


	// ─────────────────────────────────────────────
	// 초기화

	/*
	 * 지정된 DataTable Row를 읽어 구조물을 초기화한다.
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Structure|Initialization"
	)
	bool InitializeStructure();

	/*
	 * DataTable에서 현재 Row 데이터를 찾는다.
	 */
	const FSGStructureDefinition* FindStructureDefinition() const;


	// ─────────────────────────────────────────────
	// 체력

	/*
	 * 구조물에 직접 피해를 적용한다.
	 *
	 * TakeDamage()도 내부적으로 이 함수를 호출한다.
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Structure|Health"
	)
	virtual float ApplyStructureDamage(
		float DamageAmount,
		AActor* DamageCauser
	);

	/*
	 * 구조물 체력을 회복한다.
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Structure|Health"
	)
	virtual float RepairStructure(float RepairAmount);

	/*
	 * 구조물이 파괴되었는지 확인한다.
	 */
	UFUNCTION(
		BlueprintPure,
		Category = "Structure|Health"
	)
	bool IsDestroyed() const;

	/*
	 * 현재 HP 비율을 0~1로 반환한다.
	 */
	UFUNCTION(
		BlueprintPure,
		Category = "Structure|Health"
	)
	float GetHealthPercent() const;


	// ─────────────────────────────────────────────
	// 작동 상태

	/*
	 * 구조물 자체 활성 상태를 변경한다.
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Structure|State"
	)
	virtual void SetStructureActive(bool bNewActive);

	/*
	 * 구조물의 전력 공급 상태를 변경한다.
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Structure|Power"
	)
	virtual void SetPowered(bool bNewPowered);

	/*
	 * 현재 구조물이 실제로 작동할 수 있는 상태인지 확인한다.
	 */
	UFUNCTION(
		BlueprintPure,
		Category = "Structure|State"
	)
	bool CanOperate() const;

	UFUNCTION(
		BlueprintPure,
		Category = "Structure|State"
	)
	bool IsStructureActive() const
	{
		return bIsStructureActive;
	}

	UFUNCTION(
		BlueprintPure,
		Category = "Structure|Power"
	)
	bool IsPowered() const
	{
		return bIsPowered;
	}


	// ─────────────────────────────────────────────
	// Getter

	UFUNCTION(
		BlueprintPure,
		Category = "Structure|Data"
	)
	FName GetStructureID() const
	{
		return StructureID;
	}

	UFUNCTION(
		BlueprintPure,
		Category = "Structure|Data"
	)
	ESGStructureCategory GetStructureCategory() const
	{
		return StructureCategory;
	}

	UFUNCTION(
		BlueprintPure,
		Category = "Structure|Health"
	)
	float GetCurrentHP() const
	{
		return CurrentHP;
	}

	UFUNCTION(
		BlueprintPure,
		Category = "Structure|Health"
	)
	float GetMaxHP() const
	{
		return MaxHP;
	}

	UFUNCTION(
		BlueprintPure,
		Category = "Structure|State"
	)
	ESGStructureState GetStructureState() const
	{
		return StructureState;
	}

	/** 전력망 재계산에서 DataTable을 반복 조회하지 않기 위한 캐시 Getter. */
	float GetCachedPowerConsumption() const
	{
		return CachedPowerConsumption;
	}

	int32 GetCachedShutdownPriority() const
	{
		return CachedShutdownPriority;
	}

	bool RequiresCachedPower() const
	{
		return bCachedRequiresPower;
	}


protected:
	// ─────────────────────────────────────────────
	// 내부 처리

	/*
	 * 현재 HP와 전력 상태를 기준으로 상태를 갱신한다.
	 */
	virtual void UpdateStructureState();

	/*
	 * HP가 0이 되었을 때 한 번 호출된다.
	 */
	virtual void HandleStructureDestroyed(AActor* DamageCauser);

	/*
	 * 구조물 작동 상태가 변경되었을 때 호출된다.
	 *
	 * 터렛 등 자식 클래스에서 오버라이드한다.
	 */
	virtual void OnOperatingStateChanged(bool bCanOperate);

	/*
	 * BP에서 구조물 피격 연출을 추가할 때 사용한다.
	 */
	UFUNCTION(
		BlueprintImplementableEvent,
		Category = "Structure|Events",
		DisplayName = "On Structure Damaged"
	)
	void BP_OnStructureDamaged(
		float DamageAmount,
		float NewCurrentHP
	);

	/*
	 * BP에서 구조물 수리 연출을 추가할 때 사용한다.
	 */
	UFUNCTION(
		BlueprintImplementableEvent,
		Category = "Structure|Events",
		DisplayName = "On Structure Repaired"
	)
	void BP_OnStructureRepaired(
		float RepairAmount,
		float NewCurrentHP
	);

	/*
	 * BP에서 파괴 VFX, SFX 등을 추가할 때 사용한다.
	 */
	UFUNCTION(
		BlueprintImplementableEvent,
		Category = "Structure|Events",
		DisplayName = "On Structure Destroyed"
	)
	void BP_OnStructureDestroyed(AActor* DamageCauser);

	/*
	 * BP에서 전력 상태 변경 연출을 추가할 때 사용한다.
	 */
	UFUNCTION(
		BlueprintImplementableEvent,
		Category = "Structure|Events",
		DisplayName = "On Power State Changed"
	)
	void BP_OnPowerStateChanged(bool bNewPowered);


protected:
	// ─────────────────────────────────────────────
	// Components

	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Structure|Components"
	)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Structure|Components"
	)
	TObjectPtr<UStaticMeshComponent> StructureMesh;


	// ─────────────────────────────────────────────
	// DataTable

	/*
	 * 구조물 데이터가 들어 있는 DataTable
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Structure|Data"
	)
	TObjectPtr<UDataTable> StructureDataTable;

	/*
	 * DataTable에서 사용할 Row 이름
	 *
	 * 예시:
	 * PowerCore
	 * Barricade
	 * MachineGunTurret
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Structure|Data"
	)
	FName StructureRowName = NAME_None;


	// ─────────────────────────────────────────────
	// Runtime Data

	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Structure|Data"
	)
	FName StructureID = NAME_None;

	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Structure|Data"
	)
	ESGStructureCategory StructureCategory =
		ESGStructureCategory::None;

	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Structure|Health"
	)
	float MaxHP = 1000.0f;

	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Structure|Health"
	)
	float CurrentHP = 1000.0f;

	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Structure|State"
	)
	ESGStructureState StructureState =
		ESGStructureState::Inactive;

	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Structure|State"
	)
	bool bIsStructureActive = true;

	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Structure|Power"
	)
	bool bIsPowered = true;

	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Structure|Initialization"
	)
	bool bIsInitialized = false;

	/** InitializeStructure에서 한 번만 읽어 전력망 계산에 재사용한다. */
	float CachedPowerConsumption = 0.0f;

	int32 CachedShutdownPriority = 0;

	bool bCachedRequiresPower = false;

private:
	/*
	 * 파괴 처리가 여러 번 실행되는 것을 방지한다.
	 */
	bool bDestroyHandled = false;
};
