#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ASGPlayerController.generated.h"

class UInputMappingContext;

/**
 * SpaceGround 플레이어 입력 컨텍스트와 입력 잠금을 관리한다.
 */
UCLASS()
class SPACEGROUND_API ASGPlayerController
	: public APlayerController
{
	GENERATED_BODY()

public:
	ASGPlayerController();

protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintCallable, Category = "Input")
	void AddDefaultMappingContext();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void RemoveDefaultMappingContext();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetGameplayInputEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetLookInputEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetMouseGameOnlyMode();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetMouseGameAndUIInputMode();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	int32 DefaultMappingPriority = 0;
};