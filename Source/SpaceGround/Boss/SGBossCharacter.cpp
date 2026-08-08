#include "SGBossCharacter.h"

#include "SGBossAIController.h"
#include "Components/SGBossCombatComponent.h"

ASGBossCharacter::ASGBossCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	BossCombatComponent =
		CreateDefaultSubobject<USGBossCombatComponent>(
			TEXT("BossCombatComponent")
		);

	AIControllerClass = ASGBossAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}
