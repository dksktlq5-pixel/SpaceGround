#include "ShockMine.h"

#include "Kismet/GameplayStatics.h"


AShockMine::AShockMine()
{
	PrimaryActorTick.bCanEverTick = false;
}


void AShockMine::ActivateTrap(
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


	BP_OnStunRequested(
		TargetActor,
		GetEffectDuration()
	);


	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"[ShockMine] 발동. "
			"Target=%s Damage=%.1f Stun=%.2f"
		),
		*GetNameSafe(TargetActor),
		GetTrapDamage(),
		GetEffectDuration()
	);
}