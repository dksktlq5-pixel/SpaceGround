#include "USGConstructionComponent.h"

#include "../Structures/Base/StructureDefinition.h"

#include "Camera/CameraComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

#include "USGResourceInventoryComponent.h"

USGConstructionComponent::USGConstructionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

    /*
     * 평상시에는 Tick을 사용하지 않는다.
     * 건설 모드에 진입할 때만 활성화한다.
     */
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void USGConstructionComponent::BeginPlay()
{
    Super::BeginPlay();

    CachedCamera = FindOwnerCamera();

    if (!IsValid(CachedCamera))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "USGConstructionComponent: "
                "Owner camera was not found."
            )
        );
    }

    if (AActor* OwnerActor = GetOwner())
    {
        ResourceInventory =
            OwnerActor->FindComponentByClass<
                USGResourceInventoryComponent
            >();
    }

    if (!IsValid(ResourceInventory))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "USGConstructionComponent: "
                "Resource inventory was not found."
            )
        );
    }

    /*
     * DataTable 설정 검사.
     */
    if (!IsValid(StructureDataTable))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "USGConstructionComponent: "
                "StructureDataTable is not assigned."
            )
        );

        return;
    }

    /*
     * 잘못된 Row Struct를 가진 DataTable 연결 방지.
     */
    if (StructureDataTable->GetRowStruct()
        != FSGStructureDefinition::StaticStruct())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT(
                "USGConstructionComponent: "
                "StructureDataTable has an invalid Row Struct. "
                "Expected FSGStructureDefinition."
            )
        );
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

UCameraComponent*
USGConstructionComponent::FindOwnerCamera() const
{
    const AActor* OwnerActor = GetOwner();

    if (!OwnerActor)
    {
        return nullptr;
    }

    TArray<UCameraComponent*> CameraComponents;

    OwnerActor->GetComponents<UCameraComponent>(
        CameraComponents
    );

    /*
     * 현재 활성화된 카메라를 우선 사용한다.
     */
    for (UCameraComponent* CameraComponent :
        CameraComponents)
    {
        if (IsValid(CameraComponent)
            &&
            CameraComponent->IsActive())
        {
            return CameraComponent;
        }
    }

    /*
     * 활성 카메라를 찾지 못하면 첫 번째 카메라를 사용한다.
     */
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

    /*
     * 선택된 구조물이 없으면 기본 구조물을 자동 선택한다.
     */
    if (!HasSelectedStructure()
        &&
        bAutoSelectDefaultStructure
        &&
        !DefaultStructureRowName.IsNone())
    {
        if (!SelectStructureByRowName(
            DefaultStructureRowName
        ))
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT(
                    "Construction mode could not select "
                    "the default structure Row: %s"
                ),
                *DefaultStructureRowName.ToString()
            );
        }
    }

    BuildModeState =
        ESGBuildModeState::Previewing;

    SetComponentTickEnabled(true);

    if (SelectedPreviewClass)
    {
        SpawnPreviewActor();
    }

    UpdatePlacementPreview();

    OnBuildModeEntered();

    OnBuildModeChanged.Broadcast(true);

    UE_LOG(
        LogTemp,
        Log,
        TEXT(
            "Construction component entered build mode. "
            "SelectedRow=%s"
        ),
        *SelectedStructureRow.ToString()
    );
}

void USGConstructionComponent::ExitBuildMode()
{
    if (!IsBuildModeActive())
    {
        return;
    }

    BuildModeState =
        ESGBuildModeState::Inactive;

    SetComponentTickEnabled(false);

    DestroyPreviewActor();

    CurrentPlacementResult.Reset();

    OnBuildModeExited();

    OnBuildModeChanged.Broadcast(false);

    UE_LOG(
        LogTemp,
        Log,
        TEXT("Construction component exited build mode.")
    );
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

bool USGConstructionComponent::HasStructureRow(
    const FName StructureRowName
) const
{
    if (!IsValid(StructureDataTable)
        ||
        StructureRowName.IsNone())
    {
        return false;
    }

    return StructureDataTable->GetRowMap().Contains(
        StructureRowName
    );
}

bool USGConstructionComponent::SelectStructureByRowName(
    const FName StructureRowName
)
{
    if (StructureRowName.IsNone())
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "SelectStructureByRowName failed: "
                "RowName is None."
            )
        );

        return false;
    }

    /*
     * 이미 같은 구조물이 정상적으로 선택되어 있다면
     * DataTable 조회, Preview Destroy, Preview Spawn을
     * 다시 수행하지 않는다.
     */
    if (SelectedStructureRow == StructureRowName
        &&
        SelectedStructureClass != nullptr)
    {
        return true;
    }

    if (!IsValid(StructureDataTable))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT(
                "SelectStructureByRowName failed: "
                "StructureDataTable is not assigned."
            )
        );

        return false;
    }

    if (StructureDataTable->GetRowStruct()
        != FSGStructureDefinition::StaticStruct())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT(
                "SelectStructureByRowName failed: "
                "DataTable Row Struct is not "
                "FSGStructureDefinition."
            )
        );

        return false;
    }

    static const FString ContextString =
        TEXT(
            "USGConstructionComponent::"
            "SelectStructureByRowName"
        );

    const FSGStructureDefinition* Definition =
        StructureDataTable->FindRow<
            FSGStructureDefinition
        >(
            StructureRowName,
            ContextString,
            true
        );

    if (!Definition)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "Structure DataTable Row "
                "was not found: %s"
            ),
            *StructureRowName.ToString()
        );

        return false;
    }

    return ApplyStructureDefinition(
        StructureRowName,
        *Definition
    );
}

bool USGConstructionComponent::ApplyStructureDefinition(
    const FName StructureRowName,
    const FSGStructureDefinition& Definition
)
{
    if (!Definition.StructureClass)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "Structure Row '%s' has no StructureClass."
            ),
            *StructureRowName.ToString()
        );

        return false;
    }

    SelectedStructureRow =
        StructureRowName;

    SelectedStructureClass =
        Definition.StructureClass;

    /*
     * PreviewClass가 없으면 실제 StructureClass를
     * 임시 프리뷰로 사용한다.
     *
     * 실제 구조물 BeginPlay 로직까지 실행되므로
     * 이후 전용 Preview BP로 분리하는 것이 권장된다.
     */
    if (Definition.PreviewClass)
    {
        SelectedPreviewClass =
            Definition.PreviewClass;
    }
    else
    {
        SelectedPreviewClass =
            Definition.StructureClass;

        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "Structure Row '%s' has no PreviewClass. "
                "StructureClass will be used as preview."
            ),
            *StructureRowName.ToString()
        );
    }

    SelectedResourceCosts =
        Definition.BuildCosts;

    bSelectedRequiresPower =
        Definition.PowerData.bRequiresPower;

    SelectedMaxSlopeDegrees =
        FMath::Clamp(
            Definition.MaxAllowedSlope,
            0.0f,
            89.0f
        );

    SelectedPlacementExtent = FVector(
        FMath::Max(
            1.0f,
            FMath::Abs(
                Definition.PlacementExtent.X
            )
        ),
        FMath::Max(
            1.0f,
            FMath::Abs(
                Definition.PlacementExtent.Y
            )
        ),
        FMath::Max(
            1.0f,
            FMath::Abs(
                Definition.PlacementExtent.Z
            )
        )
    );

    SelectedGroundOffset =
        Definition.GroundOffset;

    bSelectedRequireFullGroundSupport =
        Definition.bRequireFullGroundSupport;

    SelectedSupportTraceStartHeight =
        FMath::Max(
            1.0f,
            Definition.SupportTraceStartHeight
        );

    SelectedSupportTraceDepth =
        FMath::Max(
            1.0f,
            Definition.SupportTraceDepth
        );

    SelectedStructureSpacing =
        FMath::Max(
            0.0f,
            Definition.StructureSpacing
        );

    CurrentPreviewYaw = 0.0f;

    DestroyPreviewActor();

    CurrentPlacementResult.Reset();

    if (IsBuildModeActive())
    {
        if (SelectedPreviewClass)
        {
            SpawnPreviewActor();
        }

        UpdatePlacementPreview();
    }

    OnStructureSelected.Broadcast(
        SelectedStructureRow
    );

    UE_LOG(
        LogTemp,
        Log,
        TEXT(
            "Structure selected from DataTable: "
            "Row=%s ID=%s RequiresPower=%s "
            "MaxSlope=%.1f Extent=%s "
            "FullSupport=%s SupportDepth=%.1f "
            "Spacing=%.1f"
        ),
        *SelectedStructureRow.ToString(),
        *Definition.StructureID.ToString(),
        bSelectedRequiresPower
            ? TEXT("true")
            : TEXT("false"),
        SelectedMaxSlopeDegrees,
        *SelectedPlacementExtent.ToString(),
        bSelectedRequireFullGroundSupport
            ? TEXT("true")
            : TEXT("false"),
        SelectedSupportTraceDepth,
        SelectedStructureSpacing
    );

    return true;
}

void USGConstructionComponent::ClearSelectedStructure()
{
    SelectedStructureRow =
        NAME_None;

    SelectedStructureClass =
        nullptr;

    SelectedPreviewClass =
        nullptr;

    SelectedResourceCosts.Empty();

    bSelectedRequiresPower =
        false;

    SelectedMaxSlopeDegrees =
        10.0f;

    SelectedPlacementExtent =
        FVector(50.0f, 50.0f, 50.0f);

    SelectedGroundOffset =
        0.0f;

    bSelectedRequireFullGroundSupport =
        true;

    SelectedSupportTraceStartHeight =
        20.0f;

    SelectedSupportTraceDepth =
        50.0f;

    SelectedStructureSpacing =
        10.0f;

    CurrentPreviewYaw =
        0.0f;

    DestroyPreviewActor();

    CurrentPlacementResult.Reset();

    CurrentPlacementResult.FailureReason =
        ESGPlacementFailureReason::InvalidDefinition;

    OnPreviewValidityChanged(
        false,
        ESGPlacementFailureReason::InvalidDefinition
    );

    OnPlacementResultChanged.Broadcast(
        CurrentPlacementResult
    );
}

void USGConstructionComponent::RotatePreview(
    const float RotationDirection
)
{
    if (!IsBuildModeActive())
    {
        return;
    }

    if (FMath::IsNearlyZero(
        RotationDirection
    ))
    {
        return;
    }

    const float Direction =
        RotationDirection > 0.0f
        ? 1.0f
        : -1.0f;

    CurrentPreviewYaw +=
        Direction * RotationStep;

    CurrentPreviewYaw =
        FMath::Fmod(
            CurrentPreviewYaw,
            360.0f
        );

    if (CurrentPreviewYaw < 0.0f)
    {
        CurrentPreviewYaw += 360.0f;
    }

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

    const UWorld* World =
        GetWorld();

    const AActor* OwnerActor =
        GetOwner();

    if (!World || !OwnerActor)
    {
        return false;
    }

    UCameraComponent* Camera =
        CachedCamera;

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
        TraceStart
        +
        Camera->GetForwardVector()
        *
        PlacementDistance;

    FCollisionQueryParams QueryParams(
        SCENE_QUERY_STAT(SGPlacementTrace),
        false,
        OwnerActor
    );

    if (IsValid(PreviewActor))
    {
        QueryParams.AddIgnoredActor(
            PreviewActor
        );
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

    if (SafeNormal.IsNearlyZero())
    {
        return false;
    }

    const float DotValue =
        FMath::Clamp(
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

    return SlopeDegrees <=
        SelectedMaxSlopeDegrees;
}

bool USGConstructionComponent::CheckStructureOverlap(
    const FTransform& PlacementTransform
) const
{
    const UWorld* World =
        GetWorld();

    const AActor* OwnerActor =
        GetOwner();

    if (!World || !OwnerActor)
    {
        return false;
    }

    FCollisionQueryParams QueryParams(
        SCENE_QUERY_STAT(SGStructureOverlap),
        false,
        OwnerActor
    );

    QueryParams.bFindInitialOverlaps = true;

    if (IsValid(PreviewActor))
    {
        QueryParams.AddIgnoredActor(
            PreviewActor
        );
    }

    /*
     * 설치 간격은 수평 방향에 추가한다.
     *
     * 데이터 테이블의 PlacementExtent와
     * 실제 구조물에 생성한 PlacementBounds를
     * 동일 기준으로 사용한다.
     */
    const FVector CheckExtent(
        FMath::Max(
            1.0f,
            SelectedPlacementExtent.X
                + SelectedStructureSpacing
        ),

        FMath::Max(
            1.0f,
            SelectedPlacementExtent.Y
                + SelectedStructureSpacing
        ),

        FMath::Max(
            1.0f,
            SelectedPlacementExtent.Z
                - PlacementCollisionTolerance
        )
    );

    /*
     * 설치 위치는 구조물 바닥 Pivot이다.
     *
     * 실제 설치 구조물의 PlacementBounds도
     * 동일하게 Extent.Z만큼 위에 생성된다.
     */
    const FVector CheckLocation =
        PlacementTransform.GetLocation()
        +
        PlacementTransform
            .GetRotation()
            .RotateVector(
                FVector(
                    0.0f,
                    0.0f,
                    SelectedPlacementExtent.Z
                )
            );

    FCollisionObjectQueryParams ObjectQueryParams;

    ObjectQueryParams.AddObjectTypesToQuery(
        StructureObjectChannel
    );

    const FCollisionShape CollisionShape =
        FCollisionShape::MakeBox(
            CheckExtent
        );

    TArray<FOverlapResult> OverlapResults;

    const bool bFoundOverlap =
        World->OverlapMultiByObjectType(
            OverlapResults,
            CheckLocation,
            PlacementTransform.GetRotation(),
            ObjectQueryParams,
            CollisionShape,
            QueryParams
        );

    if (!bFoundOverlap)
    {
        return true;
    }

    for (const FOverlapResult& OverlapResult :
        OverlapResults)
    {
        AActor* OverlappedActor =
            OverlapResult.GetActor();

        UPrimitiveComponent* OverlappedComponent =
            OverlapResult.GetComponent();

        if (!IsValid(OverlappedActor)
            ||
            !IsValid(OverlappedComponent))
        {
            continue;
        }

        if (OverlappedActor == OwnerActor
            ||
            OverlappedActor == PreviewActor)
        {
            continue;
        }

        if (OverlappedComponent
                ->GetCollisionObjectType()
            != StructureObjectChannel)
        {
            continue;
        }

        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "Structure overlap detected. "
                "SelectedRow=%s "
                "Actor=%s "
                "Component=%s "
                "CheckLocation=%s "
                "CheckExtent=%s"
            ),
            *SelectedStructureRow.ToString(),
            *GetNameSafe(OverlappedActor),
            *GetNameSafe(OverlappedComponent),
            *CheckLocation.ToString(),
            *CheckExtent.ToString()
        );

        return false;
    }

    return true;
}

bool USGConstructionComponent::CheckResources() const
{
    if (!IsValid(ResourceInventory))
    {
        /*
         * 자원 컴포넌트가 없는 테스트 캐릭터에서는
         * 비용이 없는 구조물만 설치할 수 있다.
         */
        return SelectedResourceCosts.IsEmpty();
    }

    return ResourceInventory->CanAffordCosts(
        SelectedResourceCosts
    );
}

bool USGConstructionComponent::
CheckPowerRequirement_Implementation(
    const FVector& PlacementLocation
) const
{
    /*
     * 비전력 구조물은 항상 통과한다.
     *
     * 전력 구조물은 PowerCore 시스템이 연결되기 전까지
     * 설치할 수 없다.
     */
    return !bSelectedRequiresPower;
}

bool USGConstructionComponent::CalculatePlacementResult(
    FSGPlacementResult& OutResult
) const
{
    OutResult.Reset();

    // ─────────────────────────────────────────────
    // 1. 구조물 정의 검사

    if (!SelectedStructureClass)
    {
        OutResult.FailureReason =
            ESGPlacementFailureReason::InvalidDefinition;

        return false;
    }

    // ─────────────────────────────────────────────
    // 2. 카메라 기반 중앙 표면 Trace

    FHitResult GroundHit;

    if (!CheckGroundTrace(
        GroundHit
    ))
    {
        OutResult.FailureReason =
            ESGPlacementFailureReason::NoSurface;

        return false;
    }

    OutResult.GroundHit =
        GroundHit;

    // ─────────────────────────────────────────────
    // 3. 설치 가능한 표면인지 검사

    if (!IsValidGroundSurface(
        GroundHit
    ))
    {
        OutResult.FailureReason =
            ESGPlacementFailureReason::InvalidSurface;

        return false;
    }

    // ─────────────────────────────────────────────
    // 4. 설치 Transform 계산

    const FVector PlacementLocation =
        GroundHit.ImpactPoint
        +
        GroundHit.ImpactNormal
        *
        SelectedGroundOffset;

    const FRotator PlacementRotation(
        0.0f,
        CurrentPreviewYaw,
        0.0f
    );

    OutResult.PlacementTransform =
        FTransform(
            PlacementRotation,
            PlacementLocation,
            FVector::OneVector
        );

    // ─────────────────────────────────────────────
    // 5. 중앙 표면 경사 검사

    if (!CheckSlope(
        GroundHit.ImpactNormal
    ))
    {
        OutResult.FailureReason =
            ESGPlacementFailureReason::TooSteep;

        return false;
    }

    // ─────────────────────────────────────────────
    // 6. 구조물 바닥 네 모서리 지지 검사

    if (!CheckFullGroundSupport(
        OutResult.PlacementTransform
    ))
    {
        OutResult.FailureReason =
            ESGPlacementFailureReason::Unsupported;

        return false;
    }

    // ─────────────────────────────────────────────
    // 7. 기존 구조물 중첩 검사

    if (!CheckStructureOverlap(
        OutResult.PlacementTransform
    ))
    {
        OutResult.FailureReason =
            ESGPlacementFailureReason::StructureOverlap;

        return false;
    }

    // ─────────────────────────────────────────────
    // 8. 자원 검사

    if (!CheckResources())
    {
        OutResult.FailureReason =
            ESGPlacementFailureReason::
                InsufficientResources;

        return false;
    }

    // ─────────────────────────────────────────────
    // 9. 전력 검사

    if (!CheckPowerRequirement(
        PlacementLocation
    ))
    {
        OutResult.FailureReason =
            ESGPlacementFailureReason::
                OutsidePowerRange;

        return false;
    }

    // ─────────────────────────────────────────────
    // 10. 설치 가능

    OutResult.bCanPlace = true;

    OutResult.FailureReason =
        ESGPlacementFailureReason::None;

    return true;
}

void USGConstructionComponent::UpdatePlacementPreview()
{
    if (!IsBuildModeActive())
    {
        return;
    }

    FSGPlacementResult NewResult;

    CalculatePlacementResult(
        NewResult
    );

    const bool bValidityChanged =
        NewResult.bCanPlace
            != CurrentPlacementResult.bCanPlace
        ||
        NewResult.FailureReason
            != CurrentPlacementResult.FailureReason;

    CurrentPlacementResult =
        NewResult;

    /*
     * 유효한 바닥 Transform이 계산된 경우에만
     * 프리뷰 위치를 변경한다.
     */
    if (IsValid(PreviewActor)
        &&
        CurrentPlacementResult.FailureReason !=
            ESGPlacementFailureReason::NoSurface
        &&
        CurrentPlacementResult.FailureReason !=
            ESGPlacementFailureReason::InvalidDefinition)
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
    UWorld* World =
        GetWorld();

    if (!World || !SelectedPreviewClass)
    {
        return;
    }

    DestroyPreviewActor();

    FActorSpawnParameters SpawnParameters;

    SpawnParameters.Owner =
        GetOwner();

    if (APawn* OwnerPawn =
        Cast<APawn>(GetOwner()))
    {
        SpawnParameters.Instigator =
            OwnerPawn;
    }

    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    PreviewActor =
        World->SpawnActor<AActor>(
            SelectedPreviewClass,
            FTransform::Identity,
            SpawnParameters
        );

    if (!IsValid(PreviewActor))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "Failed to spawn preview actor for Row: %s"
            ),
            *SelectedStructureRow.ToString()
        );

        return;
    }

    ConfigurePreviewActor(
        PreviewActor
    );
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

    for (UPrimitiveComponent* Primitive :
        PrimitiveComponents)
    {
        if (!IsValid(Primitive))
        {
            continue;
        }

        Primitive->SetCollisionEnabled(
            ECollisionEnabled::NoCollision
        );

        Primitive->SetGenerateOverlapEvents(
            false
        );

        Primitive->SetCanEverAffectNavigation(
            false
        );
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

    if (!CalculatePlacementResult(
        PlacementResult
    ))
    {
        CurrentPlacementResult =
            PlacementResult;

        OnPreviewValidityChanged(
            false,
            PlacementResult.FailureReason
        );

        OnPlacementResultChanged.Broadcast(
            CurrentPlacementResult
        );

        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "Structure placement failed. "
                "Row=%s FailureReason=%d"
            ),
            *SelectedStructureRow.ToString(),
            static_cast<int32>(
                PlacementResult.FailureReason
            )
        );

        return false;
    }

    UWorld* World =
        GetWorld();

    if (!World || !SelectedStructureClass)
    {
        return false;
    }

    FActorSpawnParameters SpawnParameters;

    SpawnParameters.Owner =
        GetOwner();

    if (APawn* OwnerPawn =
        Cast<APawn>(GetOwner()))
    {
        SpawnParameters.Instigator =
            OwnerPawn;
    }

    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AActor* SpawnedStructure =
        World->SpawnActor<AActor>(
            SelectedStructureClass,
            PlacementResult.PlacementTransform,
            SpawnParameters
        );

    if (!IsValid(SpawnedStructure))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "Failed to spawn structure: %s"
            ),
            *SelectedStructureRow.ToString()
        );

        return false;
    }

    /*
     * 중요:
     *
     * 실제 메시 Collision과 관계없이
     * 모든 설치 구조물에 중첩 판정 전용 Box를 생성한다.
     */
    CreatePlacementBoundsForStructure(
        SpawnedStructure
    );

    /*
     * 비용이 존재할 때만 차감한다.
     */
    if (!SelectedResourceCosts.IsEmpty())
    {
        if (!IsValid(ResourceInventory))
        {
            SpawnedStructure->Destroy();

            UE_LOG(
                LogTemp,
                Warning,
                TEXT(
                    "Structure placement failed: "
                    "Resource inventory is missing. "
                    "Row=%s"
                ),
                *SelectedStructureRow.ToString()
            );

            return false;
        }

        if (!ResourceInventory->ConsumeCosts(
            SelectedResourceCosts
        ))
        {
            SpawnedStructure->Destroy();

            UE_LOG(
                LogTemp,
                Warning,
                TEXT(
                    "Failed to consume "
                    "structure costs: %s"
                ),
                *SelectedStructureRow.ToString()
            );

            return false;
        }
    }

    OnStructurePlaced.Broadcast(
        SpawnedStructure
    );

    UE_LOG(
        LogTemp,
        Log,
        TEXT(
            "Structure placed: %s"
        ),
        *SelectedStructureRow.ToString()
    );

    /*
     * 새로 생성된 PlacementBounds가
     * 다음 프레임 중첩 검사에 즉시 포함된다.
     */
    UpdatePlacementPreview();

    return true;
}

bool USGConstructionComponent::IsValidGroundSurface(
    const FHitResult& GroundHit
) const
{
    if (!GroundHit.bBlockingHit)
    {
        return false;
    }

    const UPrimitiveComponent* HitComponent =
        GroundHit.GetComponent();

    if (!IsValid(HitComponent))
    {
        return false;
    }

    /*
     * MVP에서는 기존 구조물 위에
     * 다른 구조물을 설치할 수 없다.
     */
    if (!bAllowPlacementOnStructures
        &&
        HitComponent->GetCollisionObjectType()
            == StructureObjectChannel)
    {
        return false;
    }

    /*
     * 아래쪽을 향하는 표면은 천장으로 간주한다.
     */
    if (GroundHit.ImpactNormal.Z <= 0.0f)
    {
        return false;
    }

    return true;
}

bool USGConstructionComponent::TraceSupportPoint(
    const FVector& WorldSupportPoint
) const
{
    const UWorld* World =
        GetWorld();

    const AActor* OwnerActor =
        GetOwner();

    if (!World || !OwnerActor)
    {
        return false;
    }

    const FVector TraceStart =
        WorldSupportPoint
        +
        FVector::UpVector
        *
        SelectedSupportTraceStartHeight;

    const FVector TraceEnd =
        WorldSupportPoint
        -
        FVector::UpVector
        *
        SelectedSupportTraceDepth;

    FCollisionQueryParams QueryParams(
        SCENE_QUERY_STAT(SGSupportTrace),
        false,
        OwnerActor
    );

    if (IsValid(PreviewActor))
    {
        QueryParams.AddIgnoredActor(
            PreviewActor
        );
    }

    FHitResult SupportHit;

    const bool bHit =
        World->LineTraceSingleByChannel(
            SupportHit,
            TraceStart,
            TraceEnd,
            PlacementTraceChannel,
            QueryParams
        );

    if (!bHit)
    {
        return false;
    }

    if (!IsValidGroundSurface(
        SupportHit
    ))
    {
        return false;
    }

    /*
     * 중앙뿐 아니라 각 모서리 표면도
     * 선택 구조물의 최대 경사 기준을 만족해야 한다.
     */
    return CheckSlope(
        SupportHit.ImpactNormal
    );
}

bool USGConstructionComponent::CheckFullGroundSupport(
    const FTransform& PlacementTransform
) const
{
    if (!bSelectedRequireFullGroundSupport)
    {
        return true;
    }

    /*
     * 정확한 Box 외곽 끝을 검사하면
     * StaticMesh 또는 지형 Collision 오차로
     * 정상적인 설치도 실패할 수 있다.
     *
     * 따라서 Extent의 90% 위치를 검사한다.
     */
    const float SupportX =
        SelectedPlacementExtent.X * 0.9f;

    const float SupportY =
        SelectedPlacementExtent.Y * 0.9f;

    const FVector LocalSupportPoints[] =
    {
        FVector(
            SupportX,
            SupportY,
            0.0f
        ),

        FVector(
            SupportX,
            -SupportY,
            0.0f
        ),

        FVector(
            -SupportX,
            SupportY,
            0.0f
        ),

        FVector(
            -SupportX,
            -SupportY,
            0.0f
        )
    };

    for (const FVector& LocalSupportPoint :
        LocalSupportPoints)
    {
        const FVector WorldSupportPoint =
            PlacementTransform.TransformPosition(
                LocalSupportPoint
            );

        if (!TraceSupportPoint(
            WorldSupportPoint
        ))
        {
            return false;
        }
    }

    return true;
}

void USGConstructionComponent::CreatePlacementBoundsForStructure(
    AActor* StructureActor
) const
{
    if (!IsValid(StructureActor))
    {
        return;
    }

    USceneComponent* RootComponent =
        StructureActor->GetRootComponent();

    if (!IsValid(RootComponent))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "Failed to create placement bounds. "
                "Actor has no RootComponent: %s"
            ),
            *GetNameSafe(StructureActor)
        );

        return;
    }

    /*
     * 동일 Actor에 중복 생성되는 것을 방지한다.
     */
    TArray<UBoxComponent*> ExistingBoxComponents;

    StructureActor->GetComponents<UBoxComponent>(
        ExistingBoxComponents
    );

    for (UBoxComponent* ExistingBox :
        ExistingBoxComponents)
    {
        if (IsValid(ExistingBox)
            &&
            ExistingBox->ComponentHasTag(
                TEXT("SGPlacementBounds")
            ))
        {
            return;
        }
    }

    /*
     * 실제 Mesh Collision과 독립적인
     * 건설 중첩 검사 전용 BoxComponent.
     */
    UBoxComponent* PlacementBounds =
        NewObject<UBoxComponent>(
            StructureActor,
            UBoxComponent::StaticClass(),
            TEXT("SGPlacementBounds")
        );

    if (!IsValid(PlacementBounds))
    {
        return;
    }

    PlacementBounds->ComponentTags.Add(
        TEXT("SGPlacementBounds")
    );

    PlacementBounds->SetupAttachment(
        RootComponent
    );

    /*
     * PlacementExtent는 Half Extent다.
     *
     * 현재 설치 Transform의 위치는
     * 구조물 바닥 Pivot 기준이므로,
     * Box 중심을 높이 절반만큼 위로 이동한다.
     */
    PlacementBounds->SetRelativeLocation(
        FVector(
            0.0f,
            0.0f,
            SelectedPlacementExtent.Z
        )
    );

    PlacementBounds->SetRelativeRotation(
        FRotator::ZeroRotator
    );

    PlacementBounds->SetBoxExtent(
        SelectedPlacementExtent,
        false
    );

    /*
     * 물리 충돌에는 사용하지 않고
     * 건설 Query에서만 사용한다.
     */
    PlacementBounds->SetCollisionEnabled(
        ECollisionEnabled::QueryOnly
    );

    PlacementBounds->SetCollisionObjectType(
        StructureObjectChannel
    );

    PlacementBounds->SetCollisionResponseToAllChannels(
        ECR_Ignore
    );

    PlacementBounds->SetCollisionResponseToChannel(
        StructureObjectChannel,
        ECR_Overlap
    );

    PlacementBounds->SetGenerateOverlapEvents(
        false
    );

    PlacementBounds->SetCanEverAffectNavigation(
        false
    );

    PlacementBounds->SetHiddenInGame(
        true
    );

    PlacementBounds->RegisterComponent();

    UE_LOG(
        LogTemp,
        Log,
        TEXT(
            "Placement bounds created. "
            "Actor=%s Extent=%s RelativeLocation=%s"
        ),
        *GetNameSafe(StructureActor),
        *SelectedPlacementExtent.ToString(),
        *PlacementBounds
            ->GetRelativeLocation()
            .ToString()
    );
}