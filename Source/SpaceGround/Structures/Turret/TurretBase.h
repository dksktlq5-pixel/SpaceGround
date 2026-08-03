#pragma once

#include "CoreMinimal.h"
#include "../Base/StructureBase.h"
#include "TurretBase.generated.h"

class USceneComponent;

/*
 * 모든 공격형 터렛의 공통 부모 클래스
 *
 * MVP 기능
 * - DataTable 터렛 데이터 로드
 * - 에디터에서 직접 지정한 타깃 추적
 * - 사거리 / 시야각 / 시야 차폐 검사
 * - Yaw / Pitch 회전
 * - 탄약 관리
 * - 전력 차단 시 사격 중지
 * - HP 40% 이하 연사 속도 감소
 */
UCLASS(Abstract)
class SPACEGROUND_API ATurretBase : public AStructureBase
{
	GENERATED_BODY()

public:
	ATurretBase();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void OnOperatingStateChanged(
		bool bCanOperate
	) override;

	virtual void HandleStructureDestroyed(
		AActor* DamageCauser
	) override;


public:
	// ─────────────────────────────────────────────
	// 타깃

	UFUNCTION(
		BlueprintCallable,
		Category = "Turret|Target"
	)
	void SetTargetActor(AActor* NewTarget);

	UFUNCTION(
		BlueprintCallable,
		Category = "Turret|Target"
	)
	void ClearTargetActor();

	UFUNCTION(
		BlueprintPure,
		Category = "Turret|Target"
	)
	AActor* GetTargetActor() const
	{
		return TargetActor;
	}

	UFUNCTION(
		BlueprintPure,
		Category = "Turret|Target"
	)
	bool HasValidTarget() const;


	// ─────────────────────────────────────────────
	// 전투 상태

	UFUNCTION(
		BlueprintPure,
		Category = "Turret|Combat"
	)
	bool CanFireAtTarget() const;

	UFUNCTION(
		BlueprintPure,
		Category = "Turret|Combat"
	)
	bool IsTargetInRange() const;

	UFUNCTION(
		BlueprintPure,
		Category = "Turret|Combat"
	)
	bool IsTargetInsideViewAngle() const;

	UFUNCTION(
		BlueprintPure,
		Category = "Turret|Combat"
	)
	bool HasLineOfSightToTarget() const;

	UFUNCTION(
		BlueprintPure,
		Category = "Turret|Combat"
	)
	int32 GetCurrentAmmo() const
	{
		return CurrentAmmo;
	}

	UFUNCTION(
		BlueprintPure,
		Category = "Turret|Combat"
	)
	int32 GetMaxAmmo() const
	{
		return MaxAmmo;
	}

	UFUNCTION(
		BlueprintCallable,
		Category = "Turret|Combat"
	)
	void RefillAmmo();

	UFUNCTION(
		BlueprintCallable,
		Category = "Turret|Combat"
	)
	void AddAmmo(int32 Amount);


	// ─────────────────────────────────────────────
	// Getter

	UFUNCTION(
		BlueprintPure,
		Category = "Turret|Data"
	)
	float GetTurretDamage() const
	{
		return TurretDamage;
	}

	UFUNCTION(
		BlueprintPure,
		Category = "Turret|Data"
	)
	float GetAttackRange() const
	{
		return AttackRange;
	}

	UFUNCTION(
		BlueprintPure,
		Category = "Turret|Data"
	)
	float GetViewAngle() const
	{
		return ViewAngle;
	}

	UFUNCTION(
		BlueprintPure,
		Category = "Turret|Components"
	)
	USceneComponent* GetMuzzlePoint() const
	{
		return MuzzlePoint;
	}


protected:
	// ─────────────────────────────────────────────
	// 내부 로직

	bool LoadTurretData();

	void UpdateTurretRotation(float DeltaSeconds);

	void StartFireTimer();

	void StopFireTimer();

	void RefreshFireTimer();

	void TryFire();

	/*
	 * 실제 발사 방식은 자식 클래스에서 구현한다.
	 */
	virtual void FireAtTarget(AActor* Target);

	float GetCurrentFireInterval() const;

	FVector GetTargetLocation() const;


	// ─────────────────────────────────────────────
	// BP 이벤트

	UFUNCTION(
		BlueprintImplementableEvent,
		Category = "Turret|Events",
		DisplayName = "On Target Changed"
	)
	void BP_OnTargetChanged(AActor* NewTarget);

	UFUNCTION(
		BlueprintImplementableEvent,
		Category = "Turret|Events",
		DisplayName = "On Ammo Changed"
	)
	void BP_OnAmmoChanged(
		int32 NewCurrentAmmo,
		int32 NewMaxAmmo
	);


protected:
	// ─────────────────────────────────────────────
	// Components

	/*
	 * 좌우 회전을 담당한다.
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Turret|Components"
	)
	TObjectPtr<USceneComponent> YawPivot;

	/*
	 * 상하 회전을 담당한다.
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Turret|Components"
	)
	TObjectPtr<USceneComponent> PitchPivot;

	/*
	 * 발사 위치와 방향
	 */
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Turret|Components"
	)
	TObjectPtr<USceneComponent> MuzzlePoint;


	// ─────────────────────────────────────────────
	// Target

	/*
	 * MVP에서는 자동 탐색 대신
	 * 레벨 또는 BP에서 직접 지정한다.
	 */
	UPROPERTY(
		EditInstanceOnly,
		BlueprintReadOnly,
		Category = "Turret|Target"
	)
	TObjectPtr<AActor> TargetActor;


	// ─────────────────────────────────────────────
	// Runtime Combat Data

	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Turret|Combat"
	)
	float TurretDamage = 0.0f;

	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Turret|Combat"
	)
	float BaseFireInterval = 0.2f;

	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Turret|Combat"
	)
	float AttackRange = 1600.0f;

	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Turret|Combat"
	)
	float ViewAngle = 150.0f;

	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Turret|Combat"
	)
	int32 MaxAmmo = 0;

	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Turret|Combat"
	)
	int32 CurrentAmmo = 0;

	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Turret|Combat"
	)
	ESGElementType ElementType =
		ESGElementType::None;


	// ─────────────────────────────────────────────
	// Rotation

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Turret|Rotation",
		meta = (ClampMin = "0.0")
	)
	float YawRotationSpeed = 120.0f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Turret|Rotation",
		meta = (ClampMin = "0.0")
	)
	float PitchRotationSpeed = 90.0f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Turret|Rotation"
	)
	float MinimumPitch = -25.0f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Turret|Rotation"
	)
	float MaximumPitch = 45.0f;

	/*
	 * 이 각도 안으로 조준되어야 발사한다.
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Turret|Rotation",
		meta = (ClampMin = "0.0")
	)
	float FireAlignmentTolerance = 8.0f;


	// ─────────────────────────────────────────────
	// Damaged Penalty

	/*
	 * HP 40% 이하일 때 발사 간격 배율
	 *
	 * 1.5라면 0.2초 → 0.3초
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Turret|Damaged",
		meta = (ClampMin = "1.0")
	)
	float DamagedFireIntervalMultiplier = 1.5f;


private:
	FTimerHandle FireTimerHandle;

	float ActiveTimerInterval = 0.0f;
};