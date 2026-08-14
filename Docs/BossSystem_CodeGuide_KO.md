# SpaceGround 보스 시스템 C++ 코드 학습 가이드

이 문서는 현재 프로젝트에 구현된 보스 C++ 코드가 어떤 역할을 하며, 게임 실행 중 어떤 순서로 호출되는지 초보자 관점에서 설명한다.

현재 구현 범위는 다음과 같다.

- 보스 캐릭터와 AIController 연결
- 플레이어를 Blackboard의 `TargetActor`로 등록
- Behavior Tree에서 플레이어에게 이동
- 공격 거리에서 Single Slam 실행
- 공격 준비 중 플레이어 방향 추적
- Montage Notify에서 공격 방향 고정
- Montage Notify에서 Socket 기반 Sphere 판정 실행
- 공격 한 번에 타깃에게 한 번만 데미지 적용
- 공격 종료 및 쿨다운 후 Behavior Tree로 복귀
- Idle/Walk 애니메이션 속도 계산
- 보스 최대 체력 3000과 데미지 차감

아직 구현하지 않은 기능은 다음과 같다.

- 보스 체력 UI
- 체력 0 이후 사망 또는 Wave 전환
- 약점과 속성 데미지 계산
- Double Slam, Mouth Pounce, Tail Sweep, Vent Reposition
- 공격 선택용 Weighted Random
- 플레이어 체력

---

## 1. 전체 구조

```text
BP_SGBoss
└─ ASGBossCharacter (C++)
   ├─ USGBossCombatComponent
   ├─ USGHealthComponent
   └─ ASGBossAIController
      └─ BT_SGBoss
         ├─ Move To(TargetActor)
         └─ UBTTask_SGBossSingleSlam
```

애니메이션 연결 구조는 다음과 같다.

```text
USGBossAnimInstance
└─ ABP_SGBoss
   ├─ BS_SGBoss_Locomotion (Idle/Walk)
   └─ DefaultGroup.DefaultSlot
      └─ AM_SGBoss_SingleSlam
         ├─ Boss Lock Direction Notify
         └─ Boss Attack Hit Notify
```

공격의 실행 순서는 다음과 같다.

```text
AIController가 TargetActor 설정
→ Behavior Tree의 Move To가 플레이어에게 접근
→ BTTask Single Slam 실행
→ CombatComponent가 이동을 멈추고 Montage 재생
→ 준비 동작 중 플레이어 방향 추적
→ Boss Lock Direction Notify
→ 방향 고정
→ Boss Attack Hit Notify
→ 앞다리 Socket 위치에서 Sphere Overlap
→ 타깃이 Sphere 안에 있으면 ApplyDamage
→ Montage 종료
→ Recovery
→ BT Task 성공 처리
```

보스가 데미지를 받는 순서는 다음과 같다.

```text
무기 또는 아이템
→ UGameplayStatics::ApplyDamage(Boss, Damage, ...)
→ Boss의 OnTakeAnyDamage 이벤트
→ HealthComponent::HandleOwnerTakeAnyDamage()
→ HealthComponent::ApplyHealthDamage()
→ CurrentHealth 감소
→ 로그 및 OnHealthChanged 이벤트
```

---

## 2. Unreal C++에서 자주 보이는 문법

### `UCLASS`

```cpp
UCLASS(Blueprintable)
class SPACEGROUND_API ASGBossCharacter : public ACharacter
```

- Unreal이 이 C++ 클래스를 인식하게 한다.
- `Blueprintable`은 이 클래스를 부모로 Blueprint를 만들 수 있다는 뜻이다.
- `ASGBossCharacter` 앞의 `A`는 Actor 계열 클래스라는 Unreal 명명 규칙이다.

### `USTRUCT`

```cpp
USTRUCT(BlueprintType)
struct FSGSingleSlamAttackData
```

- 서로 관련된 여러 설정값을 하나로 묶는다.
- `BlueprintType`이면 Blueprint와 Details 패널에서 이 자료형을 사용할 수 있다.
- 구조체 이름 앞의 `F`는 Unreal 구조체 명명 규칙이다.

### `UENUM`

```cpp
UENUM(BlueprintType)
enum class ESGBossAttackType : uint8
```

- 공격 종류처럼 정해진 선택지를 만든다.
- `E`는 Enum 명명 규칙이다.
- `uint8`은 Enum 저장에 1바이트를 사용한다.

### `UPROPERTY`

```cpp
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health")
float MaxHealth;
```

- Unreal의 메모리 관리, 저장, Blueprint 및 Details 패널과 연결한다.
- `EditAnywhere`: Editor에서 값 변경 가능.
- `BlueprintReadOnly`: Blueprint에서 읽을 수 있지만 직접 쓰지는 못함.
- `VisibleInstanceOnly`: 실행 중인 인스턴스에서 확인만 가능.
- `Transient`: 저장할 필요가 없는 실행 중 임시 값.

### `UFUNCTION`

```cpp
UFUNCTION(BlueprintPure, Category = "Health")
float GetCurrentHealth() const;
```

- Unreal과 Blueprint가 C++ 함수를 인식하게 한다.
- `BlueprintPure`: 상태를 변경하지 않고 값을 읽는 함수.
- `BlueprintCallable`: Blueprint에서 실행할 수 있는 함수.

### `TObjectPtr`

```cpp
TObjectPtr<USGBossCombatComponent> BossCombatComponent;
```

- Unreal `UObject`를 가리키는 포인터다.
- Unreal의 Garbage Collection이 참조를 안전하게 추적할 수 있게 한다.

### `IsValid`

```cpp
if (!IsValid(CurrentTarget))
```

- 포인터가 단순히 `nullptr`인지뿐만 아니라, Unreal에서 제거 중인 객체인지도 검사한다.

### `const`

```cpp
float GetCurrentHealth() const;
```

- 이 함수가 객체의 멤버 값을 변경하지 않는다는 약속이다.

---

## 3. `SGBossTypes.h` — 공격 종류와 밸런스 데이터

경로: `Source/SpaceGround/CommonData/SGBossTypes.h`

이 파일은 공격을 직접 실행하지 않는다. 공격 시스템 전체가 공유하는 **자료형과 설정값**을 선언한다.

### `ESGBossAttackType`

```cpp
enum class ESGBossAttackType : uint8
{
    None,
    SingleSlam,
    DoubleSlam,
    MouthPounce,
    TailSweep,
    VentReposition
};
```

- 지금 어떤 공격을 실행하는지 구분한다.
- 현재 실제 판정이 구현된 공격은 `SingleSlam`뿐이다.
- 나머지는 앞으로 같은 공통 구조에 추가할 자리다.

### `ESGBossAttackState`

```cpp
Idle
Tracking
DirectionLocked
Hitting
Recovering
```

- `Idle`: 공격하지 않는 상태.
- `Tracking`: 공격 준비 중이며 플레이어 방향을 추적하는 상태.
- `DirectionLocked`: 공격 방향이 고정된 상태.
- `Hitting`: 실제 타격 판정을 실행하는 상태.
- `Recovering`: 공격 후딜레이 상태.

공격 종류와 공격 상태는 다른 개념이다.

```text
공격 종류: Single Slam을 사용한다.
공격 상태: 현재 Single Slam의 Tracking 단계다.
```

### `FSGAttackCommonData`

여러 공격이 공통으로 사용하는 값이다.

```cpp
float Damage = 30.0f;
float MinimumRange = 0.0f;
float ActivationRange = 450.0f;
float RecoveryTime = 1.0f;
float Cooldown = 3.0f;
float TrackingRotationSpeed = 180.0f;
EDamageElement DamageElement = EDamageElement::normal;
TObjectPtr<UAnimMontage> Montage = nullptr;
bool bAllowTimerFallback = true;
int32 MinimumPhase = 1;
float SelectionWeight = 1.0f;
```

- `Damage`: 공격 데미지.
- `MinimumRange`: 공격 가능한 최소 거리.
- `ActivationRange`: 공격을 시작할 수 있는 최대 거리.
- `RecoveryTime`: Montage 종료 후 행동할 수 없는 시간.
- `Cooldown`: 같은 공격을 다시 사용할 수 있을 때까지의 시간.
- `TrackingRotationSpeed`: 준비 동작 중 회전 속도.
- `DamageElement`: 공격 속성. 현재 Single Slam 계산에는 아직 사용하지 않는다.
- `Montage`: Editor에서 지정한 공격 Montage.
- `bAllowTimerFallback`: Montage가 없을 때 임시 타이머 공격을 허용할지 결정한다.
- `MinimumPhase`: 이 공격을 사용할 수 있는 최소 Phase. 아직 공격 선택에는 사용하지 않는다.
- `SelectionWeight`: Weighted Random 공격 선택용 값. 아직 사용하지 않는다.

`ClampMin`, `Units` 같은 `meta`는 Editor 입력을 돕는다.

```cpp
meta = (ClampMin = "0.0", Units = "cm")
```

- 음수 입력을 제한한다.
- Editor에 단위를 cm로 표시한다.

### `FSGSingleSlamAttackData`

Single Slam에만 필요한 설정이다.

- `Common`: 모든 공격이 공유하는 설정.
- `ForwardOffset`: Socket을 찾지 못했을 때 사용하는 기존 전방 판정 위치.
- `HitSocketName`: 타격 중심으로 사용할 Socket 또는 Bone 이름.
- `HitSocketOffset`: Socket 기준 로컬 위치 미세 조정.
- `HitRadius`: Sphere 판정 반지름.
- `FallbackWindupTime`: Montage가 없을 때 타격까지 걸리는 임시 시간.
- `FallbackDirectionLockLeadTime`: 임시 타격보다 몇 초 먼저 방향을 고정할지 결정한다.

현재 `BP_SGBoss`에서는 `Socket_SingleSlamHit`를 사용한다.

---

## 4. `SGBossCharacter` — 보스 본체와 구성품

경로:

- `Source/SpaceGround/Boss/SGBossCharacter.h`
- `Source/SpaceGround/Boss/SGBossCharacter.cpp`

`ASGBossCharacter`는 보스의 몸이다. 이동 가능한 `ACharacter`를 상속하고 필요한 컴포넌트를 보유한다.

### Getter 함수

```cpp
USGBossCombatComponent* GetBossCombatComponent() const;
USGHealthComponent* GetHealthComponent() const;
UBehaviorTree* GetBossBehaviorTree() const;
```

다른 클래스가 보스 내부 구성품에 안전하게 접근하는 통로다.

예를 들어 BT Task는 다음 순서로 CombatComponent를 얻는다.

```text
AIController의 Pawn
→ ASGBossCharacter로 Cast
→ GetBossCombatComponent()
```

### 생성자

```cpp
PrimaryActorTick.bCanEverTick = false;
```

보스 Character 자체에서 매 프레임 실행할 코드가 없으므로 Tick을 끈다. 이동과 AI는 Unreal의 CharacterMovement 및 AI 시스템이 처리한다.

```cpp
BossCombatComponent =
    CreateDefaultSubobject<USGBossCombatComponent>(TEXT("BossCombatComponent"));
```

- 모든 보스 인스턴스에 CombatComponent를 기본 구성품으로 생성한다.
- 그래서 `BP_SGBoss`의 Components 목록에 자동으로 표시된다.

```cpp
HealthComponent =
    CreateDefaultSubobject<USGHealthComponent>(TEXT("HealthComponent"));
HealthComponent->SetMaxHealth(3000.0f);
```

- HealthComponent를 생성한다.
- 보스 기본 최대 체력과 현재 체력을 3000으로 설정한다.

```cpp
AIControllerClass = ASGBossAIController::StaticClass();
AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
```

- 이 보스를 조종할 기본 AIController 클래스를 지정한다.
- 레벨에 직접 배치하거나 Spawn해도 AIController가 자동으로 보스를 Possess한다.

---

## 5. `SGBossAIController` — 플레이어 등록과 Behavior Tree 실행

경로:

- `Source/SpaceGround/Boss/SGBossAIController.h`
- `Source/SpaceGround/Boss/SGBossAIController.cpp`

현재 AIController의 역할은 두 가지다.

1. 보스를 Possess하면 Behavior Tree를 실행한다.
2. 0번 플레이어를 찾아 Blackboard의 `TargetActor`에 저장한다.

### `TargetActorKeyName`

```cpp
const FName ASGBossAIController::TargetActorKeyName(TEXT("TargetActor"));
```

- Blackboard Key 이름을 한곳에 정의한다.
- 문자열을 여러 함수에 반복해서 작성하지 않게 한다.

### `OnPossess`

AIController가 보스를 조종하기 시작할 때 실행된다.

```cpp
const ASGBossCharacter* BossCharacter = Cast<ASGBossCharacter>(InPawn);
```

- 조종 대상 Pawn이 우리가 만든 보스인지 확인한다.
- 실패하면 잘못된 Pawn이 연결된 것이므로 로그를 남기고 종료한다.

```cpp
UBehaviorTree* BehaviorTree = BossCharacter->GetBossBehaviorTree();
```

- `BP_SGBoss`에 지정한 `BT_SGBoss`를 가져온다.

```cpp
RunBehaviorTree(BehaviorTree)
```

- Behavior Tree와 연결된 Blackboard를 초기화하고 트리를 실행한다.

이후 0.1초 간격의 Timer로 플레이어를 찾는다.

```cpp
TargetAcquisitionInterval = 0.1f;
MaxTargetAcquisitionAttempts = 50;
```

- 최대 5초 동안 재시도한다.
- Player Pawn이 BeginPlay 순간 아직 준비되지 않은 경우를 위한 안전장치다.
- 무한히 Timer를 실행하지 않도록 최대 횟수가 있다.

### `TryAcquirePlayerTarget`

```cpp
APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
```

- 싱글플레이의 0번 플레이어 Pawn을 가져온다.

```cpp
Blackboard->SetValueAsObject(TargetActorKeyName, PlayerPawn);
```

- 찾은 플레이어를 Blackboard `TargetActor`에 저장한다.
- `Move To`와 Single Slam Task가 같은 대상을 사용한다.

### 정리 함수

`OnUnPossess`, `EndPlay`, `StopTargetAcquisition`은 AI가 보스에서 분리되거나 게임이 끝날 때 Timer를 제거한다.

---

## 6. `BTTask_SGBossSingleSlam` — BT와 공격 시스템 사이의 연결

경로:

- `Source/SpaceGround/Boss/AI/BTTask_SGBossSingleSlam.h`
- `Source/SpaceGround/Boss/AI/BTTask_SGBossSingleSlam.cpp`

Behavior Tree Task가 직접 데미지를 계산하지는 않는다. CombatComponent에 Single Slam 실행을 요청하고 완료 결과를 기다린다.

### 생성자

```cpp
NodeName = TEXT("Single Slam");
bCreateNodeInstance = true;
bNotifyTaskFinished = true;
```

- BT Editor에 `Single Slam`으로 표시한다.
- Task마다 독립적인 실행 상태와 Timer를 가질 수 있게 인스턴스를 만든다.
- Task 종료 시 정리 함수를 호출할 수 있게 한다.

```cpp
TargetActorKey.AddObjectFilter(..., AActor::StaticClass());
```

- BT Details에서 Actor 타입 Blackboard Key만 선택하게 제한한다.

### `ExecuteTask`

다음 객체를 순서대로 확인한다.

```text
AIController
→ BossCharacter
→ Blackboard
→ TargetActor
→ BossCombatComponent
```

하나라도 유효하지 않으면 `Failed`를 반환한다.

```cpp
CombatComponent->OnBossAttackFinished.AddUniqueDynamic(...);
```

- 공격 완료 이벤트를 구독한다.
- Task가 공격 종료까지 `InProgress` 상태로 기다릴 수 있게 한다.

쿨다운이 남아 있으면 Timer를 걸고 기다린다. 쿨다운이 없으면 `StartSingleSlam()`을 호출한다.

### `HandleAttackFinished`

CombatComponent가 공격 완료 이벤트를 보내면 호출된다.

- 공격 성공: BT Task `Succeeded`.
- 공격 취소 또는 실패: BT Task `Failed`.

이후 Behavior Tree가 다음 실행으로 넘어간다.

### `CleanupTaskState`

- 공격 완료 이벤트 연결 제거.
- 쿨다운 대기 Timer 제거.
- BT가 Task를 강제로 중단했다면 현재 공격도 취소.
- 임시 포인터 초기화.

이 정리가 없으면 이전 공격의 이벤트가 다음 공격에도 호출되거나, 제거된 BT Task의 Timer가 뒤늦게 실행될 수 있다.

---

## 7. `SGBossCombatComponent` — Single Slam의 핵심

경로:

- `Source/SpaceGround/Boss/Components/SGBossCombatComponent.h`
- `Source/SpaceGround/Boss/Components/SGBossCombatComponent.cpp`

이 컴포넌트는 공격 상태, 타깃 추적, Montage, Notify, 판정, 데미지, 회복 시간과 쿨다운을 관리한다.

### 주요 실행 중 데이터

- `AttackState`: 현재 공격 진행 상태.
- `CurrentAttackType`: 현재 공격 종류.
- `CurrentTarget`: 이번 공격의 대상.
- `NextAttackAllowedTimes`: 공격 종류별 다음 사용 가능 시간.
- `ActiveAttackMontage`: 현재 재생 중인 공격 Montage.
- `LockedAttackDirection`: 고정된 공격 방향.
- `bHitExecuted`: 이번 공격에서 타격 Notify가 이미 실행됐는지 여부.
- `bUsingMontageTiming`: Montage/Notify 방식인지 Timer fallback 방식인지 여부.
- `bDirectionLockedByNotify`: 방향 고정 Notify가 실행됐는지 여부.
- `bFinishingAttack`: 공격 종료 함수가 중복 실행되는 것을 방지.

### Tick 사용 방식

CombatComponent는 항상 Tick하지 않는다.

```cpp
PrimaryComponentTick.bCanEverTick = true;
PrimaryComponentTick.bStartWithTickEnabled = false;
```

- 공격 준비 중 방향 추적이 필요할 때만 Tick을 켠다.
- 방향이 고정되거나 공격이 끝나면 다시 끈다.
- Idle 상태에서 불필요한 매 프레임 연산을 하지 않는다.

### `StartAttack`

공통 공격 시작 함수다.

1. `CanStartAttack()`으로 상태, 타깃, 거리, 쿨다운을 검사한다.
2. `ValidateAttackData()`로 잘못된 설정값을 검사한다.
3. AIController의 이동을 멈춘다.
4. 현재 공격용 변수들을 초기화한다.
5. Montage를 재생한다.
6. 공격 준비 방향 추적을 위해 Tick을 켠다.
7. 공격 시작 로그와 Blueprint 이벤트를 발생시킨다.

```cpp
bool StartSingleSlam(AActor* TargetActor)
{
    return StartAttack(ESGBossAttackType::SingleSlam, TargetActor);
}
```

Single Slam 전용 함수는 공통 `StartAttack`에 공격 종류만 전달한다. 공격마다 시작 코드를 복사하지 않기 위한 구조다.

### `CanStartAttack`

다음 조건을 검사한다.

- 현재 다른 공격 중이 아닌가?
- 타깃과 Owner가 유효한가?
- 지원되는 공격 종류인가?
- 타깃이 `MinimumRange ~ ActivationRange` 안에 있는가?
- 해당 공격 쿨다운이 끝났는가?

### `TryPlayAttackMontage`

- Montage가 지정됐는지 확인.
- 보스 Mesh에 AnimInstance가 있는지 확인.
- `Montage_Play()`가 성공했는지 확인.
- Montage 종료 Delegate를 등록.
- Montage가 비정상적으로 끝나지 않을 경우를 대비해 Safety Timer 등록.

Montage가 없고 `bAllowTimerFallback`이 켜져 있으면, Tick에서 `FallbackWindupTime`을 이용해 공격 로직을 시험할 수 있다.

### `UpdateTargetTracking`

공격 준비 중 보스가 플레이어 방향으로 회전하게 한다.

- Z축 차이는 제거하고 수평 방향만 계산한다.
- `TrackingRotationSpeed`를 이용해 한 프레임에 너무 많이 회전하지 않게 한다.
- 방향 고정 Notify 이후에는 호출되지 않는다.

### `NotifyLockAttackDirection`

`Boss Lock Direction` Anim Notify가 호출한다.

- 현재 상태가 `Tracking`일 때만 실행.
- 이미 실행됐다면 무시.
- 현재 보스의 전방 방향을 `LockedAttackDirection`에 저장.
- Montage 방식에서는 더 이상 방향 추적이 필요 없으므로 Tick을 끈다.

### `NotifyExecuteCurrentAttackHit`

`Boss Attack Hit` Anim Notify가 호출한다.

```cpp
if (AttackState == Idle || bHitExecuted)
{
    return;
}
```

- 공격 중이 아니면 무시한다.
- 같은 공격에서 Hit Notify가 중복 실행돼도 한 번만 처리한다.

현재 `SingleSlam`이면 `ExecuteSingleSlamHit()`를 호출한다. 이후 공격 종류가 추가되면 이 분기에 연결한다.

### `ExecuteSingleSlamHit`

Single Slam의 실제 판정 함수다.

기본 판정 중심은 다음과 같다.

```text
보스 위치 + 고정된 공격 방향 × ForwardOffset
```

하지만 `HitSocketName`이 유효하면 다음 위치로 교체한다.

```text
Socket의 월드 Transform
→ HitSocketOffset을 적용한 위치
```

Socket이나 Mesh가 잘못 설정되어 있으면 경고를 남기고 기존 `ForwardOffset` 위치를 사용한다. 잘못된 에셋 설정 때문에 공격 전체가 멈추지 않게 하는 fallback이다.

```cpp
World->OverlapMultiByObjectType(... FCollisionShape::MakeSphere(HitRadius) ...);
```

- 지정된 위치에 Sphere가 있다고 가정하고 겹친 Pawn을 찾는다.
- 현재는 모든 겹친 Actor에게 데미지를 주지 않고 `CurrentTarget`과 같은 Actor만 찾는다.
- 타깃을 찾으면 `UGameplayStatics::ApplyDamage()`를 한 번 호출하고 반복문을 종료한다.

```cpp
DrawDebugSphere(...)
```

- `bDrawDebugAttack`이 켜져 있을 때 판정 Sphere를 표시한다.
- 명중하면 초록색, 빗나가면 빨간색이다.
- 개발 확인용이며 나중에 Shipping 빌드에서는 끄는 것이 좋다.

### Montage 종료와 Recovery

`HandleAttackMontageEnded()`는 다음을 처리한다.

- Montage가 중단되면 공격 실패 처리.
- 방향 고정 Notify가 없었다면 경고.
- 타격 Notify가 없었다면 경고.
- 정상 종료라면 `BeginRecovery()` 호출.

`BeginRecovery()`는 `RecoveryTime`만큼 Timer를 설정한다. 시간이 끝나면 `FinishCurrentAttack(true)`를 호출한다.

### `FinishCurrentAttack`

공격 종료의 공통 정리 지점이다.

- 성공한 공격의 다음 사용 가능 시간을 기록.
- Timer와 Montage Delegate 정리.
- 아직 재생 중인 Montage 중지.
- Tick 끄기.
- 타깃, 상태, 임시 변수 초기화.
- `OnBossAttackFinished` 이벤트 발생.

BT Task는 이 마지막 이벤트를 받아 성공 또는 실패로 끝난다.

---

## 8. 두 개의 Anim Notify — 언제 고정하고 언제 때리는가

경로:

- `Source/SpaceGround/Boss/Animation/Notifies/SGAnimNotify_BossLockDirection.*`
- `Source/SpaceGround/Boss/Animation/Notifies/SGAnimNotify_BossAttackHit.*`

두 Notify는 데미지 계산을 직접 하지 않는다. Montage의 특정 시점에 CombatComponent 함수만 호출한다.

### `Boss Lock Direction`

```text
Montage의 Notify 발생
→ Mesh의 Owner 확인
→ ASGBossCharacter로 Cast
→ CombatComponent 가져오기
→ NotifyLockAttackDirection()
```

### `Boss Attack Hit`

```text
Montage의 Notify 발생
→ Mesh의 Owner 확인
→ ASGBossCharacter로 Cast
→ CombatComponent 가져오기
→ NotifyExecuteCurrentAttackHit()
```

이렇게 분리한 이유는 Animation Asset은 **타이밍**만 담당하고, C++ CombatComponent는 **규칙과 데미지**를 담당하게 하기 위해서다.

다른 공격 Montage에서도 같은 의미의 Notify를 재사용할 수 있다.

---

## 9. `SGBossAnimInstance` — Idle과 Walk를 위한 속도

경로:

- `Source/SpaceGround/Boss/Animation/SGBossAnimInstance.h`
- `Source/SpaceGround/Boss/Animation/SGBossAnimInstance.cpp`

`ABP_SGBoss`의 C++ 부모 클래스다.

### `NativeInitializeAnimation`

AnimInstance가 초기화될 때 `TryGetPawnOwner()`를 이용해 보스 Character를 저장한다.

### `NativeUpdateAnimation`

매 애니메이션 업데이트에서 다음을 계산한다.

```cpp
GroundSpeed = BossCharacter->GetVelocity().Size2D();
```

- `GetVelocity()`: 보스의 현재 이동 속도 벡터.
- `Size2D()`: Z축을 제외한 XY 평면 속력.
- 멈추면 0, 이동하면 양수가 된다.
- 이 값이 `BS_SGBoss_Locomotion`의 Speed 입력으로 들어간다.

보스 참조가 일시적으로 없으면 다시 찾고, 그래도 없으면 `GroundSpeed = 0`으로 안전하게 처리한다.

---

## 10. `SGHealthComponent` — 보스 HP 3000과 데미지 차감

경로:

- `Source/SpaceGround/Combat/Components/SGHealthComponent.h`
- `Source/SpaceGround/Combat/Components/SGHealthComponent.cpp`

현재 보스 체력 테스트를 위해 만들었지만, 체력 그 자체의 규칙은 보스 AI와 무관하므로 공통 위치에 있다.

### 보유 데이터

```cpp
float MaxHealth = 100.0f;
float CurrentHealth = 100.0f;
bool bIsDepleted = false;
```

- 컴포넌트 기본값은 100.
- 보스 생성자에서 `SetMaxHealth(3000.0f)`를 호출하므로 보스는 3000으로 시작한다.

### 이벤트

```cpp
FSGOnHealthChanged OnHealthChanged;
FSGOnHealthDepleted OnHealthDepleted;
```

- `OnHealthChanged`: 체력이 감소할 때 이전 HP, 현재 HP, 실제 감소량 전달.
- `OnHealthDepleted`: 처음 HP가 0이 될 때 DamageCauser 전달.
- 현재 이 이벤트에 사망이나 Wave 로직은 연결하지 않았다.

### 생성자

```cpp
PrimaryComponentTick.bCanEverTick = false;
```

체력은 매 프레임 계산할 필요가 없으므로 Tick을 사용하지 않는다.

### `BeginPlay`

```cpp
MaxHealth = FMath::Max(1.0f, MaxHealth);
CurrentHealth = MaxHealth;
bIsDepleted = false;
```

- 게임 시작 시 최대 체력을 최소 1로 보정.
- 현재 체력을 최대 체력으로 초기화.
- 체력 소진 상태 초기화.

```cpp
OwnerActor->OnTakeAnyDamage.AddUniqueDynamic(
    this, &USGHealthComponent::HandleOwnerTakeAnyDamage);
```

- Owner인 보스가 Unreal 표준 데미지를 받을 때 이 컴포넌트가 자동으로 알림을 받는다.
- `AddUniqueDynamic`은 같은 함수를 중복 등록하지 않는다.

### `EndPlay`

`BeginPlay`에서 등록한 데미지 이벤트를 제거한다. PIE 종료나 Actor 제거 후 잘못된 콜백이 남지 않게 하는 수명 관리 코드다.

### `GetHealthNormalized`

```cpp
CurrentHealth / MaxHealth
```

결과를 `0.0 ~ 1.0`으로 제한한다. 나중에 Progress Bar의 Percent에 사용할 수 있다.

### `SetMaxHealth`

- `NaN`, 무한대 같은 비정상 숫자는 무시한다.
- 최소 최대 체력은 1이다.
- 최대 체력 설정과 동시에 현재 체력도 완전히 채운다.

현재는 보스 생성 시 한 번만 호출하므로 문제가 없다. 나중에 Wave 시스템에서 최대 체력 변경과 회복을 서로 다르게 처리해야 한다면 아래처럼 분리하는 것이 좋다.

```text
SetMaxHealth()  → 최대 체력 변경
ResetHealth()   → 현재 체력을 최대로 회복
```

### `ApplyHealthDamage`

다음 경우에는 데미지를 무시한다.

- 이미 체력이 0인 경우.
- 데미지가 0 이하인 경우.
- 데미지가 `NaN` 또는 무한대인 경우.

```cpp
CurrentHealth = FMath::Clamp(
    CurrentHealth - DamageAmount,
    0.0f,
    MaxHealth);
```

- 체력에서 데미지를 뺀다.
- 결과가 음수가 되거나 최대값보다 커지지 않게 제한한다.

```cpp
AppliedDamage = OldHealth - CurrentHealth;
```

실제로 감소한 체력을 계산한다. HP가 10일 때 30 데미지를 받아도 실제 감소량은 10이다.

체력이 0이면 `bIsDepleted`를 켜고 `OnHealthDepleted` 이벤트를 한 번 발생시킨다. 보스 사망은 아직 구현하지 않았다.

### `HandleOwnerTakeAnyDamage`

Unreal 표준 데미지 이벤트를 컴포넌트의 `ApplyHealthDamage` 함수로 전달하는 연결 함수다.

현재 아이템 테스트에서 기대하는 로그는 다음과 같다.

```text
[Health] BP_SGBoss... took 20.0 damage. HP=2980.0/3000.0
```

---

## 11. C++과 Editor Asset의 역할 분담

### C++ 담당

- 공격 가능 조건.
- 공격 상태 전환.
- 방향 추적 및 고정.
- 쿨다운과 Recovery.
- 판정 및 데미지.
- 중복 공격 방지.
- 잘못된 참조와 설정에 대한 예외 처리.
- 체력 차감.

### Editor/Blueprint 담당

- Skeletal Mesh와 Skeleton.
- `BP_SGBoss`의 Mesh, Anim Class, Behavior Tree 지정.
- `ABP_SGBoss`의 Blend Space와 Slot.
- 공격 Montage 지정.
- Montage Notify의 정확한 프레임.
- `Socket_SingleSlamHit`의 위치.
- 데미지, 거리, 반지름, 쿨다운 같은 밸런스 값.

구매한 보스 에셋의 Bone 이름과 구조는 C++ 공격 규칙과 분리한다. Socket 이름은 Details 패널에서 지정하며, 잘못된 경우 기존 전방 위치로 fallback한다.

---

## 12. 현재 코드에서 의도적으로 남겨 둔 확장 지점

### 공격 Enum에 아직 구현되지 않은 공격이 있음

공통 구조를 먼저 만든 뒤 `DoubleSlam`, `MouthPounce`, `TailSweep`, `VentReposition`을 추가하기 위한 자리다. Enum에 존재한다고 실제 공격이 구현된 것은 아니다.

### `MinimumPhase`, `SelectionWeight`

향후 Phase별 공격 해금과 Weighted Random 공격 선택을 위한 값이다. 현재 Single Slam BT에서는 사용하지 않는다.

### `DamageElement`

향후 속성 시스템을 연결하기 위한 값이다. 현재 HealthComponent는 계산이 끝난 최종 데미지만 차감한다.

### Health 이벤트

향후 UI와 Wave 시스템이 HealthComponent의 이벤트를 구독할 수 있다. HealthComponent 자체에는 보스 전용 Wave 규칙을 넣지 않는다.

---

## 13. 현재 제한사항과 주의점

1. AI는 `GetPlayerPawn(this, 0)`을 사용하므로 싱글플레이 0번 플레이어만 대상으로 한다.
2. 현재 Boss Perception이나 Line of Sight는 사용하지 않고 플레이어를 항상 Target으로 등록한다.
3. 실제 공격 판정이 구현된 공격은 Single Slam뿐이다.
4. Single Slam은 Sphere에 겹친 모든 적이 아니라 `CurrentTarget` 한 명에게만 데미지를 준다.
5. `bDrawDebugAttack`은 개발 중에만 켜고 최종 빌드 전에는 꺼야 한다.
6. HealthComponent는 HP가 0이 되어도 보스를 죽이거나 AI를 정지시키지 않는다.
7. 체력 회복 기능은 아직 없다.
8. `SetMaxHealth()`는 현재 최대 체력 설정과 완전 회복을 동시에 한다.
9. 플레이어 무기나 아이템이 Unreal의 `ApplyDamage` 계열 함수를 호출해야 HealthComponent가 자동으로 데미지를 받는다.

---

## 14. Output Log 읽는 방법

### 정상 AI 시작

```text
[BossAI] Target acquired. Boss=... Target=...
```

### 정상 Single Slam

```text
[BossCombat] Attack started. Type=1 Target=...
[BossCombat] Attack direction locked. Type=1.
[BossCombat] Single Slam hit ... for 30.0 damage.
```

### 보스 체력 감소

```text
[Health] BP_SGBoss... took 20.0 damage. HP=2980.0/3000.0
```

### 설정 확인이 필요한 경고

```text
No Montage assigned
Failed to play Montage
Montage ended without LockDirection Notify
Montage ended without AttackHit Notify
Single Slam hit socket is invalid
Attack timed out and was cancelled
```

경고가 나오면 문장에 표시된 Montage, Notify, Socket 또는 공격 상태부터 확인한다.

---

## 15. 이 코드를 공부할 때 권장 읽기 순서

1. `SGBossCharacter.cpp`에서 보스가 어떤 컴포넌트를 갖는지 본다.
2. `SGBossTypes.h`에서 공격 데이터와 상태를 본다.
3. `SGBossAIController.cpp`에서 플레이어를 Blackboard에 넣는 과정을 본다.
4. `BTTask_SGBossSingleSlam.cpp`에서 BT가 공격을 요청하는 과정을 본다.
5. `SGBossCombatComponent.cpp`의 `StartAttack()`을 본다.
6. 두 Anim Notify가 CombatComponent의 어떤 함수를 호출하는지 본다.
7. `ExecuteSingleSlamHit()`에서 Socket 판정과 `ApplyDamage()`를 본다.
8. `FinishCurrentAttack()`에서 상태와 Timer가 정리되는 것을 본다.
9. `SGHealthComponent.cpp`에서 받은 데미지가 HP로 반영되는 과정을 본다.
10. 마지막으로 `SGBossAnimInstance.cpp`에서 이동 속도가 Blend Space에 전달되는 방식을 본다.

이 순서로 읽으면 파일을 개별적으로 외우는 대신 실제 게임 실행 흐름을 따라가며 이해할 수 있다.
