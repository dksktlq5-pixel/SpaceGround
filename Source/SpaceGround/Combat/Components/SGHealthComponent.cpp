#include "SGHealthComponent.h"

#include "GameFramework/Actor.h"

USGHealthComponent::USGHealthComponent()
{
	// 체력은 데미지를 받을 때만 계산하므로 Tick 필요 없음
	PrimaryComponentTick.bCanEverTick = false;
}

// 게임 시작 시 체력 초기화 후 Owner의 데미지 이벤트에 연결
void USGHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	MaxHealth = FMath::Max(1.0f, MaxHealth);
	CurrentHealth = MaxHealth;
	bIsDepleted = false;

	// 같은 함수가 중복 등록되지 않게 AddUniqueDynamic 사용
	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->OnTakeAnyDamage.AddUniqueDynamic(
			this, &USGHealthComponent::HandleOwnerTakeAnyDamage);
	}
}

// 게임 종료나 Actor 제거 시 등록했던 데미지 이벤트 해제
void USGHealthComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->OnTakeAnyDamage.RemoveDynamic(
			this, &USGHealthComponent::HandleOwnerTakeAnyDamage);
	}

	Super::EndPlay(EndPlayReason);
}

// 현재 체력을 체력바에서 쓰기 좋은 0~1 값으로 변환
float USGHealthComponent::GetHealthNormalized() const
{
	return MaxHealth > 0.0f
		? FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f)
		: 0.0f;
}

// 최대 체력 설정과 완전 회복을 함께 처리
void USGHealthComponent::SetMaxHealth(const float NewMaxHealth)
{
	// NaN이나 무한대 같은 잘못된 숫자 차단
	if (!FMath::IsFinite(NewMaxHealth))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Health] Ignored non-finite MaxHealth on %s."), *GetNameSafe(GetOwner()));
		return;
	}

	MaxHealth = FMath::Max(1.0f, NewMaxHealth);
	CurrentHealth = MaxHealth;
	bIsDepleted = false;
}

// 최종 데미지만큼 체력을 줄이고 변화 이벤트 전달
float USGHealthComponent::ApplyHealthDamage(const float DamageAmount, AActor* DamageCauser)
{
	// 체력이 이미 0이거나 잘못된 데미지면 무시
	if (bIsDepleted || !FMath::IsFinite(DamageAmount) || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	const float OldHealth = CurrentHealth;
	// 체력이 0 아래나 최대 체력 위로 벗어나지 않게 제한
	CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.0f, MaxHealth);
	const float AppliedDamage = OldHealth - CurrentHealth;

	if (AppliedDamage <= 0.0f)
	{
		return 0.0f;
	}

	UE_LOG(LogTemp, Log, TEXT("[Health] %s took %.1f damage. HP=%.1f/%.1f"),
		*GetNameSafe(GetOwner()), AppliedDamage, CurrentHealth, MaxHealth);
	OnHealthChanged.Broadcast(OldHealth, CurrentHealth, AppliedDamage);

	// 0이 된 순간 한 번만 소진 이벤트 전달
	if (CurrentHealth <= 0.0f)
	{
		bIsDepleted = true;
		UE_LOG(LogTemp, Log, TEXT("[Health] %s health depleted."), *GetNameSafe(GetOwner()));
		OnHealthDepleted.Broadcast(DamageCauser);
	}

	return AppliedDamage;
}

// Unreal의 OnTakeAnyDamage를 컴포넌트 데미지 함수로 전달
void USGHealthComponent::HandleOwnerTakeAnyDamage(AActor* DamagedActor,
	const float DamageAmount, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	if (DamagedActor != GetOwner())
	{
		return;
	}

	ApplyHealthDamage(DamageAmount, DamageCauser);
}
