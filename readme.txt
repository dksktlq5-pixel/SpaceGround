헌호
-combat -> componets에서
healthcomponent를 만들어놓음 보스, 플레이어, 다른액터들을 여기에 체력을 저장할거임

HealthComponent 설계도를 각 Actor가 하나씩 소유하는 구조
EX)  Boss → 자기 HealthComponent → HP 3000
       Player → 자기 HealthComponent → HP 100
       Enemy → 자기 HealthComponent → HP 200

액터 종류를 몰라도 작동함 자신이 붙은 대상만 관리