#include "USGConstructionComponent.h"

#include "../Structures/Base/StructureDefinition.h"
#include "../Structures/Base/StructureBase.h"
#include "../Structures/Power/PowerCore.h"

#include "Camera/CameraComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

#include "USGResourceInventoryComponent.h"

namespace
{
    bool HasPlacementResultChanged(
        const FSGPlacementResult& Left,
        const FSGPlacementResult& Right
    )
    {
        if (Left.bCanPlace != Right.bCanPlace
            || Left.bHasValidTransform != Right.bHasValidTransform
            || Left.FailureReason != Right.FailureReason)
        {
            return true;
        }

        return Left.bHasValidTransform
            && !Left.PlacementTransform.Equals(
                Right.PlacementTransform,
                0.1f
            );
    }
}

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

void USGConstructionComponent::EndPlay(
    const EEndPlayReason::Type EndPlayReason
)
{
    /*
     * 컴포넌트와 소유 Actor가 종료될 때
     * 캐시에 저장한 프리뷰를 모두 정리한다.
     */
    DestroyAllPreviewActors();

    Super::EndPlay(EndPlayReason);
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

    if (PreviewUpdateInterval > 0.0f)
    {
        PreviewUpdateAccumulator += DeltaTime;

        if (PreviewUpdateAccumulator < PreviewUpdateInterval)
        {
            return;
        }

        PreviewUpdateAccumulator = FMath::Fmod(
            PreviewUpdateAccumulator,
            PreviewUpdateInterval
        );
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
    /*
     * 이미 건설 모드이거나 상태 변경 중이면
     * 중복 실행하지 않는다.
     */
    if (IsBuildModeActive()
        || bIsChangingBuildMode)
    {
        return;
    }

    bIsChangingBuildMode = true;

    /*
     * 선택된 구조물이 없으면
     * 기본 구조물을 자동으로 선택한다.
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

    PreviewUpdateAccumulator = 0.0f;

    /*
     * 현재 선택된 구조물의 프리뷰를 가져온다.
     *
     * 이미 캐시에 있으면 재사용하고,
     * 없을 때만 새로 생성한다.
     */
    if (SelectedPreviewClass
        &&
        !SelectedStructureRow.IsNone())
    {
        SpawnPreviewActor();
    }

    if (IsValid(PreviewActor))
    {
        SetPreviewActorActive(
            PreviewActor,
            true
        );
    }

    /*
     * 건설 모드 진입 즉시
     * 프리뷰 위치와 설치 가능 여부를 계산한다.
     */
    UpdatePlacementPreview();

    SetComponentTickEnabled(true);

    OnBuildModeEntered();

    OnBuildModeChanged.Broadcast(true);

    UE_LOG(
        LogTemp,
        Log,
        TEXT(
            "Construction component entered build mode. "
            "SelectedRow=%s Preview=%s"
        ),
        *SelectedStructureRow.ToString(),
        *GetNameSafe(PreviewActor)
    );

    bIsChangingBuildMode = false;
}

void USGConstructionComponent::ExitBuildMode()
{
    /*
     * 이미 종료됐거나 상태 변경 중이면
     * 중복 실행하지 않는다.
     */
    if (!IsBuildModeActive()
        || bIsChangingBuildMode)
    {
        return;
    }

    bIsChangingBuildMode = true;

    BuildModeState =
        ESGBuildModeState::Inactive;

    PreviewUpdateAccumulator = 0.0f;

    SetComponentTickEnabled(false);

    /*
     * 현재 프리뷰는 파괴하지 않고 숨긴다.
     * 다음 건설 모드 진입 시 다시 사용한다.
     */
    if (IsValid(PreviewActor))
    {
        SetPreviewActorActive(
            PreviewActor,
            false
        );
    }

    CurrentPlacementResult.Reset();

    OnBuildModeExited();

    OnBuildModeChanged.Broadcast(false);

    UE_LOG(
        LogTemp,
        Verbose,
        TEXT(
            "Construction component exited build mode. "
            "Preview cached=%s"
        ),
        IsValid(PreviewActor)
            ? TEXT("true")
            : TEXT("false")
    );

    bIsChangingBuildMode = false;
}

void USGConstructionComponent::ToggleBuildMode()
{
    /*
     * Enter/Exit의 Blueprint Event 또는 Delegate 처리 도중
     * 다시 Toggle이 호출되는 것을 막는다.
     */
    if (bIsChangingBuildMode)
    {
        return;
    }

    UWorld* World = GetWorld();

    if (!World)
    {
        return;
    }

    const float CurrentTime =
        World->GetRealTimeSeconds();

    /*
     * 같은 프레임 또는 지나치게 짧은 간격으로 들어온
     * 반복 Toggle 입력을 차단한다.
     */
    if (LastBuildModeToggleTime >= 0.0f
        &&
        CurrentTime - LastBuildModeToggleTime
            < BuildModeToggleInterval)
    {
        return;
    }

    LastBuildModeToggleTime =
        CurrentTime;

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
     * 킬존의 시작점은 파워코어다.
     * 코어가 설치되기 전에는 다른 구조물의
     * 선택·프리뷰 생성 자체를 차단한다.
     */
    if (StructureRowName != PowerCoreStructureRowName
        && !HasOperationalPowerCore())
    {
        UE_LOG(
            LogTemp,
            Verbose,
            TEXT(
                "Structure selection locked until "
                "PowerCore is placed. Row=%s"
            ),
            *StructureRowName.ToString()
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

    /*
     * 실제 StructureClass를 프리뷰로 사용하면 BeginPlay, Timer,
     * 전력망, 터렛 및 함정 로직이 실행될 수 있다.
     * 따라서 전용 PreviewClass가 없는 Row는 선택하지 않는다.
     */
    if (!Definition.PreviewClass)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "Structure Row '%s' has no PreviewClass. "
                "A dedicated preview class is required."
            ),
            *StructureRowName.ToString()
        );

        return false;
    }

    if (Definition.PreviewClass->IsChildOf(
        AStructureBase::StaticClass()
    ))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "Structure Row '%s' uses a StructureBase child as "
                "PreviewClass. Use a lightweight AActor preview BP."
            ),
            *StructureRowName.ToString()
        );

        return false;
    }

    /*
     * 현재 사용 중인 프리뷰를 파괴하지 않고 숨긴다.
     * 해당 프리뷰는 PreviewActorCache에 남아 있다.
     */
    if (IsValid(PreviewActor))
    {
        SetPreviewActorActive(
            PreviewActor,
            false
        );
    }

    PreviewActor = nullptr;

    SelectedStructureRow =
        StructureRowName;

    SelectedStructureClass =
        Definition.StructureClass;

    SelectedPreviewClass = Definition.PreviewClass;

    SelectedResourceCosts =
        Definition.BuildCosts;

    bSelectedRequiresPower =
        Definition.PowerData.bRequiresPower;

    bSelectedRequiresPowerCore =
        Definition.PowerData.bRequiresPowerCore;

    SelectedPowerConsumption =
        FMath::Max(
            0.0f,
            Definition.PowerData.PowerConsumption
        );

    SelectedMaxInstallCount =
        FMath::Max(
            1,
            Definition.MaxInstallCount
        );

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

    CurrentPreviewYaw =
        0.0f;

    CurrentPlacementResult.Reset();

    /*
     * 건설 모드일 때만 프리뷰를 활성화한다.
     *
     * 같은 Row의 프리뷰가 캐시에 존재하면 재사용하고,
     * 최초 선택일 때만 새로 Spawn한다.
     */
    if (IsBuildModeActive()
        &&
        SelectedPreviewClass)
    {
        SpawnPreviewActor();

        UpdatePlacementPreview();
    }

    OnStructureSelected.Broadcast(
        SelectedStructureRow
    );

    UE_LOG(
        LogTemp,
        Verbose,
        TEXT(
            "Structure selected from DataTable: "
            "Row=%s ID=%s RequiresPowerCore=%s "
            "RequiresPower=%s PowerConsumption=%.1f "
            "MaxInstallCount=%d "
            "MaxSlope=%.1f Extent=%s "
            "FullSupport=%s SupportDepth=%.1f "
            "Spacing=%.1f"
        ),
        *SelectedStructureRow.ToString(),
        *Definition.StructureID.ToString(),
        bSelectedRequiresPowerCore
            ? TEXT("true")
            : TEXT("false"),
        bSelectedRequiresPower
            ? TEXT("true")
            : TEXT("false"),
        SelectedPowerConsumption,
        SelectedMaxInstallCount,
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
    /*
     * 선택 구조물을 완전히 초기화하는 경우에는
     * 캐시된 모든 프리뷰도 제거한다.
     */
    DestroyAllPreviewActors();

    SelectedStructureRow =
        NAME_None;

    SelectedStructureClass =
        nullptr;

    SelectedPreviewClass =
        nullptr;

    SelectedResourceCosts.Empty();

    bSelectedRequiresPower =
        false;

    bSelectedRequiresPowerCore =
        false;

    SelectedPowerConsumption =
        0.0f;

    SelectedMaxInstallCount =
        1;

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

    FSGPlacementResult ClearedResult;
    ClearedResult.Reset();
    ClearedResult.FailureReason =
        ESGPlacementFailureReason::InvalidDefinition;

    const bool bResultChanged =
        HasPlacementResultChanged(
            ClearedResult,
            CurrentPlacementResult
        );

    CurrentPlacementResult = ClearedResult;

    OnPreviewValidityChanged(
        false,
        ESGPlacementFailureReason::InvalidDefinition
    );

    if (bResultChanged)
    {
        OnPlacementResultChanged.Broadcast(
            CurrentPlacementResult
        );
    }
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

    /*
     * 중앙 Trace는 설치 지면을 찾는 용도다.
     * 기존 구조물과의 충돌은 CheckStructureOverlap에서 별도로
     * 검사하므로, 여기서는 설치된 구조물을 무시한다.
     */
    for (const TObjectPtr<AActor>& StructurePtr : PlacedStructures)
    {
        if (IsValid(StructurePtr.Get()))
        {
            QueryParams.AddIgnoredActor(StructurePtr.Get());
        }
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

int32 USGConstructionComponent::GetPlacedStructureCount(
    const FName StructureRowName
) const
{
    if (StructureRowName.IsNone())
    {
        return 0;
    }

    int32 Count = 0;

    for (const TPair<TObjectPtr<AActor>, FName>& Pair :
        PlacedStructureRows)
    {
        if (IsValid(Pair.Key.Get())
            && Pair.Value == StructureRowName)
        {
            ++Count;
        }
    }

    return Count;
}

int32 USGConstructionComponent::
GetSelectedStructureRemainingCount() const
{
    if (SelectedStructureRow.IsNone())
    {
        return 0;
    }

    return FMath::Max(
        0,
        SelectedMaxInstallCount
            - GetPlacedStructureCount(
                SelectedStructureRow
            )
    );
}

bool USGConstructionComponent::CheckStructureLimit() const
{
    if (SelectedStructureRow.IsNone())
    {
        return false;
    }

    return GetPlacedStructureCount(
        SelectedStructureRow
    ) < SelectedMaxInstallCount;
}

void USGConstructionComponent::RegisterPlacedStructure(
    AActor* StructureActor,
    const FName StructureRowName
)
{
    if (!IsValid(StructureActor)
        || StructureRowName.IsNone())
    {
        return;
    }

    CompactPlacedStructures();

    if (!PlacedStructures.Contains(StructureActor))
    {
        PlacedStructures.Add(StructureActor);
    }

    PlacedStructureRows.Add(
        StructureActor,
        StructureRowName
    );

    StructureActor->OnDestroyed.RemoveDynamic(
        this,
        &USGConstructionComponent::
            HandlePlacedStructureDestroyed
    );

    StructureActor->OnDestroyed.AddDynamic(
        this,
        &USGConstructionComponent::
            HandlePlacedStructureDestroyed
    );
}

void USGConstructionComponent::
HandlePlacedStructureDestroyed(
    AActor* DestroyedActor
)
{
    if (!DestroyedActor)
    {
        return;
    }

    const FName DestroyedStructureRow =
        PlacedStructureRows.FindRef(DestroyedActor);

    PlacedStructures.Remove(DestroyedActor);
    PlacedStructureRows.Remove(DestroyedActor);

    /*
     * 파워코어가 파괴되면 다른 구조물 선택을 유지하지 않고
     * 코어로 돌아간다. 이후 2~5번 선택은 다시 잠긴다.
     */
    if (DestroyedStructureRow == PowerCoreStructureRowName
        && !HasOperationalPowerCore())
    {
        DisableAllOwnedNonCoreStructures();

        if (SelectedStructureRow != PowerCoreStructureRowName)
        {
            SelectStructureByRowName(
                PowerCoreStructureRowName
            );
        }
    }
    else
    {
        /*
         * 남은 코어가 있거나 전력 소비 구조물이 파괴된 경우
         * 반환된 전력까지 포함해 전체 전력망을 다시 분배한다.
         */
        RecalculateOwnedPowerGrids();
    }

    if (IsBuildModeActive())
    {
        UpdatePlacementPreview();
    }
}

void USGConstructionComponent::CompactPlacedStructures()
{
    for (int32 Index = PlacedStructures.Num() - 1;
        Index >= 0;
        --Index)
    {
        AActor* StructureActor =
            PlacedStructures[Index].Get();

        if (IsValid(StructureActor))
        {
            continue;
        }

        PlacedStructureRows.Remove(StructureActor);
        PlacedStructures.RemoveAtSwap(Index);
    }

    for (auto Iterator = PlacedStructureRows.CreateIterator();
        Iterator;
        ++Iterator)
    {
        if (!IsValid(Iterator.Key().Get()))
        {
            Iterator.RemoveCurrent();
        }
    }
}

void USGConstructionComponent::GetOwnedPowerCores(
    TArray<APowerCore*>& OutPowerCores
) const
{
    OutPowerCores.Reset();

    const AActor* OwnerActor = GetOwner();

    for (const TObjectPtr<AActor>& StructurePtr :
        PlacedStructures)
    {
        APowerCore* PowerCore =
            Cast<APowerCore>(StructurePtr.Get());

        if (!IsValid(PowerCore))
        {
            continue;
        }

        if (PowerCore->GetOwner() != OwnerActor)
        {
            continue;
        }

        OutPowerCores.Add(PowerCore);
    }
}

bool USGConstructionComponent::HasOperationalPowerCore() const
{
    TArray<APowerCore*> PowerCores;
    GetOwnedPowerCores(PowerCores);

    for (const APowerCore* PowerCore : PowerCores)
    {
        if (IsValid(PowerCore)
            && !PowerCore->IsDestroyed()
            && PowerCore->CanOperate())
        {
            return true;
        }
    }

    return false;
}

void USGConstructionComponent::
DisableAllOwnedNonCoreStructures() const
{
    for (const TObjectPtr<AActor>& StructurePtr :
        PlacedStructures)
    {
        AStructureBase* Structure =
            Cast<AStructureBase>(StructurePtr.Get());

        if (!IsValid(Structure)
            || Structure->IsDestroyed()
            || Cast<APowerCore>(Structure))
        {
            continue;
        }

        Structure->SetPowered(false);
    }
}

APowerCore* USGConstructionComponent::
FindPowerCoreForPlacement(
    const FVector& PlacementLocation,
    bool& bOutFoundCoreInRange
) const
{
    bOutFoundCoreInRange = false;

    TArray<APowerCore*> PowerCores;
    GetOwnedPowerCores(PowerCores);

    APowerCore* BestPowerCore = nullptr;
    float BestRemainingPower = -1.0f;

    for (APowerCore* PowerCore : PowerCores)
    {
        if (!IsValid(PowerCore)
            || !PowerCore->IsInsidePowerRange(
                PlacementLocation
            ))
        {
            continue;
        }

        bOutFoundCoreInRange = true;

        const float RemainingPower =
            PowerCore->GetRemainingPower();

        if (RemainingPower + KINDA_SMALL_NUMBER
                < SelectedPowerConsumption)
        {
            continue;
        }

        if (!BestPowerCore
            || RemainingPower > BestRemainingPower)
        {
            BestPowerCore = PowerCore;
            BestRemainingPower = RemainingPower;
        }
    }

    return BestPowerCore;
}

ESGPlacementFailureReason
USGConstructionComponent::GetPowerFailureReason(
    const FVector& PlacementLocation
) const
{
    /*
     * 모든 비코어 구조물에 적용되는 최상위 선행조건.
     * CheckPowerRequirement를 직접 호출해도 동일하게 차단된다.
     */
    if (SelectedStructureRow != PowerCoreStructureRowName
        && !HasOperationalPowerCore())
    {
        return ESGPlacementFailureReason::MissingPowerCore;
    }

    if (!bSelectedRequiresPowerCore
        && !bSelectedRequiresPower)
    {
        return ESGPlacementFailureReason::None;
    }

    /*
     * 코어 존재만 요구하고 실제 전력은 사용하지 않는 구조물.
     */
    if (!bSelectedRequiresPower)
    {
        return ESGPlacementFailureReason::None;
    }

    bool bFoundCoreInRange = false;

    if (FindPowerCoreForPlacement(
        PlacementLocation,
        bFoundCoreInRange
    ))
    {
        return ESGPlacementFailureReason::None;
    }

    return bFoundCoreInRange
        ? ESGPlacementFailureReason::InsufficientPower
        : ESGPlacementFailureReason::OutsidePowerRange;
}

void USGConstructionComponent::
RecalculateOwnedPowerGrids() const
{
    TArray<APowerCore*> PowerCores;
    GetOwnedPowerCores(PowerCores);

    /*
     * RecalculatePowerGrid() 한 번이 같은 Owner의 모든 코어를
     * 함께 계산하므로, 코어마다 중복 호출하지 않는다.
     */
    for (APowerCore* PowerCore : PowerCores)
    {
        if (IsValid(PowerCore)
            && !PowerCore->IsDestroyed()
            && PowerCore->CanOperate())
        {
            PowerCore->RecalculatePowerGrid();
            return;
        }
    }

    DisableAllOwnedNonCoreStructures();
}

bool USGConstructionComponent::
CheckPowerRequirement_Implementation(
    const FVector& PlacementLocation
) const
{
    /*
     * 기본 전력 판정은 CalculatePlacementResult에서 한 번만 수행한다.
     * 이 이벤트는 Blueprint가 추가 규칙을 확장할 때만 사용한다.
     */
    (void)PlacementLocation;
    return true;
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
    // 3. 설치 Transform 계산

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

    OutResult.bHasValidTransform = true;

    // ─────────────────────────────────────────────
    // 4. 설치 개수 제한

    if (!CheckStructureLimit())
    {
        OutResult.FailureReason =
            ESGPlacementFailureReason::StructureLimit;

        return false;
    }

    // ─────────────────────────────────────────────
    // 5. 설치 가능한 표면인지 검사

    if (!IsValidGroundSurface(
        GroundHit
    ))
    {
        OutResult.FailureReason =
            ESGPlacementFailureReason::InvalidSurface;

        return false;
    }

    // ─────────────────────────────────────────────
    // 6. 중앙 표면 경사 검사

    if (!CheckSlope(
        GroundHit.ImpactNormal
    ))
    {
        OutResult.FailureReason =
            ESGPlacementFailureReason::TooSteep;

        return false;
    }

    // ─────────────────────────────────────────────
    // 7. 구조물 바닥 네 모서리 지지 검사

    if (!CheckFullGroundSupport(
        OutResult.PlacementTransform
    ))
    {
        OutResult.FailureReason =
            ESGPlacementFailureReason::Unsupported;

        return false;
    }

    // ─────────────────────────────────────────────
    // 8. 기존 구조물 중첩 검사

    if (!CheckStructureOverlap(
        OutResult.PlacementTransform
    ))
    {
        OutResult.FailureReason =
            ESGPlacementFailureReason::StructureOverlap;

        return false;
    }

    // ─────────────────────────────────────────────
    // 9. 자원 검사

    if (!CheckResources())
    {
        OutResult.FailureReason =
            ESGPlacementFailureReason::
                InsufficientResources;

        return false;
    }

    // ─────────────────────────────────────────────
    // 10. 전력 검사

    const ESGPlacementFailureReason
        PowerFailureReason =
            GetPowerFailureReason(
                PlacementLocation
            );

    if (PowerFailureReason !=
        ESGPlacementFailureReason::None)
    {
        OutResult.FailureReason =
            PowerFailureReason;

        return false;
    }

    /*
     * 기본 C++ 전력 검사를 반복하지 않고 Blueprint 추가 규칙만
     * 한 번 호출한다. 기본 구현은 true다.
     */
    if (!CheckPowerRequirement(PlacementLocation))
    {
        OutResult.FailureReason =
            ESGPlacementFailureReason::InsufficientPower;

        return false;
    }

    // ─────────────────────────────────────────────
    // 11. 설치 가능

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

    const bool bResultChanged =
        HasPlacementResultChanged(
            NewResult,
            CurrentPlacementResult
        );

    CurrentPlacementResult =
        NewResult;

    /*
     * Trace로 계산된 Transform만 적용한다.
     * 실패 결과의 Identity Transform이 월드 원점으로 프리뷰를
     * 이동시키는 현상을 방지한다.
     */
    if (IsValid(PreviewActor)
        && CurrentPlacementResult.bHasValidTransform
        && !PreviewActor->GetActorTransform().Equals(
            CurrentPlacementResult.PlacementTransform,
            0.1f
        ))
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

    if (bResultChanged)
    {
        OnPlacementResultChanged.Broadcast(
            CurrentPlacementResult
        );
    }
}

void USGConstructionComponent::SpawnPreviewActor()
{
    UWorld* World =
        GetWorld();

    if (!World
        ||
        !SelectedPreviewClass
        ||
        SelectedStructureRow.IsNone())
    {
        return;
    }

    /*
     * 현재 프리뷰가 이미 유효하다면
     * 다시 생성하지 않고 활성화한다.
     */
    if (IsValid(PreviewActor))
    {
        SetPreviewActorActive(
            PreviewActor,
            true
        );

        return;
    }

    /*
     * 현재 선택된 Row의 프리뷰가 캐시에 있는지 확인한다.
     */
    if (TObjectPtr<AActor>* CachedPreview =
        PreviewActorCache.Find(
            SelectedStructureRow
        ))
    {
        if (IsValid(CachedPreview->Get()))
        {
            PreviewActor =
                CachedPreview->Get();

            SetPreviewActorActive(
                PreviewActor,
                true
            );

            return;
        }

        /*
         * 외부에서 파괴된 Actor가 캐시에 남아 있다면
         * 잘못된 항목을 제거한다.
         */
        PreviewActorCache.Remove(
            SelectedStructureRow
        );
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

    PreviewActor =
        World->SpawnActor<AActor>(
            SelectedPreviewClass,
            FTransform::Identity,
            SpawnParameters
        );

    if (!IsValid(PreviewActor))
    {
        PreviewActor = nullptr;

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

    /*
     * 최초 생성된 프리뷰를 현재 Row의 캐시에 저장한다.
     */
    PreviewActorCache.Add(
        SelectedStructureRow,
        PreviewActor
    );

    SetPreviewActorActive(
        PreviewActor,
        true
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

    TArray<UActorComponent*> ActorComponents;

    InPreviewActor->GetComponents<UActorComponent>(
        ActorComponents
    );

    for (UActorComponent* ActorComponent : ActorComponents)
    {
        if (IsValid(ActorComponent))
        {
            ActorComponent->SetComponentTickEnabled(false);
        }
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

        Primitive->SetComponentTickEnabled(false);
    }

    InPreviewActor->SetActorTickEnabled(false);
}
void USGConstructionComponent::SetPreviewActorActive(
    AActor* InPreviewActor,
    const bool bActive
) const
{
    if (!IsValid(InPreviewActor))
    {
        return;
    }

    InPreviewActor->SetActorHiddenInGame(
        !bActive
    );

    /*
     * 프리뷰는 활성 상태와 관계없이
     * 실제 설치 충돌을 발생시키면 안 된다.
     */
    InPreviewActor->SetActorEnableCollision(
        false
    );

    /*
     * 프리뷰 위치는 ConstructionComponent가 갱신한다.
     * 활성 프리뷰도 자체 Actor Tick을 사용하지 않는다.
     */
    InPreviewActor->SetActorTickEnabled(false);
}

void USGConstructionComponent::DestroyAllPreviewActors()
{
    /*
     * Row별로 저장된 프리뷰를 전부 파괴한다.
     */
    for (
        TPair<FName, TObjectPtr<AActor>>& PreviewPair
        : PreviewActorCache
    )
    {
        AActor* CachedPreview =
            PreviewPair.Value.Get();

        if (IsValid(CachedPreview))
        {
            CachedPreview->Destroy();
        }
    }

    PreviewActorCache.Empty();

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
        const bool bResultChanged =
            HasPlacementResultChanged(
                PlacementResult,
                CurrentPlacementResult
            );

        CurrentPlacementResult =
            PlacementResult;

        OnPreviewValidityChanged(
            false,
            PlacementResult.FailureReason
        );

        if (bResultChanged)
        {
            OnPlacementResultChanged.Broadcast(
                CurrentPlacementResult
            );
        }

        UE_LOG(
            LogTemp,
            Verbose,
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

    /*
     * 비용 차감까지 성공한 구조물만 설치 목록에 등록한다.
     * 이후 파괴되면 HandlePlacedStructureDestroyed가 자동 호출된다.
     */
    RegisterPlacedStructure(
        SpawnedStructure,
        SelectedStructureRow
    );

    /*
     * 모든 비코어 구조물은 코어에 종속된다.
     * 전력을 소비하지 않는 구조물도 코어가 있어야 Active가 된다.
     */
    if (AStructureBase* Structure =
        Cast<AStructureBase>(SpawnedStructure))
    {
        if (SelectedStructureRow != PowerCoreStructureRowName
            && !bSelectedRequiresPower)
        {
            Structure->SetPowered(
                HasOperationalPowerCore()
            );
        }
    }

    /*
     * 파워코어 또는 전력 소비 구조물이 새로 생겼으므로
     * 같은 소유자의 전력망을 즉시 다시 계산한다.
     */
    if (SelectedStructureRow == PowerCoreStructureRowName
        || bSelectedRequiresPower)
    {
        RecalculateOwnedPowerGrids();
    }

    OnStructurePlaced.Broadcast(
        SpawnedStructure
    );

    UE_LOG(
        LogTemp,
        Verbose,
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
        VeryVerbose,
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
