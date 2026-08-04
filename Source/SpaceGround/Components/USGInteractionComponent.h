#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "USGInteractionComponent.generated.h"

class UCameraComponent;

/**
 * 일반 상호작용을 담당한다.
 *
 * 건설 설치와는 분리한다.
 *
 * 추후 연결 대상:
 * - 자원 아이템 획득
 * - 구조물 수리
 * - 구조물 철거
 * - 터렛 탄약 보급
 * - 보스 시체 조사
 */
UCLASS(
	ClassGroup = (SpaceGround),
	BlueprintType,
	Blueprintable,
	meta = (BlueprintSpawnableComponent)
)
class SPACEGROUND_API USGInteractionComponent
	: public UActorComponent
{
	GENERATED_BODY()

public:
	USGInteractionComponent();

protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void TryInteract();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool PerformInteractionTrace(
		FHitResult& OutHitResult
	) const;

	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetCurrentInteractable() const
	{
		return CurrentInteractable.Get();
	}

protected:
	/**
	 * 실제 상호작용 처리는 BP 또는 추후 Interface에서 구현한다.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnInteractionRequested(
		AActor* TargetActor,
		const FHitResult& HitResult
	);

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnInteractionFailed();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction",
		meta = (ClampMin = "0.0"))
	float InteractionDistance = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	TEnumAsByte<ECollisionChannel> InteractionTraceChannel =
		ECC_Visibility;

private:
	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> CachedCamera;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CurrentInteractable;

private:
	UCameraComponent* FindOwnerCamera() const;
};