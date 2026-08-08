#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "SGBossCharacter.generated.h"

class USGBossCombatComponent;
class UBehaviorTree;


UCLASS(Blueprintable)
class SPACEGROUND_API ASGBossCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASGBossCharacter();

	UFUNCTION(BlueprintPure, Category = "Boss|Combat")
	USGBossCombatComponent* GetBossCombatComponent() const
	{
		return BossCombatComponent;
	}

	UBehaviorTree* GetBossBehaviorTree() const
	{
		return BossBehaviorTree;
	}

protected:
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Boss|Combat",
		meta = (AllowPrivateAccess = "true")
	)
	TObjectPtr<USGBossCombatComponent> BossCombatComponent;

	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Boss|AI"
	)
	TObjectPtr<UBehaviorTree> BossBehaviorTree;
};
