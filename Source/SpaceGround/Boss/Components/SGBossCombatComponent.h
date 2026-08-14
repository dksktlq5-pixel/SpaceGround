#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpaceGround/CommonData/SGBossTypes.h"

#include "SGBossCombatComponent.generated.h"

class AActor;
class UAnimInstance;
class UAnimMontage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FSGOnBossAttackFinished,
	ESGBossAttackType,
	AttackType,
	bool,
	bSucceeded
);

// 공격 상태, 몽타주, 방향 추적, 판정, 데미지를 관리하는 컴포넌트
UCLASS(ClassGroup = (SpaceGround), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class SPACEGROUND_API USGBossCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USGBossCombatComponent();

	// 공격 준비 중 방향 추적과 몽타주 없는 테스트용 타이머 처리
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	// 공격 종류와 타깃을 받아 공통 공격 시작 처리
	UFUNCTION(BlueprintCallable, Category = "Boss|Combat")
	bool StartAttack(ESGBossAttackType AttackType, AActor* TargetActor);

	// Single Slam 시작을 공통 StartAttack 함수에 요청
	UFUNCTION(BlueprintCallable, Category = "Boss|Combat")
	bool StartSingleSlam(AActor* TargetActor);

	// BT가 중단됐거나 문제가 생겼을 때 현재 공격 취소
	UFUNCTION(BlueprintCallable, Category = "Boss|Combat")
	void CancelCurrentAttack();

	// 방향 고정 Anim Notify가 호출하는 공통 진입점
	UFUNCTION(BlueprintCallable, Category = "Boss|Combat|Notify")
	void NotifyLockAttackDirection();

	// 타격 Anim Notify가 호출하는 공통 진입점
	UFUNCTION(BlueprintCallable, Category = "Boss|Combat|Notify")
	void NotifyExecuteCurrentAttackHit();

	// 거리, 쿨다운, 현재 상태를 확인해서 공격 시작 가능 여부 반환
	UFUNCTION(BlueprintPure, Category = "Boss|Combat")
	bool CanStartAttack(ESGBossAttackType AttackType, AActor* TargetActor) const;

	UFUNCTION(BlueprintPure, Category = "Boss|Combat")
	float GetRemainingCooldown(ESGBossAttackType AttackType) const;

	UFUNCTION(BlueprintPure, Category = "Boss|Combat")
	bool IsAttacking() const { return AttackState != ESGBossAttackState::Idle; }

	UFUNCTION(BlueprintPure, Category = "Boss|Combat")
	ESGBossAttackType GetCurrentAttackType() const { return CurrentAttackType; }

	UPROPERTY(BlueprintAssignable, Category = "Boss|Combat")
	FSGOnBossAttackFinished OnBossAttackFinished;

protected:
	// BP_SGBoss의 Details 패널에서 수정할 Single Slam 설정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Combat|Attacks")
	FSGSingleSlamAttackData SingleSlamData;

	// 공격 판정 Sphere를 화면에 표시할지 결정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Debug")
	bool bDrawDebugAttack = true;

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Combat|Animation")
	void BP_OnAttackStarted(ESGBossAttackType AttackType);

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Combat|Animation")
	void BP_OnAttackDirectionLocked(ESGBossAttackType AttackType);

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Combat|Animation")
	void BP_OnAttackHit(ESGBossAttackType AttackType);

private:
	// 공격 종류에 맞는 공통 설정 찾기
	const FSGAttackCommonData* FindCommonAttackData(ESGBossAttackType AttackType) const;
	// 데미지, 거리, 타이밍 값이 올바른지 확인
	bool ValidateAttackData(ESGBossAttackType AttackType) const;
	// 공격 몽타주 재생 후 종료 이벤트와 안전 타이머 연결
	bool TryPlayAttackMontage(const FSGAttackCommonData& CommonData);
	// 보스 Mesh가 사용하는 AnimInstance 가져오기
	UAnimInstance* GetOwnerAnimInstance() const;
	// 준비 동작 중 플레이어 방향으로 회전
	void UpdateTargetTracking(float DeltaTime);
	// 현재 전방 방향을 공격 방향으로 저장
	void LockAttackDirection();
	// 앞다리 Socket 위치에서 Single Slam 판정
	void ExecuteSingleSlamHit();
	// 몽타주 종료 후 Recovery 시작
	void BeginRecovery();
	void HandleRecoveryFinished();
	void HandleAttackSafetyTimeout();
	void HandleAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	// 공격에 사용한 Timer 정리
	void ClearAttackTimers();
	// 몽타주 종료 이벤트 연결 정리
	void ClearMontageDelegate();
	// 공격 결과 전달 후 모든 임시 상태 초기화
	void FinishCurrentAttack(bool bSucceeded);

	UPROPERTY(VisibleInstanceOnly, Category = "Boss|Combat")
	ESGBossAttackState AttackState = ESGBossAttackState::Idle;

	UPROPERTY(VisibleInstanceOnly, Category = "Boss|Combat")
	ESGBossAttackType CurrentAttackType = ESGBossAttackType::None;

	UPROPERTY(Transient)
	TObjectPtr<AActor> CurrentTarget;

	// 공격 종류별 다음 사용 가능 월드 시간
	UPROPERTY(Transient)
	TMap<ESGBossAttackType, float> NextAttackAllowedTimes;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveAttackMontage;

	float AttackElapsedTime = 0.0f;
	FVector LockedAttackDirection = FVector::ForwardVector;
	// 한 공격에서 Hit Notify가 중복 적용되는 것 방지
	bool bHitExecuted = false;
	// 몽타주 Notify 방식인지 테스트용 타이머 방식인지 구분
	bool bUsingMontageTiming = false;
	// 방향 고정 Notify가 정상 실행됐는지 확인
	bool bDirectionLockedByNotify = false;
	// 공격 종료 함수가 중복 호출되는 것 방지
	bool bFinishingAttack = false;

	FTimerHandle RecoveryTimerHandle;
	FTimerHandle AttackSafetyTimerHandle;
};
