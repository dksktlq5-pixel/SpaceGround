#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"

#include "SGBossAnimInstance.generated.h"

class ASGBossCharacter;


UCLASS(BlueprintType, Blueprintable)
class SPACEGROUND_API USGBossAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;

	virtual void NativeUpdateAnimation(
		float DeltaSeconds
	) override;

	UFUNCTION(BlueprintPure, Category = "Boss|Animation")
	float GetGroundSpeed() const
	{
		return GroundSpeed;
	}

	UFUNCTION(BlueprintPure, Category = "Boss|Animation")
	ASGBossCharacter* GetBossCharacter() const
	{
		return BossCharacter;
	}

protected:
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Boss|Animation"
	)
	float GroundSpeed = 0.0f;

	UPROPERTY(
		Transient,
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Boss|Animation"
	)
	TObjectPtr<ASGBossCharacter> BossCharacter;

private:
	void CacheBossCharacter();
};
