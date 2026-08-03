#include "USGConstructionComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/PrimitiveComponent.h"
#include "USGResourceInventoryComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

USGConstructionComponent::USGConstructionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void USGConstructionComponent::BeginPlay()
{
    Super::BeginPlay();

    CachedCamera = FindOwnerCamera();

    if (AActor* OwnerActor = GetOwner())
    {
        ResourceInventory =
            OwnerActor->FindComponentByClass<
                USGResourceInventoryComponent
            >();
    }
}

void USGConstructionComponent::TickComponent(
    const float DeltaTime,
    const ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction
)
{
    Super::TickComponent(
        DeltaTime,
        TickType,
        ThisTickFunction
    );

    if (!IsBuildModeActive())
    {
        return;
    }

    UpdatePlacementPreview();
}

UCameraComponent* USGConstructionComponent::FindOwnerCamera() const
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

void USGConstructionComponent::EnterBuildMode()
{
    if (IsBuildModeActive())
    {
        return;
    }

    BuildModeState = ESGBuildModeState::Previewing;

    SetComponentTickEnabled(true);

    if (SelectedPreviewClass)
    {
        SpawnPreviewActor();
    }

    OnBuildModeEntered();
    OnBuildModeChanged.Broadcast(true);
}

void USGConstructionComponent::ExitBuildMode()
{
    if (!IsBuildModeActive())
    {
        return;
    }

    BuildModeState = ESGBuildModeState::Inactive;

    SetComponentTickEnabled(false);

    DestroyPreviewActor();
    CurrentPlacementResult.Reset();

    OnBuildModeExited();
    OnBuildModeChanged.Broadcast(false);
}

void USGConstructionComponent::ToggleBuildMode()
{
    if (IsBuildModeActive())
    {
        ExitBuildMode();
    }
    else
    {
        EnterBuildMode();
    }
}

void USGConstructionComponent::SetSelectedStructureDefinition(
    const FName StructureRowName,
    const TSubclassOf<AActor> StructureClass,
    const TSubclassOf<AActor> PreviewClass,
    const TArray<FSGResourceCost>& ResourceCosts,
    const bool bRequiresPower,
    const float MaxAllowedSlope,
    const FVector PlacementExtent
)
{
    SelectedStructureRow = StructureRowName;
    SelectedStructureClass = StructureClass;
    SelectedPreviewClass = PreviewClass;
    SelectedResourceCosts = ResourceCosts;
    bSelectedRequiresPower = bRequiresPower;

    SelectedMaxSlopeDegrees =
        FMath::Clamp(MaxAllowedSlope, 0.0f, 89.0f);

    SelectedPlacementExtent = FVector(
        FMath::Max(1.0f, PlacementExtent.X),
        FMath::Max(1.0f, PlacementExtent.Y),
        FMath::Max(1.0f, PlacementExtent.Z)
    );

    CurrentPreviewYaw = 0.0f;

    DestroyPreviewActor();

    if (IsBuildModeActive() && SelectedPreviewClass)
    {
        SpawnPreviewActor();
    }
}

void USGConstructionComponent::ClearSelectedStructure()
{
    SelectedStructureRow = NAME_None;
    SelectedStructureClass = nullptr;
    SelectedPreviewClass = nullptr;
    SelectedResourceCosts.Empty();
    bSelectedRequiresPower = false;

    CurrentPreviewYaw = 0.0f;

    DestroyPreviewActor();
    CurrentPlacementResult.Reset();
}

void USGConstructionComponent::RotatePreview(
    const float RotationAmount
)
{
    if (!IsBuildModeActive())
    {
        return;
    }

    const float ActualRotation =
        FMath::IsNearlyZero(RotationAmount)
        ? RotationStep
        : RotationAmount;

    CurrentPreviewYaw =
        FMath::Fmod(
            CurrentPreviewYaw + ActualRotation,
            360.0f
        );

    UpdatePlacementPreview();
}

void USGConstructionComponent::CancelPlacement()
{
    ExitBuildMode();
}

bool USGConstructionComponent::CheckGroundTrace(
    FHitResult& OutGroundHit
) const
{
    OutGroundHit = FHitResult();

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

    const FVector TraceStart =
        Camera->GetComponentLocation();

    const FVector TraceEnd =
        TraceStart +
        Camera->GetForwardVector() * PlacementDistance;

    FCollisionQueryParams QueryParams(
        SCENE_QUERY_STAT(SGPlacementTrace),
        false,
        OwnerActor
    );

    if (IsValid(PreviewActor))
    {
        QueryParams.AddIgnoredActor(PreviewActor);
    }

    return World->LineTraceSingleByChannel(
        OutGroundHit,
        TraceStart,
        TraceEnd,
        PlacementTraceChannel,
        QueryParams
    );
}

bool USGConstructionComponent::CheckSlope(
    const FVector& SurfaceNormal
) const
{
    const FVector SafeNormal =
        SurfaceNormal.GetSafeNormal();

    const float DotValue = FMath::Clamp(
        FVector::DotProduct(
            SafeNormal,
            FVector::UpVector
        ),
        -1.0f,
        1.0f
    );

    const float SlopeDegrees =
        FMath::RadiansToDegrees(
            FMath::Acos(DotValue)
        );

    return SlopeDegrees <= SelectedMaxSlopeDegrees;
}

bool USGConstructionComponent::CheckPlacementOverlap(
    const FTransform& PlacementTransform
) const
{
    const UWorld* World = GetWorld();
    const AActor* OwnerActor = GetOwner();

    if (!World || !OwnerActor)
    {
        return false;
    }

    FCollisionQueryParams QueryParams(
        SCENE_QUERY_STAT(SGPlacementOverlap),
        false,
        OwnerActor
    );

    if (IsValid(PreviewActor))
    {
        QueryParams.AddIgnoredActor(PreviewActor);
    }

    const FCollisionShape CollisionShape =
        FCollisionShape::MakeBox(SelectedPlacementExtent);

    const bool bBlockingOverlap =
        World->OverlapBlockingTestByChannel(
            PlacementTransform.GetLocation(),
            PlacementTransform.GetRotation(),
            PlacementOverlapChannel,
            CollisionShape,
            QueryParams
        );

    return !bBlockingOverlap;
}

bool USGConstructionComponent::CheckResources() const
{
    if (!IsValid(ResourceInventory))
    {
        /*
         * 아직 자원 컴포넌트가 없는 테스트 캐릭터에서는
         * 비용이 없을 때만 설치 가능하게 한다.
         */
        return SelectedResourceCosts.IsEmpty();
    }

    return ResourceInventory->CanAffordCosts(
        SelectedResourceCosts
    );
}

bool USGConstructionComponent::CheckPowerRequirement_Implementation(
    const FVector& PlacementLocation
) const
{
    /*
     * 비전력 구조물은 항상 통과한다.
     *
     * 전력 구조물은 PowerCore 구조와 연결하기 전까지
     * Blueprint Override에서 판정해야 한다.
     */
    return !bSelectedRequiresPower;
}

bool USGConstructionComponent::CalculatePlacementResult(
    FSGPlacementResult& OutResult
) const
{
    OutResult.Reset();

    if (!SelectedStructureClass)
    {
        OutResult.FailureReason =
            ESGPlacementFailureReason::InvalidDefinition;

        return false;
    }

    FHitResult GroundHit;

    if (!CheckGroundTrace(GroundHit))
    {
        OutResult.FailureReason =
            ESGPlacementFailureReason::NoSurface;

        return false;
    }

    OutResult.GroundHit = GroundHit;

    const FVector PlacementLocation =
        GroundHit.ImpactPoint +
        GroundHit.ImpactNormal * GroundOffset;

    const FRotator PlacementRotation(
        0.0f,
        CurrentPreviewYaw,
        0.0f
    );

    OutResult.PlacementTransform = FTransform(
        PlacementRotation,
        PlacementLocation,
        FVector::OneVector
    );

    if (!CheckSlope(GroundHit.ImpactNormal))
    {
        OutResult.FailureReason =
            ESGPlacementFailureReason::TooSteep;

        return false;
    }

    if (!CheckPlacementOverlap(
        OutResult.PlacementTransform
    ))
    {
        OutResult.FailureReason =
            ESGPlacementFailureReason::Overlap;

        return false;
    }

    if (!CheckResources())
    {
        OutResult.FailureReason =
            ESGPlacementFailureReason::InsufficientResources;

        return false;
    }

    if (!CheckPowerRequirement(PlacementLocation))
    {
        OutResult.FailureReason =
            ESGPlacementFailureReason::OutsidePowerRange;

        return false;
    }

    OutResult.bCanPlace = true;
    OutResult.FailureReason =
        ESGPlacementFailureReason::None;

    return true;
}

void USGConstructionComponent::UpdatePlacementPreview()
{
    FSGPlacementResult NewResult;
    CalculatePlacementResult(NewResult);

    const bool bValidityChanged =
        NewResult.bCanPlace !=
            CurrentPlacementResult.bCanPlace
        ||
        NewResult.FailureReason !=
            CurrentPlacementResult.FailureReason;

    CurrentPlacementResult = NewResult;

    if (IsValid(PreviewActor))
    {
        PreviewActor->SetActorTransform(
            CurrentPlacementResult.PlacementTransform
        );
    }

    if (bValidityChanged)
    {
        OnPreviewValidityChanged(
            CurrentPlacementResult.bCanPlace,
            CurrentPlacementResult.FailureReason
        );
    }

    OnPlacementResultChanged.Broadcast(
        CurrentPlacementResult
    );
}

void USGConstructionComponent::SpawnPreviewActor()
{
    UWorld* World = GetWorld();

    if (!World || !SelectedPreviewClass)
    {
        return;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = GetOwner();

    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    PreviewActor = World->SpawnActor<AActor>(
        SelectedPreviewClass,
        FTransform::Identity,
        SpawnParameters
    );

    ConfigurePreviewActor(PreviewActor);
}

void USGConstructionComponent::ConfigurePreviewActor(
    AActor* InPreviewActor
) const
{
    if (!IsValid(InPreviewActor))
    {
        return;
    }

    TArray<UPrimitiveComponent*> PrimitiveComponents;
    InPreviewActor->GetComponents<UPrimitiveComponent>(
        PrimitiveComponents
    );

    for (UPrimitiveComponent* Primitive : PrimitiveComponents)
    {
        if (!IsValid(Primitive))
        {
            continue;
        }

        Primitive->SetCollisionEnabled(
            ECollisionEnabled::NoCollision
        );

        Primitive->SetGenerateOverlapEvents(false);
    }
}

void USGConstructionComponent::DestroyPreviewActor()
{
    if (!IsValid(PreviewActor))
    {
        PreviewActor = nullptr;
        return;
    }

    PreviewActor->Destroy();
    PreviewActor = nullptr;
}

bool USGConstructionComponent::TryPlaceSelectedStructure()
{
    if (!IsBuildModeActive())
    {
        return false;
    }

    FSGPlacementResult PlacementResult;

    if (!CalculatePlacementResult(PlacementResult))
    {
        CurrentPlacementResult = PlacementResult;

        OnPreviewValidityChanged(
            false,
            PlacementResult.FailureReason
        );

        return false;
    }

    UWorld* World = GetWorld();

    if (!World || !SelectedStructureClass)
    {
        return false;
    }

    /*
     * Spawn 실패 시 자원이 차감되지 않도록
     * 우선 Spawn하고 성공한 뒤 비용을 차감한다.
     */
    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = GetOwner();

    if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
    {
        SpawnParameters.Instigator = OwnerPawn;
    }

    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

    AActor* SpawnedStructure =
        World->SpawnActor<AActor>(
            SelectedStructureClass,
            PlacementResult.PlacementTransform,
            SpawnParameters
        );

    if (!IsValid(SpawnedStructure))
    {
        return false;
    }

    if (IsValid(ResourceInventory))
    {
        if (!ResourceInventory->ConsumeCosts(
            SelectedResourceCosts
        ))
        {
            /*
             * 앞선 판정 이후 다른 로직에서 자원이 사용됐을 수 있다.
             */
            SpawnedStructure->Destroy();
            return false;
        }
    }

    OnStructurePlaced.Broadcast(SpawnedStructure);

    UpdatePlacementPreview();

    return true;
}