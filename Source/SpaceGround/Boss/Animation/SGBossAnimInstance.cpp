#include "SGBossAnimInstance.h"

#include "SpaceGround/Boss/SGBossCharacter.h"

void USGBossAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	// ABP가 시작될 때 Owner 보스를 한 번 찾아둠
	CacheBossCharacter();
}

void USGBossAnimInstance::NativeUpdateAnimation(
	const float DeltaSeconds
)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// 초기화 순서 때문에 못 찾았으면 다시 시도
	if (!IsValid(BossCharacter))
	{
		CacheBossCharacter();
	}

	if (!IsValid(BossCharacter))
	{
		GroundSpeed = 0.0f;
		return;
	}

	// Z축을 제외한 실제 지상 이동 속도
	GroundSpeed =
		BossCharacter->GetVelocity().Size2D();
}

void USGBossAnimInstance::CacheBossCharacter()
{
	BossCharacter = Cast<ASGBossCharacter>(
		TryGetPawnOwner()
	);
}
