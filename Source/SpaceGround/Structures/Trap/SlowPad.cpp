#include "SlowPad.h"

#include "Kismet/GameplayStatics.h"


ASlowPad::ASlowPad()
{
	PrimaryActorTick.bCanEverTick = false;
}


void ASlowPad::ActivateTrap(
	AActor* TargetActor
)
{
	if (!IsValid(TargetActor))
	{
		return;
	}


	if (GetTrapDamage() > 0.0f)
	{
		UGameplayStatics::ApplyDamage(
			TargetActor,
			GetTrapDamage(),
			nullptr,
			this,
			nullptr
		);
	}


	BP_OnSlowRequested(
		TargetActor,
		GetSlowRate(),
		GetEffectDuration()
	);


	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"[SlowPad] 발동. "
			"Target=%s Damage=%.1f SlowRate=%.2f "
			"Duration=%.2f"
		),
		*GetNameSafe(TargetActor),
		GetTrapDamage(),
		GetSlowRate(),
		GetEffectDuration()
	);
}