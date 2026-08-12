#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "SpaceGround/CommonData/SGConstructionTypes.h"
#include "SpaceGround/CommonData/SGResourceTypes.h"

#include "USGConstructionComponent.generated.h"

class AActor;
class APowerCore;
class UCameraComponent;
class UDataTable;
class USGResourceInventoryComponent;

struct FSGStructureDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FSGOnBuildModeChanged,
    bool,
    bBuildModeActive
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FSGOnPlacementResultChanged,
    const FSGPlacementResult&,
    PlacementResult
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FSGOnStructurePlaced,
    AActor*,
    PlacedStructure
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FSGOnStructureSelected,
    FName,
    StructureRowName
);

/**
 * 플레이어의 건설 시스템을 담당하는 컴포넌트.
 *
 * 담당 기능:
 * - 건설 모드 진입 및 종료
 * - 구조물 DataTable 직접 조회
 * - 구조물 선택
 * - 동일 구조물 중복 선택 방지
 * - 프리뷰 생성 및 갱신
 * - 카메라 기반 설치 위치 계산
 * - 경사 검사
 * - 공중 설치 검사
 * - 구조물 중첩 검사
 * - 자원 및 전력 검사
 * - 실제 구조물 Spawn
 */
UCLASS(
    ClassGroup = (SpaceGround),
    BlueprintType,
    Blueprintable,
    meta = (BlueprintSpawnableComponent)
)
class SPACEGROUND_API USGConstructionComponent
    : public UActorComponent
{
    GENERATED_BODY()

public:
    USGConstructionComponent();

protected:
    virtual void BeginPlay() override;
    
    virtual void EndPlay(
    const EEndPlayReason::Type EndPlayReason
) override;

    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction
    ) override;

public:
    // ─────────────────────────────────────────────
    // 건설 모드

    UFUNCTION(BlueprintCallable, Category = "Construction")
    void EnterBuildMode();

    UFUNCTION(BlueprintCallable, Category = "Construction")
    void ExitBuildMode();

    UFUNCTION(BlueprintCallable, Category = "Construction")
    void ToggleBuildMode();

    UFUNCTION(BlueprintPure, Category = "Construction")
    bool IsBuildModeActive() const
    {
        return BuildModeState !=
            ESGBuildModeState::Inactive;
    }

public:
    // ─────────────────────────────────────────────
    // 구조물 선택

    /**
     * StructureDataTable에서 Row를 직접 조회해
     * 현재 선택 구조물로 설정한다.
     *
     * 이미 같은 Row가 선택되어 있다면
     * DataTable 재조회와 프리뷰 재생성을 생략한다.
     */
    UFUNCTION(
        BlueprintCallable,
        Category = "Construction|Data"
    )
    bool SelectStructureByRowName(
        FName StructureRowName
    );

    UFUNCTION(
        BlueprintCallable,
        Category = "Construction|Data"
    )
    void ClearSelectedStructure();

    UFUNCTION(
        BlueprintPure,
        Category = "Construction|Data"
    )
    bool HasStructureRow(
        FName StructureRowName
    ) const;

public:
    // ─────────────────────────────────────────────
    // 설치

    UFUNCTION(BlueprintCallable, Category = "Construction")
    bool TryPlaceSelectedStructure();

    /**
     * 양수는 오른쪽, 음수는 왼쪽으로 회전한다.
     *
     * 실제 회전량은 RotationStep을 사용한다.
     */
    UFUNCTION(BlueprintCallable, Category = "Construction")
    void RotatePreview(
        float RotationDirection
    );

    UFUNCTION(BlueprintCallable, Category = "Construction")
    void CancelPlacement();

public:
    // ─────────────────────────────────────────────
    // 조회

    UFUNCTION(BlueprintPure, Category = "Construction")
    const FSGPlacementResult& GetPlacementResult() const
    {
        return CurrentPlacementResult;
    }

    UFUNCTION(BlueprintPure, Category = "Construction")
    FName GetSelectedStructureRow() const
    {
        return SelectedStructureRow;
    }

    UFUNCTION(BlueprintPure, Category = "Construction")
    AActor* GetPreviewActor() const
    {
        return PreviewActor;
    }

    UFUNCTION(BlueprintPure, Category = "Construction")
    bool HasSelectedStructure() const
    {
        return SelectedStructureClass != nullptr;
    }

    UFUNCTION(BlueprintPure, Category = "Construction")
    bool DoesSelectedStructureRequirePower() const
    {
        return bSelectedRequiresPower;
    }

    UFUNCTION(BlueprintPure, Category = "Construction|Limit")
    int32 GetPlacedStructureCount(FName StructureRowName) const;

    UFUNCTION(BlueprintPure, Category = "Construction|Limit")
    int32 GetSelectedStructureRemainingCount() const;

protected:
    // ─────────────────────────────────────────────
    // 전력 판정

    /**
     * 선택된 구조물이 전력을 요구할 때
     * 설치 위치가 유효한 전력 범위인지 검사한다.
     */
    UFUNCTION(
        BlueprintNativeEvent,
        Category = "Construction|Power"
    )
    bool CheckPowerRequirement(
        const FVector& PlacementLocation
    ) const;

    virtual bool CheckPowerRequirement_Implementation(
        const FVector& PlacementLocation
    ) const;

protected:
    // ─────────────────────────────────────────────
    // Blueprint 이벤트

    UFUNCTION(
        BlueprintImplementableEvent,
        Category = "Construction"
    )
    void OnPreviewValidityChanged(
        bool bCanPlace,
        ESGPlacementFailureReason FailureReason
    );

    UFUNCTION(
        BlueprintImplementableEvent,
        Category = "Construction"
    )
    void OnBuildModeEntered();

    UFUNCTION(
        BlueprintImplementableEvent,
        Category = "Construction"
    )
    void OnBuildModeExited();

protected:
    // ─────────────────────────────────────────────
    // DataTable 설정

    /**
     * FSGStructureDefinition을 Row Struct로 사용하는
     * 구조물 DataTable.
     */
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Construction|Data",
        meta = (
            RequiredAssetDataTags =
            "RowStructure=/Script/SpaceGround.SGStructureDefinition"
        )
    )
    TObjectPtr<UDataTable> StructureDataTable;

    /**
     * 건설 모드 최초 진입 시 자동 선택할 구조물 Row.
     */
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Construction|Data"
    )
    FName DefaultStructureRowName =
        TEXT("PowerCore");

    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Construction|Data"
    )
    bool bAutoSelectDefaultStructure = true;

protected:
    // ─────────────────────────────────────────────
    // Trace 설정

    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Construction|Trace",
        meta = (
            ClampMin = "100.0",
            Units = "cm"
        )
    )
    float PlacementDistance = 600.0f;

    /**
     * 카메라 설치 Trace 및 지지 검사에 사용하는 채널.
     *
     * 기본값은 Visibility.
     */
    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Construction|Trace"
    )
    TEnumAsByte<ECollisionChannel>
    PlacementTraceChannel = ECC_Visibility;

protected:
    // ─────────────────────────────────────────────
    // 충돌 설정

    /**
     * Project Settings에서 생성한 Structure Object Channel.
     *
     * BP_SGPlayer의 ConstructionComponent에서
     * 반드시 Structure로 지정한다.
     */
    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Construction|Collision"
    )
    TEnumAsByte<ECollisionChannel>
    StructureObjectChannel = ECC_GameTraceChannel1;

    /**
     * 바닥과 정확히 맞닿는 구조물이
     * 자기 높이 때문에 중첩으로 오판되는 것을 방지한다.
     */
    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Construction|Collision",
        meta = (
            ClampMin = "0.0",
            Units = "cm"
        )
    )
    float PlacementCollisionTolerance = 2.0f;

    /**
     * 다른 구조물 위에 설치할 수 있는지 여부.
     *
     * 현재 MVP에서는 false로 사용한다.
     */
    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Construction|Collision"
    )
    bool bAllowPlacementOnStructures = false;

protected:
    // ─────────────────────────────────────────────
    // 회전 설정

    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Construction|Rotation",
        meta = (
            ClampMin = "1.0",
            ClampMax = "180.0",
            Units = "Degrees"
        )
    )
    float RotationStep = 15.0f;
    
    /**
 * 건설 모드 Toggle 최소 입력 간격.
 *
 * Preview Actor 생성/숨김, Blueprint 이벤트 등이
 * 한순간에 반복되는 것을 방지한다.
 */
    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Construction|Input",
        meta = (
            ClampMin = "0.0",
            ClampMax = "1.0",
            Units = "s"
        )
    )
    float BuildModeToggleInterval = 0.12f;

public:
    // ─────────────────────────────────────────────
    // Delegate

    UPROPERTY(BlueprintAssignable, Category = "Construction")
    FSGOnBuildModeChanged OnBuildModeChanged;

    UPROPERTY(BlueprintAssignable, Category = "Construction")
    FSGOnPlacementResultChanged OnPlacementResultChanged;

    UPROPERTY(BlueprintAssignable, Category = "Construction")
    FSGOnStructurePlaced OnStructurePlaced;

    UPROPERTY(BlueprintAssignable, Category = "Construction")
    FSGOnStructureSelected OnStructureSelected;

private:
    // ─────────────────────────────────────────────
    // 건설 상태

    UPROPERTY(
        VisibleInstanceOnly,
        Category = "Construction"
    )
    ESGBuildModeState BuildModeState =
        ESGBuildModeState::Inactive;

    /**
     * Enter/Exit의 Blueprint Event 또는 Delegate 실행 도중
     * 다시 Toggle이 호출되는 것을 차단한다.
     */
    bool bIsChangingBuildMode = false;

    /**
     * 마지막으로 ToggleBuildMode가 정상 처리된 시간.
     */
    float LastBuildModeToggleTime = -1.0f;

private:
    // ─────────────────────────────────────────────
    // 선택 구조물 데이터

    UPROPERTY(
        VisibleInstanceOnly,
        Category = "Construction|Selected"
    )
    FName SelectedStructureRow = NAME_None;

    UPROPERTY(
        VisibleInstanceOnly,
        Category = "Construction|Selected"
    )
    TSubclassOf<AActor> SelectedStructureClass;

    UPROPERTY(
        VisibleInstanceOnly,
        Category = "Construction|Selected"
    )
    TSubclassOf<AActor> SelectedPreviewClass;

    UPROPERTY(
        VisibleInstanceOnly,
        Category = "Construction|Selected"
    )
    TArray<FSGResourceCost> SelectedResourceCosts;

    UPROPERTY(
        VisibleInstanceOnly,
        Category = "Construction|Selected"
    )
    bool bSelectedRequiresPower = false;

    UPROPERTY(
        VisibleInstanceOnly,
        Category = "Construction|Selected"
    )
    bool bSelectedRequiresPowerCore = false;

    UPROPERTY(
        VisibleInstanceOnly,
        Category = "Construction|Selected"
    )
    float SelectedPowerConsumption = 0.0f;

    UPROPERTY(
        VisibleInstanceOnly,
        Category = "Construction|Selected"
    )
    int32 SelectedMaxInstallCount = 1;

    UPROPERTY(
        VisibleInstanceOnly,
        Category = "Construction|Selected"
    )
    float SelectedMaxSlopeDegrees = 10.0f;

    /**
     * 구조물 설치 충돌 검사에 사용하는 Box Half Extent.
     */
    UPROPERTY(
        VisibleInstanceOnly,
        Category = "Construction|Selected"
    )
    FVector SelectedPlacementExtent =
        FVector(50.0f, 50.0f, 50.0f);

    UPROPERTY(
        VisibleInstanceOnly,
        Category = "Construction|Selected"
    )
    float SelectedGroundOffset = 0.0f;

    UPROPERTY(
        VisibleInstanceOnly,
        Category = "Construction|Selected"
    )
    bool bSelectedRequireFullGroundSupport = true;

    UPROPERTY(
        VisibleInstanceOnly,
        Category = "Construction|Selected"
    )
    float SelectedSupportTraceStartHeight = 20.0f;

    UPROPERTY(
        VisibleInstanceOnly,
        Category = "Construction|Selected"
    )
    float SelectedSupportTraceDepth = 50.0f;

    UPROPERTY(
        VisibleInstanceOnly,
        Category = "Construction|Selected"
    )
    float SelectedStructureSpacing = 10.0f;

    UPROPERTY(
        VisibleInstanceOnly,
        Category = "Construction|Selected"
    )
    float CurrentPreviewYaw = 0.0f;

private:
    // 런타임 캐시

    UPROPERTY(Transient)
    TObjectPtr<AActor> PreviewActor;

    /**
     * 구조물마다 한 번 생성한 프리뷰를 저장한다.
     * 1~5번 전환 시 Destroy/Spawn하지 않고 재사용한다.
     */
    UPROPERTY(Transient)
    TMap<FName, TObjectPtr<AActor>>
    PreviewActorCache;

    UPROPERTY(Transient)
    TObjectPtr<UCameraComponent> CachedCamera;       

    UPROPERTY(Transient)
    TObjectPtr<USGResourceInventoryComponent>
    ResourceInventory;

    UPROPERTY(
        VisibleInstanceOnly,
        Category = "Construction"
    )
    FSGPlacementResult CurrentPlacementResult;

    /**
     * 이 컴포넌트를 통해 실제 설치된 구조물 목록.
     * 다른 플레이어가 설치한 구조물은 포함하지 않는다.
     */
    UPROPERTY(Transient)
    TArray<TObjectPtr<AActor>> PlacedStructures;

    /**
     * 설치 Actor와 원본 DataTable Row의 대응 관계.
     * MaxInstallCount 계산과 파괴 처리에 사용한다.
     */
    UPROPERTY(Transient)
    TMap<TObjectPtr<AActor>, FName> PlacedStructureRows;

private:
    // ─────────────────────────────────────────────
    // 내부 함수

    UCameraComponent* FindOwnerCamera() const;

    bool ApplyStructureDefinition(
        FName StructureRowName,
        const FSGStructureDefinition& Definition
    );

    void UpdatePlacementPreview();

    void SpawnPreviewActor();

    void SetPreviewActorActive(
        AActor* InPreviewActor,
        bool bActive
    ) const;

    void DestroyAllPreviewActors();

    void ConfigurePreviewActor(
        AActor* InPreviewActor
    ) const;

    bool CalculatePlacementResult(
        FSGPlacementResult& OutResult
    ) const;

    /**
     * 카메라에서 바라보는 위치를 찾는다.
     */
    bool CheckGroundTrace(
        FHitResult& OutGroundHit
    ) const;

    /**
     * 중앙 Trace 결과가 설치 가능한 표면인지 검사한다.
     */
    bool IsValidGroundSurface(
        const FHitResult& GroundHit
    ) const;

    /**
     * SurfaceNormal 기준 경사도를 검사한다.
     */
    bool CheckSlope(
        const FVector& SurfaceNormal
    ) const;

    /**
     * 구조물 바닥 모서리 한 지점의 지면 지지를 검사한다.
     */
    bool TraceSupportPoint(
        const FVector& WorldSupportPoint
    ) const;

    /**
     * 구조물 바닥 네 모서리가 모두 지면에 지지되는지 검사한다.
     */
    bool CheckFullGroundSupport(
        const FTransform& PlacementTransform
    ) const;

    /**
     * Structure Object Channel만 검사해
     * 기존 구조물과의 중첩 여부를 확인한다.
     */
    bool CheckStructureOverlap(
        const FTransform& PlacementTransform
    ) const;

    bool CheckResources() const;

    bool CheckStructureLimit() const;

    void RegisterPlacedStructure(
        AActor* StructureActor,
        FName StructureRowName
    );

    UFUNCTION()
    void HandlePlacedStructureDestroyed(
        AActor* DestroyedActor
    );

    void CompactPlacedStructures();

    void GetOwnedPowerCores(
        TArray<APowerCore*>& OutPowerCores
    ) const;

    APowerCore* FindPowerCoreForPlacement(
        const FVector& PlacementLocation,
        bool& bOutFoundCoreInRange
    ) const;

    ESGPlacementFailureReason GetPowerFailureReason(
        const FVector& PlacementLocation
    ) const;

    void RecalculateOwnedPowerGrids() const;
    
    /**
 * 실제 설치된 구조물에 건설 중첩 판정 전용 Box를 생성한다.
 *
 * StaticMesh 자체 Collision 유무와 관계없이
 * DataTable PlacementExtent를 기준으로 일관된 설치 판정을 제공한다.
 */
    void CreatePlacementBoundsForStructure(
        AActor* StructureActor
    ) const;
};
