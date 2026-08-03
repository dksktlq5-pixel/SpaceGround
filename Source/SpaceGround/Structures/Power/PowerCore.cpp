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

	/*
	 * BeginPlay 시점에는 다른 구조물의 BeginPlay가
	 * 아직 끝나지 않았을 수 있다.
	 *
	 * 다음 프레임에 전력망을 계산해
	 * 모든 구조물이 초기화된 뒤 연결되도록 한다.
	 */
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			this,
			&APowerCore::RecalculatePowerGrid
		);
	}
}


bool APowerCore::LoadPowerCoreData()
{
	const FSGStructureDefinition* Definition =
		FindStructureDefinition();

	if (!Definition)
	{
		return false;
	}

	PowerSupply = FMath::Max(
		0.0f,
		Definition->PowerData.PowerSupply
	);

	PowerRadius = FMath::Max(
		0.0f,
		Definition->PowerData.PowerRadius
	);

	/*
	 * 발전기 자체는 외부 전력을 필요로 하지 않는다.
	 */
	SetPowered(true);
	SetStructureActive(true);

	UE_LOG(
		LogTemp,
		Log,
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


void APowerCore::RecalculatePowerGrid()
{
	if (!CanOperate())
	{
		ShutdownPowerGrid();

		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[PowerCore] 발전기가 작동할 수 없어 "
				"전력망 계산을 중단합니다. Actor=%s"
			),
			*GetNameSafe(this)
		);

		return;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}


	/*
	 * 이전 연결을 먼저 초기화한다.
	 *
	 * MVP에서는 활성 발전기가 한 개라는 전제를 사용한다.
	 */
	ShutdownPowerGrid();

	UsedPower = 0.0f;


	/*
	 * 전력 분배 후보 구조물
	 */
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

		if (Structure == this)
		{
			continue;
		}

		if (Structure->IsDestroyed())
		{
			continue;
		}


		const FSGStructureDefinition* Definition =
			Structure->FindStructureDefinition();

		if (!Definition)
		{
			continue;
		}

		/*
		 * 전력을 요구하지 않는 구조물은
		 * 발전기 관리 대상이 아니다.
		 *
		 * 바리케이드와 비전력 함정 등이 해당한다.
		 */
		if (!Definition->PowerData.bRequiresPower)
		{
			Structure->SetPowered(true);
			continue;
		}


		const float Distance =
			FVector::Dist(
				GetActorLocation(),
				Structure->GetActorLocation()
			);


		if (Distance > PowerRadius)
		{
			Structure->SetPowered(false);
			continue;
		}


		PowerCandidates.Add(Structure);
	}


	/*
	 * ShutdownPriority가 높은 구조물부터
	 * 전력을 우선 공급한다.
	 *
	 * 기존 설계:
	 * 숫자가 낮을수록 먼저 정지
	 *
	 * 따라서 큰 숫자부터 연결한다.
	 */
	PowerCandidates.Sort(
		[](
			const AStructureBase& Left,
			const AStructureBase& Right
		)
		{
			const FSGStructureDefinition*
				LeftDefinition =
					Left.FindStructureDefinition();

			const FSGStructureDefinition*
				RightDefinition =
					Right.FindStructureDefinition();


			const int32 LeftPriority =
				LeftDefinition
					? LeftDefinition
						->PowerData
						.ShutdownPriority
					: 0;

			const int32 RightPriority =
				RightDefinition
					? RightDefinition
						->PowerData
						.ShutdownPriority
					: 0;


			return LeftPriority > RightPriority;
		}
	);


	for (AStructureBase* Structure : PowerCandidates)
	{
		if (!IsValid(Structure))
		{
			continue;
		}


		const FSGStructureDefinition* Definition =
			Structure->FindStructureDefinition();

		if (!Definition)
		{
			Structure->SetPowered(false);
			continue;
		}


		const float Consumption =
			FMath::Max(
				0.0f,
				Definition
					->PowerData
					.PowerConsumption
			);


		const bool bHasEnoughPower =
			UsedPower + Consumption <= PowerSupply;


		if (!bHasEnoughPower)
		{
			Structure->SetPowered(false);

			UE_LOG(
				LogTemp,
				Warning,
				TEXT(
					"[PowerCore] 전력 부족. "
					"Structure=%s Need=%.1f "
					"Remaining=%.1f"
				),
				*Structure->GetStructureID().ToString(),
				Consumption,
				GetRemainingPower()
			);

			continue;
		}


		UsedPower += Consumption;

		Structure->SetPowered(true);
		ConnectedStructures.Add(Structure);


		UE_LOG(
			LogTemp,
			Log,
			TEXT(
				"[PowerCore] 구조물 연결. "
				"Structure=%s Consumption=%.1f "
				"Used=%.1f/%.1f"
			),
			*Structure->GetStructureID().ToString(),
			Consumption,
			UsedPower,
			PowerSupply
		);
	}


	DrawPowerRadiusDebug();


	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"[PowerCore] 전력망 계산 완료. "
			"Connected=%d Used=%.1f/%.1f "
			"Remaining=%.1f"
		),
		GetConnectedStructureCount(),
		UsedPower,
		PowerSupply,
		GetRemainingPower()
	);
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

		if (Structure->IsDestroyed())
		{
			continue;
		}

		Structure->SetPowered(false);
	}


	ConnectedStructures.Empty();
	UsedPower = 0.0f;
}


void APowerCore::HandleStructureDestroyed(
	AActor* DamageCauser
)
{
	/*
	 * 발전기가 파괴되면 연결된 구조물부터 정지시킨다.
	 */
	ShutdownPowerGrid();


	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"[PowerCore] 발전기 파괴. "
			"연결된 모든 구조물의 전력을 차단합니다."
		)
	);


	Super::HandleStructureDestroyed(
		DamageCauser
	);
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