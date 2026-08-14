#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "SGHealthComponent.generated.h"

class AController;
class UDamageType;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FSGOnHealthChanged,
	float, OldHealth,
	float, NewHealth,
	float, AppliedDamage
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FSGOnHealthDepleted,
	AActor*, DamageCauser
);

// 최대 체력과 현재 체력을 관리하고 Unreal 기본 데미지 이벤트를 받는 컴포넌트
UCLASS(ClassGroup = (SpaceGround), BlueprintType, meta = (BlueprintSpawnableComponent))
class SPACEGROUND_API USGHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USGHealthComponent();

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }

	// 현재 체력을 0~1 비율로 반환, 나중에 체력바 Percent에 사용 가능
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealthNormalized() const;

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDepleted() const { return bIsDepleted; }

	// 최대 체력을 바꾸고 현재 체력도 최대치로 채움
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetMaxHealth(float NewMaxHealth);

	// 계산이 끝난 최종 데미지를 적용하고 실제로 줄어든 체력 반환
	UFUNCTION(BlueprintCallable, Category = "Health")
	float ApplyHealthDamage(float DamageAmount, AActor* DamageCauser);

	// 체력이 줄어들 때 BP와 UI에 알려줄 이벤트
	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FSGOnHealthChanged OnHealthChanged;

	// 체력이 처음 0이 될 때 알려줄 이벤트, 사망 처리는 아직 연결하지 않음
	UPROPERTY(BlueprintAssignable, Category = "Health|Events")
	FSGOnHealthDepleted OnHealthDepleted;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Health")
	float CurrentHealth = 100.0f;

private:
	// Owner가 ApplyDamage를 받으면 호출되는 연결 함수
	UFUNCTION()
	void HandleOwnerTakeAnyDamage(AActor* DamagedActor, float DamageAmount,
		const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	bool bIsDepleted = false;
};
