// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SGPlayerState.generated.h"

/*
* PlayerState — "나에 대한 정보인데, 남도 볼 수 있는 것"
* PlayerController와 헷갈리기 쉬운데 차이
* : PlayerController는 본인 클라이언트한테만 복제되고, PlayerState는 전원한테 복제
* 즉 "다른 플레이어가 봐도 되는 내 정보"만 여기 넣어야 함.

* 개인 킬/기여도, 이 플레이어가 발견/입힌 속성별 데미지 기여도 (누가 약점을 찾았는지 팀원끼리 보이면 좋으니까)
* 준비 페이즈 "레디" 상태 (다들 준비됐는지 서로 봐야 함)
* 생존/사망 여부
* 반대로 인벤토리 내용물처럼 "본인만 알아도 되는 것"은 PlayerState에 넣지 말고 PlayerController나 Pawn 쪽 컴포넌트에 두는 게 맞습니다 
* — 남한테까지 복제할 필요 없는 데이터를 GameState/PlayerState에 넣으면 불필요한 네트워크 트래픽만 늘어남.
 */
UCLASS()
class SPACEGROUND_API ASGPlayerState : public APlayerState
{
	GENERATED_BODY()
	
private:
	ASGPlayerState();
	
	virtual void BeginPlay();
	
public:
	
};
