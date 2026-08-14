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
	// AnimInstance가 처음 만들어질 때 보스 Character 저장
	virtual void NativeInitializeAnimation() override;

	// 매 애니메이션 업데이트마다 이동 속도 계산
	virtual void NativeUpdateAnimation(
		float DeltaSeconds
	) override;

	// Blend Space에 전달할 지상 이동 속도
	UFUNCTION(BlueprintPure, Category = "Boss|Animation")
	float GetGroundSpeed() const
	{
		return GroundSpeed;
	}

	// 현재 AnimInstance를 사용하는 보스 Character
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
	// Mesh를 소유한 Pawn이 SGBossCharacter인지 확인해서 저장
	void CacheBossCharacter();
};
