#include "Barricade.h"

#include "Components/StaticMeshComponent.h"


ABarricade::ABarricade()
{
	PrimaryActorTick.bCanEverTick = false;
}


void ABarricade::BeginPlay()
{
	Super::BeginPlay();

	/*
	 * 바리케이드는 전력을 소비하지 않지만 코어에 종속된다.
	 * 코어 파괴 시 상태는 Unpowered가 되며,
	 * 물리 Collision은 그대로 유지되어 장애물 역할은 계속한다.
	 */


	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"[Barricade] 초기화 완료. "
			"Actor=%s ID=%s HP=%.1f/%.1f "
			"HeavyMultiplier=%.2f"
		),
		*GetNameSafe(this),
		*GetStructureID().ToString(),
		GetCurrentHP(),
		GetMaxHP(),
		HeavyAttackDamageMultiplier
	);
}


float ABarricade::ApplyHeavyAttackDamage(
	float DamageAmount,
	AActor* DamageCauser
)
{
	if (DamageAmount <= 0.0f)
	{
		return 0.0f;
	}


	const float FinalDamage =
		DamageAmount *
		FMath::Max(
			1.0f,
			HeavyAttackDamageMultiplier
		);


	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"[Barricade] 강한 구조물 공격. "
			"BaseDamage=%.1f Multiplier=%.2f "
			"FinalDamage=%.1f"
		),
		DamageAmount,
		HeavyAttackDamageMultiplier,
		FinalDamage
	);


	return ApplyStructureDamage(
		FinalDamage,
		DamageCauser
	);
}


void ABarricade::HandleStructureDestroyed(
	AActor* DamageCauser
)
{
	/*
	 * 충돌 제거와 Actor 삭제는
	 * StructureBase에서 공통 처리한다.
	 *
	 * 여기서는 바리케이드 전용 로그와
	 * 추후 파괴 VFX 연결 지점을 제공한다.
	 */
	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"[Barricade] 바리케이드 파괴 처리. "
			"Actor=%s ID=%s"
		),
		*GetNameSafe(this),
		*GetStructureID().ToString()
	);


	Super::HandleStructureDestroyed(
		DamageCauser
	);
}


void ABarricade::DebugApplyDamage100()
{
	if (!HasActorBegunPlay())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[Barricade] DebugApplyDamage100은 "
				"PIE 또는 Simulate 실행 중 사용하세요."
			)
		);

		return;
	}


	ApplyStructureDamage(
		100.0f,
		nullptr
	);
}


void ABarricade::DebugRepair100()
{
	if (!HasActorBegunPlay())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[Barricade] DebugRepair100은 "
				"PIE 또는 Simulate 실행 중 사용하세요."
			)
		);

		return;
	}


	RepairStructure(100.0f);
}


void ABarricade::DebugApplyBossDamage()
{
	if (!HasActorBegunPlay())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[Barricade] DebugApplyBossDamage는 "
				"PIE 또는 Simulate 실행 중 사용하세요."
			)
		);

		return;
	}


	/*
	 * 보스 기본 설치물 피해를 임시로 400으로 가정한다.
	 *
	 * 1.25배 적용:
	 * 400 × 1.25 = 500
	 */
	ApplyHeavyAttackDamage(
		400.0f,
		nullptr
	);
}
