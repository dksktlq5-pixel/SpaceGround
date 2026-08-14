#include "SGAnimNotify_BossAttackHit.h"

#include "SpaceGround/Boss/SGBossCharacter.h"
#include "SpaceGround/Boss/Components/SGBossCombatComponent.h"

#include "Components/SkeletalMeshComponent.h"

void USGAnimNotify_BossAttackHit::Notify(
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

	// 실제 판정과 데미지는 Notify가 아닌 CombatComponent에서 처리
	if (USGBossCombatComponent* CombatComponent =
		BossCharacter->GetBossCombatComponent())
	{
		CombatComponent->NotifyExecuteCurrentAttackHit();
	}
}

FString USGAnimNotify_BossAttackHit::GetNotifyName_Implementation() const
{
	return TEXT("Boss Attack Hit");
}
