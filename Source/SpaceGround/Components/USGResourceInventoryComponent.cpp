#include "USGResourceInventoryComponent.h"

USGResourceInventoryComponent::USGResourceInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void USGResourceInventoryComponent::BeginPlay()
{
    Super::BeginPlay();

    InitializeStartingResources();
}

void USGResourceInventoryComponent::InitializeStartingResources()
{
    Resources.Empty();

    for (const FSGResourceAmount& Resource : StartingResources)
    {
        if (Resource.Amount <= 0)
        {
            continue;
        }

        const int32 ExistingAmount =
            Resources.FindRef(Resource.ResourceType);

        Resources.Add(
            Resource.ResourceType,
            ExistingAmount + Resource.Amount
        );
    }
}

int32 USGResourceInventoryComponent::AddResource(
    const ESGResourceType ResourceType,
    const int32 Amount
)
{
    if (Amount <= 0)
    {
        return GetResourceAmount(ResourceType);
    }

    const int32 OldAmount = GetResourceAmount(ResourceType);
    const int32 NewAmount = OldAmount + Amount;

    Resources.Add(ResourceType, NewAmount);

    BroadcastResourceChanged(
        ResourceType,
        OldAmount,
        NewAmount
    );

    return NewAmount;
}

bool USGResourceInventoryComponent::ConsumeResource(
    const ESGResourceType ResourceType,
    const int32 Amount
)
{
    if (Amount <= 0)
    {
        return true;
    }

    const int32 OldAmount = GetResourceAmount(ResourceType);

    if (OldAmount < Amount)
    {
        return false;
    }

    const int32 NewAmount = OldAmount - Amount;

    Resources.Add(ResourceType, NewAmount);

    BroadcastResourceChanged(
        ResourceType,
        OldAmount,
        NewAmount
    );

    return true;
}

bool USGResourceInventoryComponent::ConsumeCosts(
    const TArray<FSGResourceCost>& Costs
)
{
    if (!CanAffordCosts(Costs))
    {
        return false;
    }

    for (const FSGResourceCost& Cost : Costs)
    {
        if (Cost.Amount <= 0)
        {
            continue;
        }

        ConsumeResource(
            Cost.ResourceType,
            Cost.Amount
        );
    }

    return true;
}

bool USGResourceInventoryComponent::HasResource(
    const ESGResourceType ResourceType,
    const int32 RequiredAmount
) const
{
    if (RequiredAmount <= 0)
    {
        return true;
    }

    return GetResourceAmount(ResourceType) >= RequiredAmount;
}

bool USGResourceInventoryComponent::CanAffordCosts(
    const TArray<FSGResourceCost>& Costs
) const
{
    /*
     * 동일 자원이 여러 번 들어올 가능성까지 고려해서
     * 자원별 총 필요량을 먼저 계산한다.
     */
    TMap<ESGResourceType, int32> TotalRequired;

    for (const FSGResourceCost& Cost : Costs)
    {
        if (Cost.Amount <= 0)
        {
            continue;
        }

        TotalRequired.FindOrAdd(Cost.ResourceType) += Cost.Amount;
    }

    for (const TPair<ESGResourceType, int32>& Pair : TotalRequired)
    {
        if (!HasResource(Pair.Key, Pair.Value))
        {
            return false;
        }
    }

    return true;
}

int32 USGResourceInventoryComponent::GetResourceAmount(
    const ESGResourceType ResourceType
) const
{
    return Resources.FindRef(ResourceType);
}

void USGResourceInventoryComponent::SetResourceAmount(
    const ESGResourceType ResourceType,
    const int32 NewAmount
)
{
    const int32 OldAmount = GetResourceAmount(ResourceType);
    const int32 ClampedAmount = FMath::Max(0, NewAmount);

    Resources.Add(ResourceType, ClampedAmount);

    BroadcastResourceChanged(
        ResourceType,
        OldAmount,
        ClampedAmount
    );
}

void USGResourceInventoryComponent::ClearAllResources()
{
    const TMap<ESGResourceType, int32> PreviousResources = Resources;

    Resources.Empty();

    for (const TPair<ESGResourceType, int32>& Pair : PreviousResources)
    {
        if (Pair.Value <= 0)
        {
            continue;
        }

        BroadcastResourceChanged(
            Pair.Key,
            Pair.Value,
            0
        );
    }
}

void USGResourceInventoryComponent::BroadcastResourceChanged(
    const ESGResourceType ResourceType,
    const int32 OldAmount,
    const int32 NewAmount
)
{
    if (OldAmount == NewAmount)
    {
        return;
    }

    OnResourceChanged.Broadcast(
        ResourceType,
        OldAmount,
        NewAmount
    );
}