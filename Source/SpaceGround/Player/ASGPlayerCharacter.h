#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "../Player/SGPlayerTypes.h"
#include "ASGPlayerCharacter.generated.h"

class UInputAction;
class USGConstructionComponent;
class USGInteractionComponent;
class USGResourceInventoryComponent;

/**
 * SpaceGround 플레이어의 게임플레이 기반 클래스.
 *
 * Blueprint에서 유지하는 항목:
 * - Skeletal Mesh
 * - AnimBP
 * - CameraArm
 * - Camera
 * - True First Person 연출
 * - Zoom Timeline
 * - BPC_TraversalSystem
 * - Footstep
 */
UCLASS()
class SPACEGROUND_API ASGPlayerCharacter
    : public ACharacter
{
    GENERATED_BODY()

public:
    ASGPlayerCharacter();

protected:
    virtual void BeginPlay() override;

    virtual void Tick(float DeltaSeconds) override;

    virtual void SetupPlayerInputComponent(
        UInputComponent* PlayerInputComponent
    ) override;

public:
    UFUNCTION(BlueprintPure, Category = "Player|Component")
    USGResourceInventoryComponent*
    GetResourceInventoryComponent() const
    {
        return ResourceInventoryComponent;
    }

    UFUNCTION(BlueprintPure, Category = "Player|Component")
    USGConstructionComponent*
    GetConstructionComponent() const
    {
        return ConstructionComponent;
    }

    UFUNCTION(BlueprintPure, Category = "Player|Component")
    USGInteractionComponent*
    GetInteractionComponent() const
    {
        return InteractionComponent;
    }

    UFUNCTION(BlueprintPure, Category = "Player|State")
    ESGPlayerActionState GetPlayerActionState() const
    {
        return PlayerActionState;
    }

    UFUNCTION(BlueprintCallable, Category = "Player|State")
    void SetPlayerActionState(
        ESGPlayerActionState NewState
    );

    UFUNCTION(BlueprintPure, Category = "Player|State")
    bool CanMove() const;

    UFUNCTION(BlueprintPure, Category = "Player|State")
    bool CanUseGameplayAction() const;

    UFUNCTION(BlueprintPure, Category = "Player|Movement")
    bool IsSprinting() const
    {
        return bIsSprinting;
    }

    UFUNCTION(BlueprintPure, Category = "Player|Movement")
    float GetForwardInputValue() const
    {
        return ForwardInputValue;
    }

    UFUNCTION(BlueprintPure, Category = "Player|Movement")
    float GetRightInputValue() const
    {
        return RightInputValue;
    }

    UFUNCTION(BlueprintPure, Category = "Player|Animation")
    FRotator GetControlRotationForAnimation() const
    {
        return ControlRotationForAnimation;
    }

protected:
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);

    void HandleJumpStarted();
    void HandleJumpCompleted();

    void HandleCrouchStarted();

    void HandleSprintStarted();
    void HandleSprintCompleted();

    void HandleZoomStarted();
    void HandleZoomCompleted();

    void HandleLeanLeftStarted();
    void HandleLeanLeftCompleted();

    void HandleLeanRightStarted();
    void HandleLeanRightCompleted();

    void HandleBuildMode();
    void HandlePlaceStructure();
    void HandleCancelBuild();
    void HandleRotateStructure(const FInputActionValue& Value);

    void HandleInteract();

    /**
     * BP Traversal 컴포넌트가 Jump 입력을 소비했는지 반환한다.
     *
     * BP_SGPlayerCharacter에서 Override한다.
     */
    UFUNCTION(BlueprintNativeEvent, Category = "Player|Traversal")
    bool TryStartTraversal();

    virtual bool TryStartTraversal_Implementation();

    /**
     * 기존 Slide 기능을 유지할 때 Blueprint에서 Override한다.
     */
    UFUNCTION(BlueprintNativeEvent, Category = "Player|Movement")
    bool TryStartSlide();

    virtual bool TryStartSlide_Implementation();

    /**
     * 기존 BP Zoom Timeline을 실행하기 위한 이벤트.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Player|Camera")
    void OnZoomStateChanged(bool bZoomActive);

    /**
     * 기존 AnimBP Lean 값 연결용 이벤트.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Player|Movement")
    void OnLeanStateChanged(
        float BaseLeanAlpha,
        float LeftArmLeanAlpha,
        float RightArmLeanAlpha
    );

    UFUNCTION(BlueprintImplementableEvent, Category = "Player|State")
    void OnPlayerActionStateChanged(
        ESGPlayerActionState OldState,
        ESGPlayerActionState NewState
    );

private:
    void UpdateControlRotationForAnimation();
    void SetSprinting(bool bNewSprinting);
    void SetLeanValues(
        float NewBaseLean,
        float NewLeftArmLean,
        float NewRightArmLean
    );

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
        Category = "Player|Component")
    TObjectPtr<USGResourceInventoryComponent>
    ResourceInventoryComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
        Category = "Player|Component")
    TObjectPtr<USGConstructionComponent>
    ConstructionComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
        Category = "Player|Component")
    TObjectPtr<USGInteractionComponent>
    InteractionComponent;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> MoveAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> LookAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> JumpAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> CrouchAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> SprintAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> ZoomAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> LeanLeftAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> LeanRightAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> BuildModeAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> PlaceStructureAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> CancelBuildAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> RotateStructureAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> InteractAction;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Movement|Speed",
        meta = (ClampMin = "0.0"))
    float WalkSpeed = 150.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Movement|Speed",
        meta = (ClampMin = "0.0"))
    float SprintSpeed = 450.0f;

    /**
     * 기획상 점프를 사용하지 않으므로 기본 false.
     * 에셋 테스트 단계에서는 true로 변경할 수 있다.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Movement|Feature")
    bool bEnableJump = false;

    /**
     * 기획상 앉기를 사용하지 않으므로 기본 false.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Movement|Feature")
    bool bEnableCrouch = false;

    /**
     * Vault/Mantle를 실제 게임에서 사용할지 여부.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Movement|Feature")
    bool bEnableTraversal = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Movement|Feature")
    bool bEnableLean = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Movement|Lean")
    float LeanAngle = 25.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
        Category = "Movement|Lean")
    float LeanArmAngle = 15.0f;

protected:
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly,
        Category = "Player|State")
    ESGPlayerActionState PlayerActionState =
        ESGPlayerActionState::Normal;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly,
        Category = "Movement")
    bool bIsSprinting = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly,
        Category = "Movement")
    bool bIsZooming = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly,
        Category = "Movement")
    float ForwardInputValue = 0.0f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly,
        Category = "Movement")
    float RightInputValue = 0.0f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly,
        Category = "Movement")
    float TurnInputValue = 0.0f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly,
        Category = "Movement")
    float LookInputValue = 0.0f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly,
        Category = "Animation")
    FRotator ControlRotationForAnimation =
        FRotator::ZeroRotator;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly,
        Category = "Movement|Lean")
    float BaseLeaningAlpha = 0.0f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly,
        Category = "Movement|Lean")
    float LeaningLeftArmAlpha = 0.0f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly,
        Category = "Movement|Lean")
    float LeaningRightArmAlpha = 0.0f;
};