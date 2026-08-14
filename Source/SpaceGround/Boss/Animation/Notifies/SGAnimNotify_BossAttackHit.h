#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"

#include "SGAnimNotify_BossAttackHit.generated.h"

// 여러 보스 공격 몽타주에서 공통으로 사용할 타격 Notify
UCLASS(Const, HideCategories = Object, CollapseCategories, meta = (DisplayName = "Boss Attack Hit"))
class SPACEGROUND_API USGAnimNotify_BossAttackHit : public UAnimNotify
{
	GENERATED_BODY()

public:
	// 몽타주가 이 Notify 위치에 도착하면 CombatComponent에 타격 판정 요청
	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference
	) override;

	// 몽타주 Notify 목록에 표시할 이름
	virtual FString GetNotifyName_Implementation() const override;
};
