#include "SGBossAnimInstance.h"

#include "SpaceGround/Boss/SGBossCharacter.h"

void USGBossAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	CacheBossCharacter();
}

void USGBossAnimInstance::NativeUpdateAnimation(
	const float DeltaSeconds
)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!IsValid(BossCharacter))
	{
		CacheBossCharacter();
	}

	if (!IsValid(BossCharacter))
	{
		GroundSpeed = 0.0f;
		return;
	}

	GroundSpeed =
		BossCharacter->GetVelocity().Size2D();
}

void USGBossAnimInstance::CacheBossCharacter()
{
	BossCharacter = Cast<ASGBossCharacter>(
		TryGetPawnOwner()
	);
}
