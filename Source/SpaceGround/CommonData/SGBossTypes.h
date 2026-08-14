#pragma once

#include "CoreMinimal.h"
#include "SpaceGround/CommonData/SGDamageTypes.h"

#include "SGBossTypes.generated.h"

class UAnimMontage;

//보스가 선택하거나 실행할 수 있는 공격 종류
UENUM(BlueprintType)
enum class ESGBossAttackType : uint8
{
	None UMETA(DisplayName = "None"),
	SingleSlam UMETA(DisplayName = "Single Slam"),
	DoubleSlam UMETA(DisplayName = "Double Slam"),
	MouthPounce UMETA(DisplayName = "Mouth Pounce"),
	TailSweep UMETA(DisplayName = "Tail Sweep"),
	VentReposition UMETA(DisplayName = "Vent Reposition")
};

//공격 실행 중 전투 컴포넌트의 공통 상태
UENUM(BlueprintType)
enum class ESGBossAttackState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Tracking UMETA(DisplayName = "Tracking"),
	DirectionLocked UMETA(DisplayName = "Direction Locked"),
	Hitting UMETA(DisplayName = "Hitting"),
	Recovering UMETA(DisplayName = "Recovering")
};

//여러 공격 패턴이 공유하는 설정
USTRUCT(BlueprintType)
struct FSGAttackCommonData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0"))
	float Damage = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0", Units = "cm"))
	float MinimumRange = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0", Units = "cm"))
	float ActivationRange = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0", Units = "s"))
	float RecoveryTime = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0", Units = "s"))
	float Cooldown = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0", Units = "deg/s"))
	float TrackingRotationSpeed = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	EDamageElement DamageElement = EDamageElement::normal;

	//Generic Notify 연결 단계에서 실행에 사용할 Montage
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	TObjectPtr<UAnimMontage> Montage = nullptr;

	//Montage가 없거나 재생에 실패했을 때 임시 타이머 공격을 허용한다(보스 공격구현이 끝나면 없애도됨)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Fallback")
	bool bAllowTimerFallback = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Selection", meta = (ClampMin = "1", ClampMax = "5"))
	int32 MinimumPhase = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Selection", meta = (ClampMin = "0.0"))
	float SelectionWeight = 1.0f;
};

//Single Slam 전용 공간 판정 및 임시 타이밍 설정
USTRUCT(BlueprintType)
struct FSGSingleSlamAttackData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Single Slam")
	FSGAttackCommonData Common;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Single Slam", meta = (ClampMin = "0.0", Units = "cm"))
	float ForwardOffset = 220.0f;

	// 타격 Sphere의 기준으로 사용할 Socket 또는 Bone 이름
	// 이름이 없거나 잘못됐으면 ForwardOffset 위치에서 판정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Single Slam|Hit")
	FName HitSocketName = NAME_None;

	// Socket을 기준으로 판정 위치를 조금 옮길 때 사용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Single Slam|Hit", meta = (Units = "cm"))
	FVector HitSocketOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Single Slam", meta = (ClampMin = "1.0", Units = "cm"))
	float HitRadius = 180.0f;

	// 몽타주 없이 테스트할 때 공격 시작부터 타격까지 걸리는 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fallback Timing", meta = (ClampMin = "0.01", Units = "s"))
	float FallbackWindupTime = 1.2f;

	// 테스트용 타격 시점보다 몇 초 먼저 방향을 고정할지 설정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fallback Timing", meta = (ClampMin = "0.0", Units = "s"))
	float FallbackDirectionLockLeadTime = 0.3f;
};
