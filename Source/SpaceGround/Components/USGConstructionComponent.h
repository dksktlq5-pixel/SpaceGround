#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Construction/SGConstructionTypes.h"
#include "../Resources/SGResourceTypes.h"
#include "USGConstructionComponent.generated.h"

class UCameraComponent;
class USGResourceInventoryComponent;

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

/**
 * 플레이어의 건설 모드, 프리뷰, 설치 판정을 담당한다.
 *
 * 현재 구조물 DataTable의 실제 Row Struct가 제공되지 않았으므로,
 * 구조물 선택 시 필요한 정보를 SetSelectedStructureDefinition으로
 * 전달받도록 구성한다.
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

    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction
    ) override;

public:
    UFUNCTION(BlueprintCallable, Category = "Construction")
    void EnterBuildMode();

    UFUNCTION(BlueprintCallable, Category = "Construction")
    void ExitBuildMode();

    UFUNCTION(BlueprintCallable, Category = "Construction")
    void ToggleBuildMode();

    UFUNCTION(BlueprintPure, Category = "Construction")
    bool IsBuildModeActive() const
    {
        return BuildModeState != ESGBuildModeState::Inactive;
    }

    /**
     * DataTable을 읽은 뒤 선택된 구조물 정보를 전달한다.
     *
     * 이후 기존 StructureDefinition Struct와 직접 연결할 예정이다.
     */
    UFUNCTION(BlueprintCallable, Category = "Construction")
    void SetSelectedStructureDefinition(
        FName StructureRowName,
        TSubclassOf<AActor> StructureClass,
        TSubclassOf<AActor> PreviewClass,
        const TArray<FSGResourceCost>& ResourceCosts,
        bool bRequiresPower,
        float MaxAllowedSlope,
        FVector PlacementExtent
    );

    UFUNCTION(BlueprintCallable, Category = "Construction")
    void ClearSelectedStructure();

    UFUNCTION(BlueprintCallable, Category = "Construction")
    bool TryPlaceSelectedStructure();

    UFUNCTION(BlueprintCallable, Category = "Construction")
    void RotatePreview(float RotationAmount);

    UFUNCTION(BlueprintCallable, Category = "Construction")
    void CancelPlacement();

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

protected:
    /**
     * 전력 구조물의 발전기 반경 및 잔여 전력 검사.
     *
     * 기존 PowerCore 구조에 맞춰 Blueprint에서 우선 연결할 수 있다.
     */
    UFUNCTION(BlueprintNativeEvent, Category = "Construction|Power")
    bool CheckPowerRequirement(
        const FVector& PlacementLocation
    ) const;

    virtual bool CheckPowerRequirement_Implementation(
        const FVector& PlacementLocation
    ) const;

    UFUNCTION(BlueprintImplementableEvent, Category = "Construction")
    void OnPreviewValidityChanged(
        bool bCanPlace,
        ESGPlacementFailureReason FailureReason
    );

    UFUNCTION(BlueprintImplementableEvent, Category = "Construction")
    void OnBuildModeEntered();

    UFUNCTION(BlueprintImplementableEvent, Category = "Construction")
    void OnBuildModeExited();

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Construction|Trace",
        meta = (ClampMin = "100.0"))
    float PlacementDistance = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Construction|Trace")
    TEnumAsByte<ECollisionChannel> PlacementTraceChannel =
        ECC_Visibility;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Construction|Trace")
    TEnumAsByte<ECollisionChannel> PlacementOverlapChannel =
        ECC_WorldStatic;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "Construction|Rotation",
        meta = (ClampMin = "1.0"))
    float RotationStep = 15.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "Construction|Placement",
        meta = (ClampMin = "0.0"))
    float GroundOffset = 0.0f;

public:
    UPROPERTY(BlueprintAssignable, Category = "Construction")
    FSGOnBuildModeChanged OnBuildModeChanged;

    UPROPERTY(BlueprintAssignable, Category = "Construction")
    FSGOnPlacementResultChanged OnPlacementResultChanged;

    UPROPERTY(BlueprintAssignable, Category = "Construction")
    FSGOnStructurePlaced OnStructurePlaced;

private:
    UPROPERTY(VisibleInstanceOnly, Category = "Construction")
    ESGBuildModeState BuildModeState =
        ESGBuildModeState::Inactive;

    UPROPERTY(VisibleInstanceOnly, Category = "Construction")
    FName SelectedStructureRow = NAME_None;

    UPROPERTY(VisibleInstanceOnly, Category = "Construction")
    TSubclassOf<AActor> SelectedStructureClass;

    UPROPERTY(VisibleInstanceOnly, Category = "Construction")
    TSubclassOf<AActor> SelectedPreviewClass;

    UPROPERTY(VisibleInstanceOnly, Category = "Construction")
    TArray<FSGResourceCost> SelectedResourceCosts;

    UPROPERTY(VisibleInstanceOnly, Category = "Construction")
    bool bSelectedRequiresPower = false;

    UPROPERTY(VisibleInstanceOnly, Category = "Construction")
    float SelectedMaxSlopeDegrees = 10.0f;

    UPROPERTY(VisibleInstanceOnly, Category = "Construction")
    FVector SelectedPlacementExtent =
        FVector(50.0f, 50.0f, 50.0f);

    UPROPERTY(VisibleInstanceOnly, Category = "Construction")
    float CurrentPreviewYaw = 0.0f;

    UPROPERTY(Transient)
    TObjectPtr<AActor> PreviewActor;

    UPROPERTY(Transient)
    TObjectPtr<UCameraComponent> CachedCamera;

    UPROPERTY(Transient)
    TObjectPtr<USGResourceInventoryComponent> ResourceInventory;

    UPROPERTY(VisibleInstanceOnly, Category = "Construction")
    FSGPlacementResult CurrentPlacementResult;

private:
    UCameraComponent* FindOwnerCamera() const;

    void UpdatePlacementPreview();
    void SpawnPreviewActor();
    void DestroyPreviewActor();
    void ConfigurePreviewActor(AActor* InPreviewActor) const;

    bool CalculatePlacementResult(
        FSGPlacementResult& OutResult
    ) const;

    bool CheckGroundTrace(
        FHitResult& OutGroundHit
    ) const;

    bool CheckSlope(
        const FVector& SurfaceNormal
    ) const;

    bool CheckPlacementOverlap(
        const FTransform& PlacementTransform
    ) const;

    bool CheckResources() const;
};