#include "SGBossCharacter.h"

#include "SGBossAIController.h"
#include "Components/SGBossCombatComponent.h"
#include "SpaceGround/Combat/Components/SGHealthComponent.h"

ASGBossCharacter::ASGBossCharacter()
{
	// Character 자체에서는 매 프레임 처리할 일이 없어서 Tick 끔
	PrimaryActorTick.bCanEverTick = false;

	// 모든 보스가 기본으로 가질 공격 컴포넌트 생성
	BossCombatComponent =
		CreateDefaultSubobject<USGBossCombatComponent>(
			TEXT("BossCombatComponent")
		);

	// 모든 보스가 기본으로 가질 체력 컴포넌트 생성
	HealthComponent =
		CreateDefaultSubobject<USGHealthComponent>(
			TEXT("HealthComponent")
		);
	// Wave 1 아이템 데미지 테스트용 체력
	HealthComponent->SetMaxHealth(3000.0f);

	// 레벨 배치 또는 Spawn 시 SGBossAIController가 자동으로 보스를 조종함
	AIControllerClass = ASGBossAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}
