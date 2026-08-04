#include "ASGPlayerCharacter.h"

#include "../Components/USGConstructionComponent.h"
#include "../Components/USGInteractionComponent.h"
#include "../Components/USGResourceInventoryComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include "Engine/LocalPlayer.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

#include "InputAction.h"
#include "InputMappingContext.h"

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

void ASGPlayerCharacter::Tick(
    const float DeltaSeconds
)
{
    Super::Tick(DeltaSeconds);

    UpdateControlRotationForAnimation();
}

void ASGPlayerCharacter::SetupPlayerInputComponent(
    UInputComponent* PlayerInputComponent
)
{
    Super::SetupPlayerInputComponent(
        PlayerInputComponent
    );

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

    // ─────────────────────────────────────────────
    // 이동

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

    // ─────────────────────────────────────────────
    // 시점

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

    // ─────────────────────────────────────────────
    // 점프

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

    // ─────────────────────────────────────────────
    // 앉기

    if (CrouchAction)
    {
        EnhancedInputComponent->BindAction(
            CrouchAction,
            ETriggerEvent::Started,
            this,
            &ASGPlayerCharacter::HandleCrouchStarted
        );
    }

    // ─────────────────────────────────────────────
    // 달리기

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

    // ─────────────────────────────────────────────
    // 줌 / ADS

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

    // ─────────────────────────────────────────────
    // 좌측 기울이기

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

    // ─────────────────────────────────────────────
    // 우측 기울이기

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

    // ─────────────────────────────────────────────
    // 건설 모드

    if (BuildModeAction)
    {
        EnhancedInputComponent->BindAction(
            BuildModeAction,
            ETriggerEvent::Started,
            this,
            &ASGPlayerCharacter::HandleBuildMode
        );
    }

    // ─────────────────────────────────────────────
    // 구조물 설치

    if (PlaceStructureAction)
    {
        EnhancedInputComponent->BindAction(
            PlaceStructureAction,
            ETriggerEvent::Started,
            this,
            &ASGPlayerCharacter::HandlePlaceStructure
        );
    }

    // ─────────────────────────────────────────────
    // 건설 취소

    if (CancelBuildAction)
    {
        EnhancedInputComponent->BindAction(
            CancelBuildAction,
            ETriggerEvent::Started,
            this,
            &ASGPlayerCharacter::HandleCancelBuild
        );
    }

    // ─────────────────────────────────────────────
    // 구조물 회전

    if (RotateStructureAction)
    {
        EnhancedInputComponent->BindAction(
            RotateStructureAction,
            ETriggerEvent::Triggered,
            this,
            &ASGPlayerCharacter::HandleRotateStructure
        );
    }

    // ─────────────────────────────────────────────
    // 구조물 선택

    if (SelectPowerCoreAction)
    {
        EnhancedInputComponent->BindAction(
            SelectPowerCoreAction,
            ETriggerEvent::Started,
            this,
            &ASGPlayerCharacter::HandleSelectPowerCore
        );
    }

    if (SelectTurretAction)
    {
        EnhancedInputComponent->BindAction(
            SelectTurretAction,
            ETriggerEvent::Started,
            this,
            &ASGPlayerCharacter::HandleSelectTurret
        );
    }

    if (SelectBarricadeAction)
    {
        EnhancedInputComponent->BindAction(
            SelectBarricadeAction,
            ETriggerEvent::Started,
            this,
            &ASGPlayerCharacter::HandleSelectBarricade
        );
    }

    if (SelectShockMineAction)
    {
        EnhancedInputComponent->BindAction(
            SelectShockMineAction,
            ETriggerEvent::Started,
            this,
            &ASGPlayerCharacter::HandleSelectShockMine
        );
    }

    if (SelectSlowPadAction)
    {
        EnhancedInputComponent->BindAction(
            SelectSlowPadAction,
            ETriggerEvent::Started,
            this,
            &ASGPlayerCharacter::HandleSelectSlowPad
        );
    }

    // ─────────────────────────────────────────────
    // 상호작용

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

    if (PlayerActionState ==
            ESGPlayerActionState::Disabled
        ||
        PlayerActionState ==
            ESGPlayerActionState::Dead)
    {
        return;
    }

    AddControllerYawInput(
        TurnInputValue
    );

    AddControllerPitchInput(
        LookInputValue
    );
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

    UCharacterMovementComponent* Movement =
        GetCharacterMovement();

    if (!Movement || Movement->IsFalling())
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
     * 건설 중에는 달리기 입력을 차단한다.
     */
    if (PlayerActionState ==
        ESGPlayerActionState::Building)
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
     * ADS 시작 시 달리기 즉시 취소.
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

    /*
     * 이미 건설 모드라면 종료한다.
     */
    if (ConstructionComponent->IsBuildModeActive())
    {
        ConstructionComponent->ExitBuildMode();

        RemoveConstructionMappingContext();

        SetPlayerActionState(
            ESGPlayerActionState::Normal
        );

        UE_LOG(
            LogTemp,
            Log,
            TEXT("Construction mode exited.")
        );

        return;
    }

    /*
     * Normal 상태에서만 건설 모드 진입 가능.
     */
    if (!CanUseGameplayAction())
    {
        return;
    }

    /*
     * 건설 모드 진입 시 달리기 종료.
     */
    SetSprinting(false);

    /*
     * 건설 모드 진입 시 Zoom 종료.
     */
    if (bIsZooming)
    {
        bIsZooming = false;

        OnZoomStateChanged(false);
    }

    /*
     * 기울이기 초기화.
     */
    SetLeanValues(
        0.0f,
        0.0f,
        0.0f
    );

    /*
     * 건설 전용 Mapping Context 활성화.
     */
    AddConstructionMappingContext();

    /*
     * 건설 컴포넌트 활성화.
     */
    ConstructionComponent->EnterBuildMode();

    SetPlayerActionState(
        ESGPlayerActionState::Building
    );

    UE_LOG(
        LogTemp,
        Log,
        TEXT("Construction mode entered.")
    );
}

void ASGPlayerCharacter::HandlePlaceStructure()
{
    if (!ConstructionComponent
        ||
        !ConstructionComponent->IsBuildModeActive())
    {
        return;
    }

    const bool bPlaced =
        ConstructionComponent
            ->TryPlaceSelectedStructure();

    if (!bPlaced)
    {
        const FSGPlacementResult& PlacementResult =
            ConstructionComponent
                ->GetPlacementResult();

        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "Structure placement failed. "
                "FailureReason=%d"
            ),
            static_cast<int32>(
                PlacementResult.FailureReason
            )
        );
    }
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

    RemoveConstructionMappingContext();

    SetPlayerActionState(
        ESGPlayerActionState::Normal
    );

    UE_LOG(
        LogTemp,
        Log,
        TEXT("Construction placement cancelled.")
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

    if (Value.GetValueType() !=
        EInputActionValueType::Axis1D)
    {
        return;
    }

    const float RotationInput =
        Value.Get<float>();

    if (FMath::IsNearlyZero(RotationInput))
    {
        return;
    }

    /*
     * 방향만 ConstructionComponent에 전달한다.
     * 실제 회전 각도는 RotationStep에서 관리한다.
     */
    ConstructionComponent->RotatePreview(
        RotationInput > 0.0f
        ? 1.0f
        : -1.0f
    );
}

void ASGPlayerCharacter::HandleSelectPowerCore()
{
    RequestStructureSelection(
        TEXT("PowerCore")
    );
}

void ASGPlayerCharacter::HandleSelectTurret()
{
    RequestStructureSelection(
        TEXT("MachineGunTurret")
    );
}

void ASGPlayerCharacter::HandleSelectBarricade()
{
    RequestStructureSelection(
        TEXT("Barricade")
    );
}

void ASGPlayerCharacter::HandleSelectShockMine()
{
    RequestStructureSelection(
        TEXT("ShockMine")
    );
}

void ASGPlayerCharacter::HandleSelectSlowPad()
{
    RequestStructureSelection(
        TEXT("SlowPad")
    );
}

void ASGPlayerCharacter::RequestStructureSelection(
    const FName StructureRowName
)
{
    if (!ConstructionComponent
        ||
        !ConstructionComponent->IsBuildModeActive())
    {
        return;
    }

    if (StructureRowName.IsNone())
    {
        return;
    }

    const bool bSelected =
        ConstructionComponent
            ->SelectStructureByRowName(
                StructureRowName
            );

    if (!bSelected)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "Failed to select structure Row: %s"
            ),
            *StructureRowName.ToString()
        );
    }
}

void ASGPlayerCharacter::AddConstructionMappingContext()
{
    if (!ConstructionMappingContext)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "ConstructionMappingContext "
                "is not assigned."
            )
        );

        return;
    }

    APlayerController* PlayerController =
        Cast<APlayerController>(
            GetController()
        );

    if (!PlayerController)
    {
        return;
    }

    ULocalPlayer* LocalPlayer =
        PlayerController->GetLocalPlayer();

    if (!LocalPlayer)
    {
        return;
    }

    UEnhancedInputLocalPlayerSubsystem*
        InputSubsystem =
            LocalPlayer->GetSubsystem<
                UEnhancedInputLocalPlayerSubsystem
            >();

    if (!InputSubsystem)
    {
        return;
    }

    /*
     * 중복 추가를 방지하기 위해 먼저 제거한 뒤 추가한다.
     */
    InputSubsystem->RemoveMappingContext(
        ConstructionMappingContext
    );

    InputSubsystem->AddMappingContext(
        ConstructionMappingContext,
        ConstructionMappingPriority
    );
}

void ASGPlayerCharacter::RemoveConstructionMappingContext()
{
    if (!ConstructionMappingContext)
    {
        return;
    }

    APlayerController* PlayerController =
        Cast<APlayerController>(
            GetController()
        );

    if (!PlayerController)
    {
        return;
    }

    ULocalPlayer* LocalPlayer =
        PlayerController->GetLocalPlayer();

    if (!LocalPlayer)
    {
        return;
    }

    UEnhancedInputLocalPlayerSubsystem*
        InputSubsystem =
            LocalPlayer->GetSubsystem<
                UEnhancedInputLocalPlayerSubsystem
            >();

    if (!InputSubsystem)
    {
        return;
    }

    InputSubsystem->RemoveMappingContext(
        ConstructionMappingContext
    );
}

void ASGPlayerCharacter::HandleInteract()
{
    if (!InteractionComponent
        ||
        PlayerActionState ==
            ESGPlayerActionState::Dead
        ||
        PlayerActionState ==
            ESGPlayerActionState::Disabled
        ||
        PlayerActionState ==
            ESGPlayerActionState::Building)
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

    /*
     * Normal 이외의 상태에 들어가면 달리기 종료.
     */
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
    /*
     * Building 상태에서도 이동과 시점 조작은 허용한다.
     */
    return PlayerActionState !=
            ESGPlayerActionState::Interacting
        &&
        PlayerActionState !=
            ESGPlayerActionState::Traversing
        &&
        PlayerActionState !=
            ESGPlayerActionState::HardLanding
        &&
        PlayerActionState !=
            ESGPlayerActionState::Disabled
        &&
        PlayerActionState !=
            ESGPlayerActionState::Dead;
}

bool ASGPlayerCharacter::CanUseGameplayAction() const
{
    /*
     * 사격, ADS, 기울이기, 건설 모드 진입 등
     * 일반 행동은 Normal 상태에서만 허용한다.
     */
    return PlayerActionState ==
        ESGPlayerActionState::Normal;
}