#pragma once

#include "CoreMinimal.h"
#include "../Base/StructureBase.h"
#include "Barricade.generated.h"

/*
 * 킬존의 경로를 통제하는 방어형 구조물
 *
 * MVP 담당 기능
 * - 전력 없이 작동
 * - 일반 구조물 피해 수신
 * - 강한 구조물 공격에 추가 피해
 * - 파괴 시 충돌 제거
 */
UCLASS()
class SPACEGROUND_API ABarricade : public AStructureBase
{
	GENERATED_BODY()

public:
	ABarricade();

protected:
	virtual void BeginPlay() override;

	/*
	 * 바리케이드 파괴 시 추가 처리를 수행한다.
	 */
	virtual void HandleStructureDestroyed(
		AActor* DamageCauser
	) override;

public:
	/*
	 * 보스의 양손 내려찍기처럼
	 * 구조물에 강한 공격을 적용할 때 사용한다.
	 *
	 * 실제 적용 피해:
	 * DamageAmount × HeavyAttackDamageMultiplier
	 */
	UFUNCTION(
		BlueprintCallable,
		Category = "Barricade|Damage"
	)
	float ApplyHeavyAttackDamage(
		float DamageAmount,
		AActor* DamageCauser
	);

	/*
	 * 현재 강한 공격 피해 배율을 반환한다.
	 */
	UFUNCTION(
		BlueprintPure,
		Category = "Barricade|Damage"
	)
	float GetHeavyAttackDamageMultiplier() const
	{
		return HeavyAttackDamageMultiplier;
	}


	// ─────────────────────────────────────────────
	// MVP 디버그 테스트

	/*
	 * 캐릭터와 보스가 없는 현재 단계에서
	 * Details 패널 버튼으로 피해 100을 적용한다.
	 */
	UFUNCTION(
		CallInEditor,
		Category = "Barricade|Debug"
	)
	void DebugApplyDamage100();

	/*
	 * Details 패널 버튼으로 HP를 100 회복한다.
	 */
	UFUNCTION(
		CallInEditor,
		Category = "Barricade|Debug"
	)
	void DebugRepair100();

	/*
	 * 일반 피해 400을 적용한다.
	 *
	 * 기획상 보스 공격 약 3회에 파괴되는지
	 * 빠르게 검증하기 위한 버튼이다.
	 */
	UFUNCTION(
		CallInEditor,
		Category = "Barricade|Debug"
	)
	void DebugApplyBossDamage();


protected:
	/*
	 * 보스의 구조물 파괴형 공격에 적용할 피해 배율
	 *
	 * 초기값 1.25:
	 * 기본 피해 400 → 바리케이드 피해 500
	 */
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Barricade|Damage",
		meta = (ClampMin = "1.0")
	)
	float HeavyAttackDamageMultiplier = 1.25f;
};