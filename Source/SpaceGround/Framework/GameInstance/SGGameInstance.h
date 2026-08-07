// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SpaceGround/CommonData/SGDamageTypes.h"
#include "SGGameInstance.generated.h"

/**
* GameInstance — "판이 바뀌어도 안 죽는 기억"
* 레벨을 이동해도(로비→킬존→결과창) 안 사라지는, 클라이언트 개인 프로세스에 붙은 클래스. 
* 서버/클라가 서로 복제해주는 게 아니라 각자 따로 가지고 있는 것이라는 점이 중요.

* "속성별 데미지 통계 기록 → 다음 등장 시 저항 생길 수 있음"
* 한 판(레벨) 끝나고 다음 보스 조우로 넘어갈 때도 안 사라져야 하는 데이터
* 세션/매치메이킹 정보(같이 할 인원 모으기), 사운드/그래픽 설정 같은 전역 설정
* (나중에 완전히 껐다 켜도 유지해야 하면 SaveGame 오브젝트를 여기서 들고 있는 식으로 확장)
 */
UCLASS()
class SPACEGROUND_API USGGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
private:
	virtual void Init() override; // 게임 시작 시 1회 (레벨 로드보다도 먼저)
	virtual void Shutdown() override; // 게임 종료 시 -> 게임 진행상황 저장?
	
	void Initialize_TotalDamageAmounts();
	
	// todo : , 옵션 전역 설정, SaveGame 오브젝트 저장 함수
#pragma region Damage Statistics
public:
	UPROPERTY(BlueprintReadOnly)
	TMap<EDamageElement, float> TotalDamageAmounts; // 모든 라운드 합산 속성별 데미지

	UFUNCTION(BlueprintCallable)
	void Add_RoundDamageAmount(const TMap<EDamageElement, float>& RoundDamageAmount);

#pragma endregion
	
};
