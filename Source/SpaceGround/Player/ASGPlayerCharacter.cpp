#include "ASGPlayerCharacter.h"

#include "../Components/USGConstructionComponent.h"
#include "../Components/USGInteractionComponent.h"
#include "../Components/USGResourceInventoryComponent.h"

#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputAction.h"

ASGPlayerCharacter::ASGPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	ResourceInventoryComponent =
		CreateDefaultSubobject<
			USGResourceInventoryComponent
		>(TEXT("ResourceInventoryComponent"));

	ConstructionComponent =
		CreateDefaultSubobject<
			USGConstructionComponent
		>(TEXT("ConstructionComponent"));

	InteractionComponent =
		CreateDefaultSubobject<
			USGInteractionComponent
		>(TEXT("InteractionComponent"));

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* Movement =
		GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = false;
		Movement->MaxWalkSpeed = WalkSpeed;

		Movement
			->GetNavAgentPropertiesRef()
			.bCanCrouch = true;
	}
}

void ASGPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (UCharacterMovementComponent* Movement =
        GetCharacterMovement())
    {
        Movement->MaxWalkSpeed = WalkSpeed;
    }
}

void ASGPlayerCharacter::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    UpdateControlRotationForAnimation();
}

void ASGPlayerCharacter::SetupPlayerInputComponent(
    UInputComponent* PlayerInputComponent
)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UEnhancedInputComponent* EnhancedInputComponent =
        Cast<UEnhancedInputComponent>(
            PlayerInputComponent
        );

    if (!EnhancedInputComponent)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT(
                "ASGPlayerCharacter requires "
                "UEnhancedInputComponent."
            )
        );

        return;
    }

    if (MoveAction)
    {
        EnhancedInputComponent->BindAction(
            MoveAction,
            ETriggerEvent::Triggered,
            this,
            &ASGPlayerCharacter::Move
        );

        EnhancedInputComponent->BindAction(
            MoveAction,
            ETriggerEvent::Completed,
            this,
            &ASGPlayerCharacter::Move
        );
    }

    if (LookAction)
    {
        EnhancedInputComponent->BindAction(
            LookAction,
            ETriggerEvent::Triggered,
            this,
            &ASGPlayerCharacter::Look
        );

        EnhancedInputComponent->BindAction(
            LookAction,
            ETriggerEvent::Completed,
            this,
            &ASGPlayerCharacter::Look
        );
    }

    if (JumpAction)
    {
        EnhancedInputComponent->BindAction(
            JumpAction,
            ETriggerEvent::Started,
            this,
            &ASGPlayerCharacter::HandleJumpStarted
        );

        EnhancedInputComponent->BindAction(
            JumpAction,
            ETriggerEvent::Completed,
            this,
            &ASGPlayerCharacter::HandleJumpCompleted
        );
    }

    if (CrouchAction)
    {
        EnhancedInputComponent->BindAction(
            CrouchAction,
            ETriggerEvent::Started,
            this,
            &ASGPlayerCharacter::HandleCrouchStarted
        );
    }

    if (SprintAction)
    {
        EnhancedInputComponent->BindAction(
            SprintAction,
            ETriggerEvent::Started,
            this,
            &ASGPlayerCharacter::HandleSprintStarted
        );

        EnhancedInputComponent->BindAction(
            SprintAction,
            ETriggerEvent::Completed,
            this,
            &ASGPlayerCharacter::HandleSprintCompleted
        );
    }

    if (ZoomAction)
    {
        EnhancedInputComponent->BindAction(
            ZoomAction,
            ETriggerEvent::Started,
            this,
            &ASGPlayerCharacter::HandleZoomStarted
        );

        EnhancedInputComponent->BindAction(
            ZoomAction,
            ETriggerEvent::Completed,
            this,
            &ASGPlayerCharacter::HandleZoomCompleted
        );
    }

    if (LeanLeftAction)
    {
        EnhancedInputComponent->BindAction(
            LeanLeftAction,
            ETriggerEvent::Started,
            this,
            &ASGPlayerCharacter::HandleLeanLeftStarted
        );

        EnhancedInputComponent->BindAction(
            LeanLeftAction,
            ETriggerEvent::Completed,
            this,
            &ASGPlayerCharacter::HandleLeanLeftCompleted
        );
    }

    if (LeanRightAction)
    {
        EnhancedInputComponent->BindAction(
            LeanRightAction,
            ETriggerEvent::Started,
            this,
            &ASGPlayerCharacter::HandleLeanRightStarted
        );

        EnhancedInputComponent->BindAction(
            LeanRightAction,
            ETriggerEvent::Completed,
            this,
            &ASGPlayerCharacter::HandleLeanRightCompleted
        );
    }

    if (BuildModeAction)
    {
        EnhancedInputComponent->BindAction(
            BuildModeAction,
            ETriggerEvent::Started,
            this,
            &ASGPlayerCharacter::HandleBuildMode
        );
    }

    if (PlaceStructureAction)
    {
        EnhancedInputComponent->BindAction(
            PlaceStructureAction,
            ETriggerEvent::Started,
            this,
            &ASGPlayerCharacter::HandlePlaceStructure
        );
    }

    if (CancelBuildAction)
    {
        EnhancedInputComponent->BindAction(
            CancelBuildAction,
            ETriggerEvent::Started,
            this,
            &ASGPlayerCharacter::HandleCancelBuild
        );
    }

    if (RotateStructureAction)
    {
        EnhancedInputComponent->BindAction(
            RotateStructureAction,
            ETriggerEvent::Triggered,
            this,
            &ASGPlayerCharacter::HandleRotateStructure
        );
    }

    if (InteractAction)
    {
        EnhancedInputComponent->BindAction(
            InteractAction,
            ETriggerEvent::Started,
            this,
            &ASGPlayerCharacter::HandleInteract
        );
    }
}

void ASGPlayerCharacter::Move(
    const FInputActionValue& Value
)
{
    const FVector2D MoveInput =
        Value.Get<FVector2D>();

    ForwardInputValue = MoveInput.Y;
    RightInputValue = MoveInput.X;

    if (!CanMove() || !Controller)
    {
        return;
    }

    const FRotator ControlRotation =
        Controller->GetControlRotation();

    const FRotator YawRotation(
        0.0f,
        ControlRotation.Yaw,
        0.0f
    );

    const FVector ForwardDirection =
        FRotationMatrix(YawRotation)
            .GetUnitAxis(EAxis::X);

    const FVector RightDirection =
        FRotationMatrix(YawRotation)
            .GetUnitAxis(EAxis::Y);

    AddMovementInput(
        ForwardDirection,
        ForwardInputValue
    );

    AddMovementInput(
        RightDirection,
        RightInputValue
    );
}

void ASGPlayerCharacter::Look(
    const FInputActionValue& Value
)
{
    const FVector2D LookInput =
        Value.Get<FVector2D>();

    TurnInputValue = LookInput.X;
    LookInputValue = LookInput.Y;

    if (PlayerActionState == ESGPlayerActionState::Disabled
        ||
        PlayerActionState == ESGPlayerActionState::Dead)
    {
        return;
    }

    AddControllerYawInput(TurnInputValue);
    AddControllerPitchInput(LookInputValue);
}

void ASGPlayerCharacter::HandleJumpStarted()
{
    if (!bEnableJump || !CanMove())
    {
        return;
    }

    if (bEnableTraversal && TryStartTraversal())
    {
        return;
    }

    Jump();
}

void ASGPlayerCharacter::HandleJumpCompleted()
{
    StopJumping();
}

void ASGPlayerCharacter::HandleCrouchStarted()
{
    if (!bEnableCrouch || !CanMove())
    {
        return;
    }

    if (TryStartSlide())
    {
        return;
    }

    if (GetCharacterMovement()->IsFalling())
    {
        return;
    }

    if (bIsCrouched)
    {
        UnCrouch();
    }
    else
    {
        Crouch();
    }
}

void ASGPlayerCharacter::HandleSprintStarted()
{
    if (!CanMove())
    {
        return;
    }

    /*
     * 뒤로 달리기 방지.
     * 기존 BP의 W 직접 감지 대신 IA_Move Y값으로 판단한다.
     */
    if (ForwardInputValue <= 0.1f)
    {
        return;
    }

    SetSprinting(true);
}

void ASGPlayerCharacter::HandleSprintCompleted()
{
    SetSprinting(false);
}

void ASGPlayerCharacter::SetSprinting(
    const bool bNewSprinting
)
{
    bIsSprinting = bNewSprinting;

    if (UCharacterMovementComponent* Movement =
        GetCharacterMovement())
    {
        Movement->MaxWalkSpeed =
            bIsSprinting
            ? SprintSpeed
            : WalkSpeed;
    }
}

void ASGPlayerCharacter::HandleZoomStarted()
{
    if (!CanUseGameplayAction())
    {
        return;
    }

    /*
     * 기획 기준 ADS 시작 시 달리기 즉시 취소.
     */
    SetSprinting(false);

    bIsZooming = true;
    OnZoomStateChanged(true);
}

void ASGPlayerCharacter::HandleZoomCompleted()
{
    bIsZooming = false;
    OnZoomStateChanged(false);
}

void ASGPlayerCharacter::HandleLeanLeftStarted()
{
    if (!bEnableLean || !CanUseGameplayAction())
    {
        return;
    }

    SetLeanValues(
        -LeanAngle,
        LeanArmAngle,
        0.0f
    );
}

void ASGPlayerCharacter::HandleLeanLeftCompleted()
{
    SetLeanValues(
        0.0f,
        0.0f,
        0.0f
    );
}

void ASGPlayerCharacter::HandleLeanRightStarted()
{
    if (!bEnableLean || !CanUseGameplayAction())
    {
        return;
    }

    SetLeanValues(
        LeanAngle,
        0.0f,
        -LeanArmAngle
    );
}

void ASGPlayerCharacter::HandleLeanRightCompleted()
{
    SetLeanValues(
        0.0f,
        0.0f,
        0.0f
    );
}

void ASGPlayerCharacter::SetLeanValues(
    const float NewBaseLean,
    const float NewLeftArmLean,
    const float NewRightArmLean
)
{
    BaseLeaningAlpha = NewBaseLean;
    LeaningLeftArmAlpha = NewLeftArmLean;
    LeaningRightArmAlpha = NewRightArmLean;

    OnLeanStateChanged(
        BaseLeaningAlpha,
        LeaningLeftArmAlpha,
        LeaningRightArmAlpha
    );
}

void ASGPlayerCharacter::HandleBuildMode()
{
    if (!ConstructionComponent)
    {
        return;
    }

    if (ConstructionComponent->IsBuildModeActive())
    {
        ConstructionComponent->ExitBuildMode();
        SetPlayerActionState(
            ESGPlayerActionState::Normal
        );
    }
    else
    {
        if (!CanUseGameplayAction())
        {
            return;
        }

        SetSprinting(false);

        ConstructionComponent->EnterBuildMode();
        SetPlayerActionState(
            ESGPlayerActionState::Building
        );
    }
}

void ASGPlayerCharacter::HandlePlaceStructure()
{
    if (!ConstructionComponent
        ||
        !ConstructionComponent->IsBuildModeActive())
    {
        return;
    }

    ConstructionComponent->TryPlaceSelectedStructure();
}

void ASGPlayerCharacter::HandleCancelBuild()
{
    if (!ConstructionComponent
        ||
        !ConstructionComponent->IsBuildModeActive())
    {
        return;
    }

    ConstructionComponent->CancelPlacement();

    SetPlayerActionState(
        ESGPlayerActionState::Normal
    );
}

void ASGPlayerCharacter::HandleRotateStructure(
    const FInputActionValue& Value
)
{
    if (!ConstructionComponent
        ||
        !ConstructionComponent->IsBuildModeActive())
    {
        return;
    }

    float RotationInput = 0.0f;

    if (Value.GetValueType()
        == EInputActionValueType::Axis1D)
    {
        RotationInput = Value.Get<float>();
    }

    if (FMath::IsNearlyZero(RotationInput))
    {
        return;
    }

    ConstructionComponent->RotatePreview(
        RotationInput > 0.0f
        ? 15.0f
        : -15.0f
    );
}

void ASGPlayerCharacter::HandleInteract()
{
    if (!InteractionComponent
        ||
        PlayerActionState == ESGPlayerActionState::Dead
        ||
        PlayerActionState == ESGPlayerActionState::Disabled)
    {
        return;
    }

    InteractionComponent->TryInteract();
}

bool ASGPlayerCharacter::TryStartTraversal_Implementation()
{
    return false;
}

bool ASGPlayerCharacter::TryStartSlide_Implementation()
{
    return false;
}

void ASGPlayerCharacter::UpdateControlRotationForAnimation()
{
    if (!Controller)
    {
        return;
    }

    ControlRotationForAnimation =
        Controller->GetControlRotation();
}

void ASGPlayerCharacter::SetPlayerActionState(
    const ESGPlayerActionState NewState
)
{
    if (PlayerActionState == NewState)
    {
        return;
    }

    const ESGPlayerActionState OldState =
        PlayerActionState;

    PlayerActionState = NewState;

    if (NewState != ESGPlayerActionState::Normal)
    {
        SetSprinting(false);
    }

    OnPlayerActionStateChanged(
        OldState,
        NewState
    );
}

bool ASGPlayerCharacter::CanMove() const
{
    return PlayerActionState != ESGPlayerActionState::Interacting
        &&
        PlayerActionState != ESGPlayerActionState::Traversing
        &&
        PlayerActionState != ESGPlayerActionState::HardLanding
        &&
        PlayerActionState != ESGPlayerActionState::Disabled
        &&
        PlayerActionState != ESGPlayerActionState::Dead;
}

bool ASGPlayerCharacter::CanUseGameplayAction() const
{
    return PlayerActionState == ESGPlayerActionState::Normal;
}