#include "USGInteractionComponent.h"

#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

USGInteractionComponent::USGInteractionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void USGInteractionComponent::BeginPlay()
{
    Super::BeginPlay();

    CachedCamera = FindOwnerCamera();
}

UCameraComponent* USGInteractionComponent::FindOwnerCamera() const
{
    const AActor* OwnerActor = GetOwner();

    if (!OwnerActor)
    {
        return nullptr;
    }

    TArray<UCameraComponent*> CameraComponents;
    OwnerActor->GetComponents<UCameraComponent>(CameraComponents);

    for (UCameraComponent* CameraComponent : CameraComponents)
    {
        if (IsValid(CameraComponent) && CameraComponent->IsActive())
        {
            return CameraComponent;
        }
    }

    return CameraComponents.IsEmpty()
        ? nullptr
        : CameraComponents[0];
}

bool USGInteractionComponent::PerformInteractionTrace(
    FHitResult& OutHitResult
) const
{
    OutHitResult = FHitResult();

    const UWorld* World = GetWorld();
    const AActor* OwnerActor = GetOwner();

    if (!World || !OwnerActor)
    {
        return false;
    }

    UCameraComponent* Camera = CachedCamera;

    if (!IsValid(Camera))
    {
        Camera = FindOwnerCamera();
    }

    if (!IsValid(Camera))
    {
        return false;
    }

    const FVector Start = Camera->GetComponentLocation();
    const FVector End =
        Start +
        Camera->GetForwardVector() * InteractionDistance;

    FCollisionQueryParams QueryParams(
        SCENE_QUERY_STAT(SGInteractionTrace),
        false,
        OwnerActor
    );

    return World->LineTraceSingleByChannel(
        OutHitResult,
        Start,
        End,
        InteractionTraceChannel,
        QueryParams
    );
}

void USGInteractionComponent::TryInteract()
{
    FHitResult HitResult;

    if (!PerformInteractionTrace(HitResult))
    {
        CurrentInteractable.Reset();
        OnInteractionFailed();
        return;
    }

    AActor* HitActor = HitResult.GetActor();

    if (!IsValid(HitActor))
    {
        CurrentInteractable.Reset();
        OnInteractionFailed();
        return;
    }

    CurrentInteractable = HitActor;

    OnInteractionRequested(
        HitActor,
        HitResult
    );
}