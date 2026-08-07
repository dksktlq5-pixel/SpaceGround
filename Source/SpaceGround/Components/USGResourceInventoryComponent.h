#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpaceGround/CommonData/SGResourceTypes.h"
#include "USGResourceInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FSGOnResourceChanged,
    ESGResourceType,
    ResourceType,
    int32,
    OldAmount,
    int32,
    NewAmount
);

/**
 * 플레이어가 보유한 건설 및 제작 자원을 관리한다.
 */
UCLASS(
    ClassGroup = (SpaceGround),
    BlueprintType,
    Blueprintable,
    meta = (BlueprintSpawnableComponent)
)
class SPACEGROUND_API USGResourceInventoryComponent
    : public UActorComponent
{
    GENERATED_BODY()

public:
    USGResourceInventoryComponent();

protected:
    virtual void BeginPlay() override;

public:
    /**
     * 자원을 추가한다.
     *
     * @return 실제 변경 이후의 최종 수량
     */
    UFUNCTION(BlueprintCallable, Category = "Resource")
    int32 AddResource(
        ESGResourceType ResourceType,
        int32 Amount
    );

    /**
     * 자원을 차감한다.
     *
     * 수량이 부족하면 아무것도 차감하지 않는다.
     */
    UFUNCTION(BlueprintCallable, Category = "Resource")
    bool ConsumeResource(
        ESGResourceType ResourceType,
        int32 Amount
    );

    /**
     * 여러 자원을 한 번에 차감한다.
     *
     * 하나라도 부족하면 전체 차감을 취소한다.
     */
    UFUNCTION(BlueprintCallable, Category = "Resource")
    bool ConsumeCosts(
        const TArray<FSGResourceCost>& Costs
    );

    UFUNCTION(BlueprintPure, Category = "Resource")
    bool HasResource(
        ESGResourceType ResourceType,
        int32 RequiredAmount
    ) const;

    UFUNCTION(BlueprintPure, Category = "Resource")
    bool CanAffordCosts(
        const TArray<FSGResourceCost>& Costs
    ) const;

    UFUNCTION(BlueprintPure, Category = "Resource")
    int32 GetResourceAmount(
        ESGResourceType ResourceType
    ) const;

    /**
     * 특정 자원의 수량을 직접 설정한다.
     *
     * 초기화와 세이브 복구 용도로 사용한다.
     */
    UFUNCTION(BlueprintCallable, Category = "Resource")
    void SetResourceAmount(
        ESGResourceType ResourceType,
        int32 NewAmount
    );

    UFUNCTION(BlueprintCallable, Category = "Resource")
    void ClearAllResources();

    UFUNCTION(BlueprintPure, Category = "Resource")
    const TMap<ESGResourceType, int32>& GetAllResources() const
    {
        return Resources;
    }

public:
    UPROPERTY(BlueprintAssignable, Category = "Resource")
    FSGOnResourceChanged OnResourceChanged;

protected:
    /**
     * 에디터에서 설정할 초기 자원.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resource")
    TArray<FSGResourceAmount> StartingResources;

private:
    UPROPERTY(VisibleInstanceOnly, Category = "Resource")
    TMap<ESGResourceType, int32> Resources;

private:
    void InitializeStartingResources();
    void BroadcastResourceChanged(
        ESGResourceType ResourceType,
        int32 OldAmount,
        int32 NewAmount
    );
};