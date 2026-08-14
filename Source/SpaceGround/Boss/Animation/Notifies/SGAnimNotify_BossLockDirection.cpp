#include "SGAnimNotify_BossLockDirection.h"

#include "SpaceGround/Boss/SGBossCharacter.h"
#include "SpaceGround/Boss/Components/SGBossCombatComponent.h"

#include "Components/SkeletalMeshComponent.h"

void USGAnimNotify_BossLockDirection::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	// Notify를 실행한 Mesh의 Owner가 보스인지 확인
	ASGBossCharacter* BossCharacter = MeshComp
		? Cast<ASGBossCharacter>(MeshComp->GetOwner())
		: nullptr;
	if (!IsValid(BossCharacter))
	{
		return;
	}

	// 실제 방향 고정 규칙은 Notify가 아닌 CombatComponent에서 처리
	if (USGBossCombatComponent* CombatComponent =
		BossCharacter->GetBossCombatComponent())
	{
		CombatComponent->NotifyLockAttackDirection();
	}
}

FString USGAnimNotify_BossLockDirection::GetNotifyName_Implementation() const
{
	return TEXT("Boss Lock Direction");
}
