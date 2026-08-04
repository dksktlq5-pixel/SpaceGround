#include "StructureBase.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DataTable.h"


AStructureBase::AStructureBase()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot =
		CreateDefaultSubobject<USceneComponent>(
			TEXT("SceneRoot")
		);

	SetRootComponent(SceneRoot);


	StructureMesh =
		CreateDefaultSubobject<UStaticMeshComponent>(
			TEXT("StructureMesh")
		);

	StructureMesh->SetupAttachment(SceneRoot);

	/*
	 * 기본 구조물은 월드와 충돌한다.
	 *
	 * 세부 충돌 설정은 각 구조물 BP 또는
	 * 자식 클래스에서 변경할 수 있다.
	 */
	StructureMesh->SetCollisionEnabled(
		ECollisionEnabled::QueryAndPhysics
	);

	StructureMesh->SetCollisionProfileName(
		TEXT("BlockAllDynamic")
	);
}


void AStructureBase::BeginPlay()
{
	Super::BeginPlay();

	InitializeStructure();
}


bool AStructureBase::InitializeStructure()
{
	if (bIsInitialized)
	{
		return true;
	}

	const FSGStructureDefinition* Definition =
		FindStructureDefinition();

	if (!Definition)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[StructureBase] 구조물 초기화 실패. "
				"Actor=%s Table=%s Row=%s"
			),
			*GetNameSafe(this),
			*GetNameSafe(StructureDataTable),
			*StructureRowName.ToString()
		);

		return false;
	}


	StructureID = Definition->StructureID;
	StructureCategory = Definition->Category;

	MaxHP = FMath::Max(1.0f, Definition->MaxHP);
	CurrentHP = MaxHP;

	/*
	 * 발전기가 필요하지 않은 구조물은
	 * 처음부터 전력이 공급된 것으로 처리한다.
	 */
	bIsPowered =
		!Definition->PowerData.bRequiresPower;

	bIsStructureActive = true;
	bDestroyHandled = false;
	bIsInitialized = true;

	UpdateStructureState();


	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"[StructureBase] 초기화 완료. "
			"Actor=%s ID=%s Category=%d HP=%.1f "
			"RequiresPower=%s PowerConsumption=%.1f"
		),
		*GetNameSafe(this),
		*StructureID.ToString(),
		static_cast<int32>(StructureCategory),
		CurrentHP,
		Definition->PowerData.bRequiresPower
			? TEXT("True")
			: TEXT("False"),
		Definition->PowerData.PowerConsumption
	);

	return true;
}


const FSGStructureDefinition*
AStructureBase::FindStructureDefinition() const
{
	if (!IsValid(StructureDataTable))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[StructureBase] StructureDataTable이 "
				"설정되지 않았습니다. Actor=%s"
			),
			*GetNameSafe(this)
		);

		return nullptr;
	}

	if (StructureRowName.IsNone())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[StructureBase] StructureRowName이 "
				"설정되지 않았습니다. Actor=%s"
			),
			*GetNameSafe(this)
		);

		return nullptr;
	}


	const FString ContextString =
		FString::Printf(
			TEXT("StructureBase::FindStructureDefinition [%s]"),
			*GetNameSafe(this)
		);

	return StructureDataTable
		->FindRow<FSGStructureDefinition>(
			StructureRowName,
			ContextString,
			true
		);
}


float AStructureBase::TakeDamage(
	float DamageAmount,
	const FDamageEvent& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser
)
{
	const float SuperDamage =
		Super::TakeDamage(
			DamageAmount,
			DamageEvent,
			EventInstigator,
			DamageCauser
		);

	/*
	 * 기본 Super::TakeDamage()는 일반적으로
	 * 넘겨받은 DamageAmount를 반환한다.
	 *
	 * 혹시 0이 반환되는 상황에서도 전달받은
	 * 피해량을 사용할 수 있도록 처리한다.
	 */
	const float FinalDamage =
		SuperDamage > 0.0f
			? SuperDamage
			: DamageAmount;

	return ApplyStructureDamage(
		FinalDamage,
		DamageCauser
	);
}


float AStructureBase::ApplyStructureDamage(
	float DamageAmount,
	AActor* DamageCauser
)
{
	if (!bIsInitialized)
	{
		InitializeStructure();
	}

	if (IsDestroyed())
	{
		return 0.0f;
	}

	if (DamageAmount <= 0.0f)
	{
		return 0.0f;
	}


	const float PreviousHP = CurrentHP;

	CurrentHP = FMath::Clamp(
		CurrentHP - DamageAmount,
		0.0f,
		MaxHP
	);

	const float AppliedDamage =
		PreviousHP - CurrentHP;


	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"[StructureBase] 피해 적용. "
			"ID=%s Damage=%.1f HP=%.1f/%.1f "
			"Causer=%s"
		),
		*StructureID.ToString(),
		AppliedDamage,
		CurrentHP,
		MaxHP,
		*GetNameSafe(DamageCauser)
	);


	BP_OnStructureDamaged(
		AppliedDamage,
		CurrentHP
	);


	if (CurrentHP <= 0.0f)
	{
		HandleStructureDestroyed(DamageCauser);
	}
	else
	{
		UpdateStructureState();
	}


	return AppliedDamage;
}


float AStructureBase::RepairStructure(
	float RepairAmount
)
{
	if (!bIsInitialized)
	{
		InitializeStructure();
	}

	if (IsDestroyed())
	{
		return 0.0f;
	}

	if (RepairAmount <= 0.0f)
	{
		return 0.0f;
	}

	if (CurrentHP >= MaxHP)
	{
		return 0.0f;
	}


	const float PreviousHP = CurrentHP;

	CurrentHP = FMath::Clamp(
		CurrentHP + RepairAmount,
		0.0f,
		MaxHP
	);

	const float AppliedRepair =
		CurrentHP - PreviousHP;


	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"[StructureBase] 수리 적용. "
			"ID=%s Repair=%.1f HP=%.1f/%.1f"
		),
		*StructureID.ToString(),
		AppliedRepair,
		CurrentHP,
		MaxHP
	);


	BP_OnStructureRepaired(
		AppliedRepair,
		CurrentHP
	);

	UpdateStructureState();

	return AppliedRepair;
}


bool AStructureBase::IsDestroyed() const
{
	return
		bDestroyHandled ||
		StructureState == ESGStructureState::Destroyed ||
		CurrentHP <= 0.0f;
}


float AStructureBase::GetHealthPercent() const
{
	if (MaxHP <= 0.0f)
	{
		return 0.0f;
	}

	return FMath::Clamp(
		CurrentHP / MaxHP,
		0.0f,
		1.0f
	);
}


void AStructureBase::SetStructureActive(
	bool bNewActive
)
{
	if (IsDestroyed())
	{
		return;
	}

	if (bIsStructureActive == bNewActive)
	{
		return;
	}


	bIsStructureActive = bNewActive;

	UpdateStructureState();
	OnOperatingStateChanged(CanOperate());


	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"[StructureBase] 활성 상태 변경. "
			"ID=%s Active=%s"
		),
		*StructureID.ToString(),
		bIsStructureActive
			? TEXT("True")
			: TEXT("False")
	);
}


void AStructureBase::SetPowered(
	bool bNewPowered
)
{
	if (IsDestroyed())
	{
		return;
	}

	if (bIsPowered == bNewPowered)
	{
		return;
	}


	bIsPowered = bNewPowered;

	UpdateStructureState();
	OnOperatingStateChanged(CanOperate());

	BP_OnPowerStateChanged(bIsPowered);


	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"[StructureBase] 전력 상태 변경. "
			"ID=%s Powered=%s"
		),
		*StructureID.ToString(),
		bIsPowered
			? TEXT("True")
			: TEXT("False")
	);
}


bool AStructureBase::CanOperate() const
{
	return
		bIsInitialized &&
		!IsDestroyed() &&
		bIsStructureActive &&
		bIsPowered;
}


void AStructureBase::UpdateStructureState()
{
	if (CurrentHP <= 0.0f || bDestroyHandled)
	{
		StructureState =
			ESGStructureState::Destroyed;

		return;
	}


	if (!bIsStructureActive)
	{
		StructureState =
			ESGStructureState::Inactive;

		return;
	}


	if (!bIsPowered)
	{
		StructureState =
			ESGStructureState::Unpowered;

		return;
	}


	/*
	 * HP 40% 이하부터 Damaged 상태로 처리한다.
	 *
	 * 기관총 터렛은 이 상태에서
	 * 연사 속도가 감소하도록 구현할 예정이다.
	 */
	if (GetHealthPercent() <= 0.4f)
	{
		StructureState =
			ESGStructureState::Damaged;

		return;
	}


	StructureState =
		ESGStructureState::Active;
}


void AStructureBase::HandleStructureDestroyed(
	AActor* DamageCauser
)
{
	if (bDestroyHandled)
	{
		return;
	}


	bDestroyHandled = true;
	CurrentHP = 0.0f;
	bIsStructureActive = false;
	bIsPowered = false;

	StructureState =
		ESGStructureState::Destroyed;


	/*
	 * 파괴된 구조물의 충돌을 제거한다.
	 */
	if (IsValid(StructureMesh))
	{
		StructureMesh->SetCollisionEnabled(
			ECollisionEnabled::NoCollision
		);
	}


	OnOperatingStateChanged(false);
	BP_OnStructureDestroyed(DamageCauser);


	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"[StructureBase] 구조물 파괴. "
			"Actor=%s ID=%s Causer=%s"
		),
		*GetNameSafe(this),
		*StructureID.ToString(),
		*GetNameSafe(DamageCauser)
	);


	/*
	 * 파괴 연출을 BP에서 실행할 시간을 조금 남긴다.
	 *
	 * 추후 파괴 잔해 시스템을 만들면
	 * 여기에서 즉시 제거하지 않고 상태만 유지할 수 있다.
	 */
	SetLifeSpan(0.1f);
}


void AStructureBase::OnOperatingStateChanged(
	bool bCanOperate
)
{
	/*
	 * 기본 구조물은 별도 처리가 없다.
	 *
	 * TurretBase 등에서 오버라이드하여
	 * 사격 타이머를 중지하거나 시작한다.
	 */
}