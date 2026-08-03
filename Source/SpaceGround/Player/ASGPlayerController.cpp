#include "ASGPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

ASGPlayerController::ASGPlayerController()
{
    bShowMouseCursor = false;
}

void ASGPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (!IsLocalController())
    {
        return;
    }

    AddDefaultMappingContext();
    SetMouseGameOnlyMode();
}

void ASGPlayerController::AddDefaultMappingContext()
{
    if (!IsLocalController() || !DefaultMappingContext)
    {
        return;
    }

    ULocalPlayer* LocalPlayer = GetLocalPlayer();

    if (!LocalPlayer)
    {
        return;
    }

    UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
        LocalPlayer->GetSubsystem<
            UEnhancedInputLocalPlayerSubsystem
        >();

    if (!InputSubsystem)
    {
        return;
    }

    InputSubsystem->AddMappingContext(
        DefaultMappingContext,
        DefaultMappingPriority
    );
}

void ASGPlayerController::RemoveDefaultMappingContext()
{
    if (!IsLocalController() || !DefaultMappingContext)
    {
        return;
    }

    ULocalPlayer* LocalPlayer = GetLocalPlayer();

    if (!LocalPlayer)
    {
        return;
    }

    UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
        LocalPlayer->GetSubsystem<
            UEnhancedInputLocalPlayerSubsystem
        >();

    if (!InputSubsystem)
    {
        return;
    }

    InputSubsystem->RemoveMappingContext(
        DefaultMappingContext
    );
}

void ASGPlayerController::SetGameplayInputEnabled(
    const bool bEnabled
)
{
    if (bEnabled)
    {
        EnableInput(this);
        ResetIgnoreMoveInput();
    }
    else
    {
        SetIgnoreMoveInput(true);
    }
}

void ASGPlayerController::SetLookInputEnabled(
    const bool bEnabled
)
{
    if (bEnabled)
    {
        ResetIgnoreLookInput();
    }
    else
    {
        SetIgnoreLookInput(true);
    }
}

void ASGPlayerController::SetMouseGameOnlyMode()
{
    FInputModeGameOnly InputMode;
    SetInputMode(InputMode);

    bShowMouseCursor = false;
}

void ASGPlayerController::SetMouseGameAndUIInputMode()
{
    FInputModeGameAndUI InputMode;

    InputMode.SetHideCursorDuringCapture(false);
    InputMode.SetLockMouseToViewportBehavior(
        EMouseLockMode::DoNotLock
    );

    SetInputMode(InputMode);

    bShowMouseCursor = true;
}