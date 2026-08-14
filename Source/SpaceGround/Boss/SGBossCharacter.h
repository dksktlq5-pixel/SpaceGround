#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "SGBossCharacter.generated.h"

class USGBossCombatComponent;
class USGHealthComponent;
class UBehaviorTree;


UCLASS(Blueprintable)
class SPACEGROUND_API ASGBossCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASGBossCharacter();

	// 보스 공격 컴포넌트를 다른 클래스에서 가져갈 때 사용
	UFUNCTION(BlueprintPure, Category = "Boss|Combat")
	USGBossCombatComponent* GetBossCombatComponent() const
	{
		return BossCombatComponent;
	}

	// 보스 체력 컴포넌트를 가져갈 때 사용
	UFUNCTION(BlueprintPure, Category = "Boss|Health")
	USGHealthComponent* GetHealthComponent() const
	{
		return HealthComponent;
	}

	// BP_SGBoss에 지정된 Behavior Tree 반환
	UBehaviorTree* GetBossBehaviorTree() const
	{
		return BossBehaviorTree;
	}

protected:
	// 공격 상태, 몽타주, 판정, 데미지를 관리하는 컴포넌트
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Boss|Combat",
		meta = (AllowPrivateAccess = "true")
	)
	TObjectPtr<USGBossCombatComponent> BossCombatComponent;

	// 최대 체력과 현재 체력을 관리하는 컴포넌트
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Boss|Health",
		meta = (AllowPrivateAccess = "true")
	)
	TObjectPtr<USGHealthComponent> HealthComponent;

	// 이 보스가 실행할 Behavior Tree
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Boss|AI"
	)
	TObjectPtr<UBehaviorTree> BossBehaviorTree;
};
