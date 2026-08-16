#include "PowerCore.h"

#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "TimerManager.h"


APowerCore::APowerCore()
{
	PrimaryActorTick.bCanEverTick = false;
}


void APowerCore::BeginPlay()
{
	Super::BeginPlay();

	if (!LoadPowerCoreData())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[PowerCore] 전력 데이터 로드 실패. "
				"Actor=%s"
			),
			*GetNameSafe(this)
		);

		return;
	}

	/**
	 * BeginPlay 시점에는 다른 구조물의 BeginPlay가
	 * 아직 끝나지 않았을 수 있다.
	 *
	 * 다음 프레임에 전력망을 계산해
	 * 모든 구조물이 초기화된 뒤 연결되도록 한다.
	 */
	RequestPowerGridRecalculation();
}


bool APowerCore::LoadPowerCoreData()
{
	const FSGStructureDefinition* Definition =
		FindStructureDefinition();

	if (!Definition)
	{
		return false;
	}

	PowerSupply =
		FMath::Max(
			0.0f,
			Definition->PowerData.PowerSupply
		);

	PowerRadius =
		FMath::Max(
			0.0f,
			Definition->PowerData.PowerRadius
		);

	UsedPower = 0.0f;
	ConnectedStructures.Reset();

	/**
	 * 파워코어 자체는 외부 전력을 필요로 하지 않는다.
	 */
	SetPowered(true);
	SetStructureActive(true);

	UE_LOG(
		LogTemp,
		Verbose,
		TEXT(
			"[PowerCore] 전력 데이터 로드 완료. "
			"ID=%s Supply=%.1f PU Radius=%.1f"
		),
		*GetStructureID().ToString(),
		PowerSupply,
		PowerRadius
	);

	return true;
}


void APowerCore::RequestPowerGridRecalculation()
{
	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	FTimerManager& TimerManager = World->GetTimerManager();

	if (TimerManager.IsTimerActive(
		PowerGridRecalculationTimerHandle
	))
	{
		return;
	}

	PowerGridRecalculationTimerHandle =
		TimerManager.SetTimerForNextTick(
			this,
			&APowerCore::RecalculatePowerGrid
		);
}


bool APowerCore::IsInsidePowerRange(
	const FVector& WorldLocation
) const
{
	if (IsDestroyed())
	{
		return false;
	}

	if (!CanOperate())
	{
		return false;
	}

	if (PowerRadius <= 0.0f)
	{
		return false;
	}

	const float DistanceSquared =
		FVector::DistSquared(
			GetActorLocation(),
			WorldLocation
		);

	return
		DistanceSquared <=
		FMath::Square(PowerRadius);
}


void APowerCore::RecalculatePowerGrid()
{
	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	/* 직접 재계산되었다면 BeginPlay에서 예약한 중복 호출을 취소한다. */
	World->GetTimerManager().ClearTimer(
		PowerGridRecalculationTimerHandle
	);
	PowerGridRecalculationTimerHandle.Invalidate();

	/**
	 * 전력망은 Actor Owner 기준으로 구분한다.
	 *
	 * 건설 컴포넌트에서 구조물을 Spawn할 때
	 * SpawnParameters.Owner에 플레이어 캐릭터가 들어가므로
	 * 다른 플레이어의 구조물과 전력망이 섞이지 않는다.
	 */
	AActor* GridOwner = GetOwner();


	// ─────────────────────────────────────────────
	// 같은 소유자의 파워코어 탐색

	TArray<APowerCore*> OwnedPowerCores;

	for (
		TActorIterator<APowerCore> Iterator(World);
		Iterator;
		++Iterator
	)
	{
		APowerCore* PowerCore = *Iterator;

		if (!IsValid(PowerCore))
		{
			continue;
		}

		if (PowerCore->IsDestroyed())
		{
			continue;
		}

		if (PowerCore->GetOwner() != GridOwner)
		{
			continue;
		}

		/**
		 * 기존에 연결된 구조물의 파괴 Delegate를 제거한다.
		 *
		 * 전력망 재분배 후 실제로 연결되는 파워코어가
		 * 다시 Delegate를 등록한다.
		 */
		for (
			AStructureBase* ConnectedStructure :
			PowerCore->ConnectedStructures
		)
		{
			if (!IsValid(ConnectedStructure))
			{
				continue;
			}

			ConnectedStructure->OnDestroyed.RemoveDynamic(
				PowerCore,
				&APowerCore::
					HandleConnectedStructureDestroyed
			);
		}

		PowerCore->ConnectedStructures.Reset();
		PowerCore->UsedPower = 0.0f;

		if (!PowerCore->CanOperate())
		{
			continue;
		}

		OwnedPowerCores.Add(PowerCore);
	}


	// ─────────────────────────────────────────────
	// 같은 소유자의 전력 소비 구조물 탐색

	TArray<AStructureBase*> PowerCandidates;

	for (
		TActorIterator<AStructureBase> Iterator(World);
		Iterator;
		++Iterator
	)
	{
		AStructureBase* Structure = *Iterator;

		if (!IsValid(Structure))
		{
			continue;
		}

		if (Structure->IsDestroyed())
		{
			continue;
		}

		if (Structure->GetOwner() != GridOwner)
		{
			continue;
		}

		/**
		 * 파워코어는 다른 파워코어로부터
		 * 전력을 공급받는 구조물이 아니다.
		 */
		if (Cast<APowerCore>(Structure))
		{
			continue;
		}

		if (Structure->GetStructureID().IsNone())
		{
			continue;
		}

		/**
		 * 전력을 소비하지 않는 구조물도 코어 존재에는 종속된다.
		 * 코어가 하나 이상 작동할 때만 활성화한다.
		 */
		if (!Structure->RequiresCachedPower())
		{
			Structure->SetPowered(
				!OwnedPowerCores.IsEmpty()
			);
			continue;
		}

		/* 최종 분배 결과가 나온 뒤에만 전력 상태를 한 번 변경한다. */
		PowerCandidates.Add(Structure);
	}


	/**
	 * 사용 가능한 파워코어가 없다면
	 * 전력을 요구하는 모든 구조물은
	 * 전원이 꺼진 상태로 유지된다.
	 */
	if (OwnedPowerCores.IsEmpty())
	{
		for (AStructureBase* Structure : PowerCandidates)
		{
			if (IsValid(Structure))
			{
				Structure->SetPowered(false);
			}
		}

		UE_LOG(
			LogTemp,
			Verbose,
			TEXT(
				"[PowerCore] 작동 가능한 파워코어가 없습니다. "
				"Owner=%s"
			),
			*GetNameSafe(GridOwner)
		);

		return;
	}


	// ─────────────────────────────────────────────
	// 전력 우선순위 정렬

	/**
	 * ShutdownPriority가 높은 구조물부터
	 * 전력을 우선 공급한다.
	 *
	 * 현재 설계:
	 * 숫자가 낮을수록 전력 부족 시 먼저 정지한다.
	 *
	 * 따라서 큰 숫자부터 연결한다.
	 */
	PowerCandidates.Sort(
		[](
			const AStructureBase& Left,
			const AStructureBase& Right
		)
		{
			const int32 LeftPriority =
				Left.GetCachedShutdownPriority();

			const int32 RightPriority =
				Right.GetCachedShutdownPriority();

			return LeftPriority > RightPriority;
		}
	);


	// ─────────────────────────────────────────────
	// 전력 분배

	for (AStructureBase* Structure : PowerCandidates)
	{
		if (!IsValid(Structure))
		{
			continue;
		}

		if (Structure->IsDestroyed())
		{
			continue;
		}

		const float Consumption =
			Structure->GetCachedPowerConsumption();

		APowerCore* BestPowerCore = nullptr;
		float BestRemainingPower = -1.0f;
		bool bFoundCoreInRange = false;

		for (APowerCore* PowerCore : OwnedPowerCores)
		{
			if (!IsValid(PowerCore))
			{
				continue;
			}

			if (!PowerCore->IsInsidePowerRange(
				Structure->GetActorLocation()
			))
			{
				continue;
			}

			bFoundCoreInRange = true;

			const float RemainingPower =
				PowerCore->GetRemainingPower();

			if (
				RemainingPower
					+ KINDA_SMALL_NUMBER
				<
				Consumption
			)
			{
				continue;
			}

			/**
			 * 범위 안에 있고 전력이 충분한 코어 중
			 * 잔여 전력이 가장 많은 코어를 선택한다.
			 *
			 * 건설 컴포넌트의 설치 가능 판정과
			 * 같은 선택 기준을 사용한다.
			 */
			if (
				!BestPowerCore ||
				RemainingPower > BestRemainingPower
			)
			{
				BestPowerCore = PowerCore;
				BestRemainingPower = RemainingPower;
			}
		}

		if (!BestPowerCore)
		{
			Structure->SetPowered(false);

			if (bFoundCoreInRange)
			{
				UE_LOG(
					LogTemp,
					Verbose,
					TEXT(
						"[PowerCore] 전력 부족. "
						"Structure=%s Need=%.1f"
					),
					*Structure
						->GetStructureID()
						.ToString(),
					Consumption
				);
			}
			else
			{
				UE_LOG(
					LogTemp,
					Verbose,
					TEXT(
						"[PowerCore] 전력 범위 밖. "
						"Structure=%s"
					),
					*Structure
						->GetStructureID()
						.ToString()
				);
			}

			continue;
		}

		BestPowerCore->UsedPower += Consumption;

		Structure->SetPowered(true);

		BestPowerCore->ConnectedStructures.AddUnique(
			Structure
		);

		/**
		 * 연결 구조물이 파괴되면
		 * 소비 전력을 즉시 다시 계산한다.
		 */
		Structure->OnDestroyed.RemoveDynamic(
			BestPowerCore,
			&APowerCore::
				HandleConnectedStructureDestroyed
		);

		Structure->OnDestroyed.AddUniqueDynamic(
			BestPowerCore,
			&APowerCore::
				HandleConnectedStructureDestroyed
		);

		UE_LOG(
			LogTemp,
			VeryVerbose,
			TEXT(
				"[PowerCore] 구조물 연결. "
				"Core=%s Structure=%s "
				"Consumption=%.1f Used=%.1f/%.1f"
			),
			*GetNameSafe(BestPowerCore),
			*Structure
				->GetStructureID()
				.ToString(),
			Consumption,
			BestPowerCore->UsedPower,
			BestPowerCore->PowerSupply
		);
	}


	// ─────────────────────────────────────────────
	// 결과 출력

	for (APowerCore* PowerCore : OwnedPowerCores)
	{
		if (!IsValid(PowerCore))
		{
			continue;
		}

		UE_LOG(
			LogTemp,
			Verbose,
			TEXT(
				"[PowerCore] 전력망 계산 완료. "
				"Core=%s Connected=%d "
				"Used=%.1f/%.1f Remaining=%.1f"
			),
			*GetNameSafe(PowerCore),
			PowerCore
				->GetConnectedStructureCount(),
			PowerCore->UsedPower,
			PowerCore->PowerSupply,
			PowerCore->GetRemainingPower()
		);
	}
}


void APowerCore::ShutdownPowerGrid()
{
	for (
		AStructureBase* Structure :
		ConnectedStructures
	)
	{
		if (!IsValid(Structure))
		{
			continue;
		}

		Structure->OnDestroyed.RemoveDynamic(
			this,
			&APowerCore::
				HandleConnectedStructureDestroyed
		);

		if (Structure->IsDestroyed())
		{
			continue;
		}

		Structure->SetPowered(false);
	}

	ConnectedStructures.Reset();
	UsedPower = 0.0f;
}


void APowerCore::ShutdownAllOwnedStructures()
{
	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	AActor* GridOwner = GetOwner();

	for (
		TActorIterator<AStructureBase> Iterator(World);
		Iterator;
		++Iterator
	)
	{
		AStructureBase* Structure = *Iterator;

		if (!IsValid(Structure)
			|| Structure == this
			|| Structure->IsDestroyed()
			|| Structure->GetOwner() != GridOwner
			|| Cast<APowerCore>(Structure))
		{
			continue;
		}

		Structure->SetPowered(false);
	}
}


void APowerCore::HandleConnectedStructureDestroyed(
	AActor* DestroyedActor
)
{
	AStructureBase* DestroyedStructure =
		Cast<AStructureBase>(DestroyedActor);

	if (DestroyedStructure)
	{
		ConnectedStructures.Remove(
			DestroyedStructure
		);
	}

	/**
	 * Actor의 파괴 처리 도중에 월드 전체를 탐색하지 않고
	 * 다음 프레임에 안전하게 전력망을 계산한다.
	 */
	RequestPowerGridRecalculation();
}


void APowerCore::HandleStructureDestroyed(
	AActor* DamageCauser
)
{
	UWorld* World = GetWorld();
	AActor* GridOwner = GetOwner();

	APowerCore* ReplacementPowerCore = nullptr;

	/**
	 * 현재 파워코어가 파괴된 다음
	 * 전력망을 재분배할 다른 파워코어를 찾는다.
	 */
	if (World)
	{
		for (
			TActorIterator<APowerCore> Iterator(World);
			Iterator;
			++Iterator
		)
		{
			APowerCore* PowerCore = *Iterator;

			if (!IsValid(PowerCore))
			{
				continue;
			}

			if (PowerCore == this)
			{
				continue;
			}

			if (PowerCore->IsDestroyed())
			{
				continue;
			}

			if (PowerCore->GetOwner() != GridOwner)
			{
				continue;
			}

			if (!PowerCore->CanOperate())
			{
				continue;
			}

			ReplacementPowerCore = PowerCore;
			break;
		}
	}

	/**
	 * 현재 발전기가 공급하던 전력을 먼저 차단한다.
	 */
	ShutdownPowerGrid();

	if (!IsValid(ReplacementPowerCore))
	{
		ShutdownAllOwnedStructures();
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"[PowerCore] 발전기 파괴. "
				"연결 구조물의 전력을 차단하고 "
				"남은 파워코어로 전력망을 재분배합니다. "
				"Core=%s"
		),
		*GetNameSafe(this)
	);

	/**
	 * 파괴 처리는 부모 클래스에서 수행한다.
	 */
	Super::HandleStructureDestroyed(
		DamageCauser
	);

	/**
	 * 남은 파워코어가 있다면 다음 프레임에
	 * 전체 전력망을 다시 계산한다.
	 */
	if (
		World &&
		IsValid(ReplacementPowerCore)
	)
	{
		ReplacementPowerCore->RequestPowerGridRecalculation();
	}
}


int32 APowerCore::GetConnectedStructureCount() const
{
	int32 ValidCount = 0;

	for (
		const AStructureBase* Structure :
		ConnectedStructures
	)
	{
		if (
			IsValid(Structure) &&
			!Structure->IsDestroyed()
		)
		{
			++ValidCount;
		}
	}

	return ValidCount;
}


void APowerCore::DrawPowerRadiusDebug() const
{
	if (!bDrawDebugPowerRadius)
	{
		return;
	}

	const UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	DrawDebugSphere(
		World,
		GetActorLocation(),
		PowerRadius,
		64,
		FColor::Cyan,
		false,
		DebugDrawDuration,
		0,
		3.0f
	);
}
