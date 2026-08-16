#include "TrapBase.h"

#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"


ATrapBase::ATrapBase()
{
	PrimaryActorTick.bCanEverTick = false;


	TriggerBox =
		CreateDefaultSubobject<UBoxComponent>(
			TEXT("TriggerBox")
		);

	TriggerBox->SetupAttachment(SceneRoot);

	TriggerBox->SetBoxExtent(
		FVector(
			100.0f,
			100.0f,
			50.0f
		)
	);

	TriggerBox->SetCollisionEnabled(
		ECollisionEnabled::QueryOnly
	);

	TriggerBox->SetCollisionResponseToAllChannels(
		ECR_Ignore
	);

	TriggerBox->SetCollisionResponseToChannel(
		ECC_Pawn,
		ECR_Overlap
	);

	TriggerBox->SetGenerateOverlapEvents(true);
}


void ATrapBase::BeginPlay()
{
	Super::BeginPlay();

	if (!LoadTrapData())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[TrapBase] 함정 데이터 로드 실패. "
				"Actor=%s"
			),
			*GetNameSafe(this)
		);

		return;
	}


	TriggerBox->OnComponentBeginOverlap
		.AddDynamic(
			this,
			&ATrapBase::OnTriggerBoxBeginOverlap
		);


	/*
	 * 현재 초기 전력 상태를 TriggerBox에 즉시 반영한다.
	 * 이후 코어 파괴/복구 때도 같은 함수가 자동 호출된다.
	 */
	OnOperatingStateChanged(
		CanOperate()
	);


	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"[TrapBase] 초기화 완료. "
			"ID=%s Damage=%.1f CC=%d Duration=%.2f "
			"SlowRate=%.2f MaxTriggers=%d"
		),
		*GetStructureID().ToString(),
		TrapDamage,
		static_cast<int32>(CrowdControlType),
		EffectDuration,
		SlowRate,
		MaxTriggerCount
	);
}


bool ATrapBase::LoadTrapData()
{
	const FSGStructureDefinition* Definition =
		FindStructureDefinition();

	if (!Definition)
	{
		return false;
	}


	TrapDamage =
		FMath::Max(
			0.0f,
			Definition->TrapData.Damage
		);

	CrowdControlType =
		Definition
			->TrapData
			.CrowdControlType;

	EffectDuration =
		FMath::Max(
			0.0f,
			Definition
				->TrapData
				.EffectDuration
		);

	SlowRate =
		FMath::Clamp(
			Definition
				->TrapData
				.SlowRate,
			0.0f,
			1.0f
		);

	MaxTriggerCount =
		FMath::Max(
			1,
			Definition
				->TrapData
				.MaxTriggerCount
		);

	CurrentTriggerCount = 0;

	return true;
}


bool ATrapBase::CanTrigger() const
{
	return
		CanOperate() &&
		!bActivationInProgress &&
		CurrentTriggerCount < MaxTriggerCount;
}


bool ATrapBase::IsValidTrapTarget(
	AActor* OtherActor
) const
{
	if (!IsValid(OtherActor))
	{
		return false;
	}

	if (OtherActor == this)
	{
		return false;
	}

	if (OtherActor->IsActorBeingDestroyed())
	{
		return false;
	}

	if (
		!RequiredTargetTag.IsNone() &&
		!OtherActor->ActorHasTag(
			RequiredTargetTag
		)
	)
	{
		return false;
	}

	return true;
}


void ATrapBase::OnTriggerBoxBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	if (!CanTrigger())
	{
		return;
	}

	if (!IsValidTrapTarget(OtherActor))
	{
		return;
	}


	bActivationInProgress = true;

	ActivateTrap(OtherActor);

	BP_OnTrapActivated(OtherActor);

	FinishTrapActivation();
}


void ATrapBase::ActivateTrap(
	AActor* TargetActor
)
{
	/*
	 * 자식 클래스에서 구현한다.
	 */
}


void ATrapBase::FinishTrapActivation()
{
	++CurrentTriggerCount;

	bActivationInProgress = false;


	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"[TrapBase] 함정 발동 완료. "
			"ID=%s Trigger=%d/%d"
		),
		*GetStructureID().ToString(),
		CurrentTriggerCount,
		MaxTriggerCount
	);


	if (CurrentTriggerCount < MaxTriggerCount)
	{
		return;
	}


	if (IsValid(TriggerBox))
	{
		TriggerBox->SetCollisionEnabled(
			ECollisionEnabled::NoCollision
		);
	}


	SetStructureActive(false);

	SetLifeSpan(
		DestroyDelayAfterFinalTrigger
	);
}


void ATrapBase::HandleStructureDestroyed(
	AActor* DamageCauser
)
{
	if (IsValid(TriggerBox))
	{
		TriggerBox->SetCollisionEnabled(
			ECollisionEnabled::NoCollision
		);
	}

	Super::HandleStructureDestroyed(
		DamageCauser
	);
}


void ATrapBase::OnOperatingStateChanged(
	bool bCanOperateNow
)
{
	Super::OnOperatingStateChanged(
		bCanOperateNow
	);

	if (!IsValid(TriggerBox))
	{
		return;
	}

	TriggerBox->SetGenerateOverlapEvents(
		bCanOperateNow
	);

	TriggerBox->SetCollisionEnabled(
		bCanOperateNow
			? ECollisionEnabled::QueryOnly
			: ECollisionEnabled::NoCollision
	);
}
