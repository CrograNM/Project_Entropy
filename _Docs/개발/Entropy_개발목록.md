# Project Entropy — 개발 목록

> 기준 커밋: `81738fb` · Source 11,250줄
> 관련 문서: `_Docs/개발/Entropy_Memo.txt`, `_Docs/기획/Entropy 기획.txt`, `Project_Entropy_리팩토링_체크리스트.md`

---

## 진행 순서 요약

| 단계 | 내용 | 규모 | 선행 단계 |
|:--:|---|:--:|:--:|
| **0** | 즉시 수정 (동작 불일치 2건) | 반나절 | — |
| **1** | 전투 생명주기 (전투 종료 · 드로우 룰) | 반나절~1일 | — |
| **2** | 스킬 실행 구조 재편 | 1~2일 | — |
| **3** | 상태이상 시스템 | 2~3일 | 2 |
| **4** | 타일 속성 시스템 | 3~4일 | 2, 3 |
| **5** | 시각화 정리 | 1일 | 4 |
| **6** | 카드 수치 서버 권위화 + 동적 수치 | 2~3일 | 1 |
| **7** | 카드 표현 축 확장 | 3~5일 | 3, 6 |
| **8** | 유물 · 마석 | 미산정 | 6, 7 |
| **9** | 대기 · 잔여 정리 | 수시 | — |

---

# 단계 0 — 즉시 수정

현재 코드에 남아 있는 동작 불일치 2건.

### 0-1. 즉발 스킬 스플라인 미표시

**작업** — 투사체가 없는 페이즈(`ProjectileSpeed <= 0`)일 때 궤적 스플라인을 그리지 않도록 판정 추가. 판정 함수는 `CanBeBlocked` / `IsPiercingProjectile`과 같은 위치에 둔다.

**배경** — 현재 `Solve`가 즉발 페이즈에도 총구→조준점 2점 직선을 채워 반환하고, 시각화가 이를 그대로 그린다. 파도·장판류 즉발 스킬에 불필요한 직선 화살표가 표시된다.

**대상** — `Public/CardSystem/PE_SkillTrajectory.h`, `Private/Components/ACTargetingVisualizerComponent.cpp`

**완료 기준** — 즉발 스킬 조준 시 범위 타일만 칠해지고 화살표는 표시되지 않음. 투사체 스킬은 기존과 동일.

---

### 0-2. 힐 이펙트 모듈 추가

**작업** — `UPE_SkillEffect_Heal` 추가. `UACStatComponent::Heal`을 호출한다.

**배경** — `UPE_SkillData::BaseHeal` 필드는 존재하지만 적용 모듈이 없어 `GetFormattedDescription`의 UI 텍스트에만 쓰인다. 현재 모듈은 `Damage`, `Push` 2종.

**대상** — `Public/CardSystem/PE_SkillEffectModule.h`, `Private/CardSystem/PE_SkillEffectModule.cpp`

**완료 기준** — `Snap_Ally` / `Self` 대상 회복 카드가 데이터 에셋만으로 동작.

---

# 단계 1 — 전투 생명주기

`BattleStart`와 `EnvironmentTurn`은 열거형과 주석만 있고 실제 처리가 없는 통과 페이즈다. 이 단계에서 두 페이즈를 실제 동작하게 만든다.

### 1-1. 팀 집계를 PlayerState 기준으로 전환

**작업** — `EvaluateTurnEnd`와 턴 시작 레디 리셋의 팀 소속 판정을 `PC->GetPawn()` → `APE_PlayerState` 기준으로 변경.

**배경** — 현재는 Pawn을 통해 팀을 판정한다. 플레이어 사망 시 Pawn이 0.5초 후 파괴되어 집계에서 빠지고, 팀 전원 사망 시 `TotalPlayerCount == 0`이 되어 `ReadyTeamPlayerCount >= TotalPlayerCount` 조건이 성립하지 않는다. → 턴이 넘어가지 않음.

**대상** — `Private/Core/PE_TurnManagerComponent.cpp` (`EvaluateTurnEnd`, 턴 시작 레디 리셋 루프)

**완료 기준** — 플레이어가 사망한 상태에서도 턴이 정상적으로 넘어간다.

> 해당 단계에서 PlayerController의 턴 조작 코드도 없애는 방향으로 리팩토링 한다. (오로지 입력 담당을 위해)
>
> 턴 조작은 별도의 클래스에서 담당하거나 GameState의 TurnManager가 담당하도록 한다.

---

### 1-2. 전투 종료 판정

**작업**
1. `UACStatComponent::OnDeath` → GameMode에 사망 통지 경로 추가
2. GameMode에서 팀별 생존자 검사
3. 한쪽 팀 전멸 시 `ChangePhase(EPEBattlePhase::BattleEnd)`
4. `BattleEnd` 진입 시 입력 차단 및 결과 전달용 델리게이트 방송

**배경** — `EPEBattlePhase::BattleEnd`는 열거형에만 존재하고 어디서도 진입하지 않는다. 승패 판정 자체가 없다.

**대상** — `Private/Core/PE_BattleGameMode.cpp`, `Private/Core/PE_TurnManagerComponent.cpp`, `Private/Characters/PE_CharacterBase.cpp`

**완료 기준** — 적 전멸 시 `BattleEnd` 진입 + 입력 차단. 플레이어 팀 전멸 시에도 동일.

**후속 (별도 작업)** — 기획 문서의 전술 평가·랭크·보상 연출은 `BattleEnd` 진입 이후에 붙는다. 이 단계에서는 진입점과 델리게이트까지만.

---

### 1-3. 드로우 룰 구현

**작업**
1. `BattleStart` 페이즈에서 덱 초기화 + 초기 드로우
2. `TeamTurn` 진입 시 턴당 드로우
3. `APE_PlayerController::BeginPlay`의 `TestStartingDeck` 초기화 제거 (GameMode 지시로 이전)
4. 테스트용 `OnTestDrawCard`는 치트로 유지

**배경** — `DrawCards`는 D키 테스트 함수에서만 호출된다. 덱 초기화도 PlayerController의 `BeginPlay`에서 `TestStartingDeck`으로 처리하고 있어, 기획 문서의 *"전투 맵 진입 시 GameMode가 덱 초기화 및 초기 5장 드로우 지시"* 와 어긋난다. 드로우 연출은 `701359c`로 준비되어 있다.

**대상** — `Private/Core/PE_TurnManagerComponent.cpp`, `Private/Core/PE_BattleGameMode.cpp`, `Private/Core/PE_PlayerController.cpp`, `Private/Components/ACDeckManagerComponent.cpp`

**완료 기준** — 전투 진입 시 초기 5장이 자동 드로우되고, 매 턴 시작 시 규정 수량이 드로우된다.

---

### 1-4. 손패 관련 수치를 스탯으로 이전

**작업** — `UACDeckManagerComponent::MaxHandSize`와 턴당 드로우 수량을 `UACStatComponent`로 이전.

**배경** — 거점 업그레이드(*탐구자의 자질*)가 건드릴 축이므로 캐릭터 스탯에 위치해야 한다.

**대상** — `Public/Components/ACStatComponent.h`, `Public/Components/ACDeckManagerComponent.h`

**완료 기준** — 스탯 값 변경으로 최대 손패와 턴당 드로우가 바뀐다.

---

# 단계 2 — 스킬 실행 구조 재편

`UACSkillComponent::CommitQueuedSkill`이 281줄(230~510)이며 3중 중첩 람다(`ExecutePhaseFunc` → `ExplodeFunc` → `ApplyHitFunc`)로 구성되어 있다. 폭발 → 피격 → 모듈 적용 체인이 즉발 경로와 투사체 경로에 각각 구현되어 있다.

| 단계 | 즉발 경로 | 투사체 경로 |
|---|---|---|
| 폭발 연출 | `ACSkillComponent.cpp:470` | `PE_SkillActionActor.cpp:216` |
| `HitDelay` 타이머 | `ACSkillComponent.cpp:475` | `PE_SkillActionActor.cpp:224` |
| 모듈 적용 | `ACSkillComponent.cpp:453` | `PE_SkillActionActor.cpp:172`(관통) · `:243`(일반) |
| 피격 연출 | `ACSkillComponent.cpp:457` | `:177` · `:256` |

`Module->ApplyEffects` 호출부가 3곳, 피격 연출이 3곳이다. P2-1(스윕 궤적 2벌) · P2-2(밀치기 2벌)와 같은 구조이며, 한쪽만 수정하면 즉발 스킬과 투사체 스킬의 동작이 갈라진다.

프로젝트에는 이미 같은 형태의 해법이 세 번 적용되어 있다. 이 단계는 그 패턴을 스킬 실행에 적용한다.

| 규칙 (정적 함수) | 결과 (구조체) |
|---|---|
| `FPETargetRules` | `FPETargetCandidate` |
| `FPESkillTrajectory` | `FPESkillTrajectoryResult` |
| `FPEPushResolver` | `FPEPushStep` |

**선행 관계** — 단계 4(타일 속성)와 단계 6(동적 수치)이 모두 이 경로를 수정한다. 이 단계를 시그니처 확장으로만 처리하면 같은 함수를 세 번 고치게 된다.

---

### 2-0. `EPESkillTargetType` 2필드 분리

**작업** — 한 열거형에 섞여 있는 두 축을 별도 필드로 쪼갠다.

```
EPESkillAimMode     조준 방식   Tile / SnapCharacter / Self / None(조준 없음)
EPESkillTargetTeam  대상 필터   Enemy / Ally / Any
```

기존 5개 값을 두 필드 조합으로 옮긴다.

| 기존 값 | AimMode | TargetTeam |
|---|---|---|
| `Tile` | Tile | Enemy |
| `Snap_Enemy` | SnapCharacter | Enemy |
| `Snap_Ally` | SnapCharacter | Ally |
| `Self` | Self | Ally |
| `All_Enemies` | None | Enemy |

**배경** — 조준 방식과 대상 필터가 한 열거형에 눌려 있어 조합을 표현할 수 없다. "아군 전체", "타일 지정 + 아군 전용"이 불가능하고, 값을 추가하면 교차곱만큼 늘어난다. 0-2(힐 모듈)가 `Snap_Ally` 하나만 쓸 수 있는 상태다.

**대상** — `Public/CardSystem/PE_DataTypes.h`, `Public/CardSystem/PE_SkillData.h`, `Public/Combat/PE_TargetRules.h`(4곳), `Private/Combat/PE_TargetRules.cpp`(12곳), `Private/Components/ACSkillComponent.cpp`(7곳), `Private/Components/ACCardInteractionComponent.cpp`(2곳), `Private/Components/ACTargetingVisualizerComponent.cpp`, `Private/Components/PE_CardCastComponent.cpp`, `Private/Characters/PE_EnemyBase.cpp`

**주의 — 데이터 마이그레이션 필요.** 열거형 값이 바뀌므로 기존 스킬 데이터 에셋의 `TargetType`이 유실된다. 둘 중 하나를 택한다.

1. 기존 `TargetType` 필드를 남겨둔 채 새 필드 2개를 추가하고, `UPE_SkillData::PostLoad`에서 위 표대로 이관한 뒤 다음 커밋에서 구 필드를 제거
2. `DefaultEngine.ini`에 `+EnumRedirects`를 작성

스킬 에셋 수가 적으면 1번이 안전하다.

**완료 기준** — "아군 전체 회복" 스킬이 코드 수정 없이 데이터만으로 동작한다.

**선행 관계** — 2-5(`TargetType` 분기 통합)보다 **먼저** 진행한다. 순서를 바꾸면 정리한 분기를 다시 손대게 된다.

---

### 2-1. `FPESkillPhasePlan` 결과 구조체 + 정적 리졸버

**작업** — 페이즈 1건을 "지금 전장"에 대고 푼 결과를 담는 구조체와, 그것을 만드는 상태 없는 정적 함수를 추가한다.

```
FPESkillPhasePlan
├─ FIntPoint                 ImpactPos          // Line 끝단 보정 + 막힘 판정까지 끝난 최종 착탄 칸
├─ FVector                   ImpactLocation
├─ FVector                   MuzzleLocation
├─ FRotator                  CasterRotation
├─ APE_CharacterBase*        ImpactCharacter    // 막혀서 부딪힌 대상 (없으면 null)
├─ TSet<FIntPoint>           AffectedPositions  // 영향 타일 집합
├─ TSet<APE_CharacterBase*>  AffectedTargets
└─ float                     Damage             // BaseDamage × DamageMultiplier
```

```
FPEPhaseResolver::Resolve(World, Grid, Caster, CasterPos, TargetPos, Range, Phase, TargetType)
```

**배경** — 현재 이 값들은 `ExecutePhaseFunc` 람다 안의 지역 변수 6개(`PhaseTargetLoc`, `PhaseTargetChar`, `MuzzleLoc`, `bHasMuzzleLoc`, `ExactRotation`, `AffectedTargets`)로 흩어져 있고, `TargetPos`는 `mutable` 캡처로 스코프를 넘나들며 변형된다.

**대상** — `Public/CardSystem/PE_SkillPhasePlan.h`(신규), `Private/CardSystem/PE_SkillPhasePlan.cpp`(신규)

**완료 기준** — 서버 실행과 클라 예측이 같은 `Resolve`를 호출해 동일한 `FPESkillPhasePlan`을 얻는다.

---

### 2-2. 폭발 → 피격 → 모듈 체인을 단일 경로로 통합

**작업** — 위 표의 두 경로를 하나로 합친다. 투사체 액터는 "도착했다"만 보고하고, 폭발 연출 · `HitDelay` 대기 · 모듈 적용 · 피격 연출 · 액션 큐 토큰 반납은 통합된 실행자 한 곳에서 처리한다.

**대상** — `Private/Components/ACSkillComponent.cpp`, `Private/CardSystem/PE_SkillActionActor.cpp`

**완료 기준** — `Module->ApplyEffects` 호출부가 1곳으로 줄어든다. 즉발 스킬과 투사체 스킬이 같은 함수를 통과한다. 관통 투사체의 순차 타격은 기존 동작을 유지한다.

---

### 2-3. `ApplyEffects`에 페이즈 계획 전달

**작업** — `UPE_SkillEffectModule::ApplyEffects` 시그니처를 개별 인자 6개에서 `const FPESkillPhasePlan&` 전달로 교체한다. 기존 모듈 2종(`Damage`, `Push`)을 맞춘다.

**배경** — 현재 모듈이 받는 것은 캐릭터 집합과 단일 좌표뿐이어서, 타일을 대상으로 하는 모듈(단계 4의 타일 속성 부여)이 AoE 계산을 다시 수행해야 한다.

**대상** — `Public/CardSystem/PE_SkillEffectModule.h`, `Private/CardSystem/PE_SkillEffectModule.cpp`

**완료 기준** — 모듈 내부에서 `GetAffectedGridPositions`를 호출하지 않고 영향 타일 목록을 얻을 수 있다.

---

### 2-4. `InitializeActionActor` 인자를 구조체로

**작업** — 인자 11개(`Caster`, `PhaseTargetChar`, `PhaseTargetLoc`, `SkillData`, `PhaseIndex`, `FinalDamage`, `LogIDToClear`, `PhaseToken`, `AffectedTargets`, `CasterPos`, `TargetPos`)를 `FPESkillPhasePlan` + 토큰 정보 구조체로 교체한다.

**대상** — `Public/CardSystem/PE_SkillActionActor.h`, `Private/CardSystem/PE_SkillActionActor.cpp`, `Private/Components/ACSkillComponent.cpp:428`

---

### 2-5. `TargetType` 분기 통합

**작업** — `CommitQueuedSkill` 안에서 `Self` / `All_Enemies` / 그 외 분기가 회전 결정 · 대상 수집 · 총구 결정 세 곳에 반복된다. 대상 수집은 `FPEPhaseResolver`로 옮기고, 나머지는 분기 1회로 정리한다.

**대상** — `Private/Components/ACSkillComponent.cpp`

---

### 2-6. `-999` 리터럴 교체

**작업** — `FIntPoint(-999, -999)` 리터럴을 `PEGridMath::InvalidGridPos()`로 교체한다.

**배경** — `PEGridMath::InvalidCoord = -999`와 `InvalidGridPos()`가 이미 있고 `ACSkillComponent.cpp`도 이를 사용하지만, 같은 파일 안에 리터럴이 5곳 남아 있다. 전역으로는 10곳이다.

**대상** — `ACSkillComponent.cpp`(153, 283, 351, 359, 385, 405), `ACTargetingVisualizerComponent.cpp`(80, 133) 및 헤더(79), `PE_PlayerCharacter.cpp:74`, `ACGridMovementComponent.cpp:17`, `PE_PlayerController.cpp:534`

**완료 기준** — `grep -rn "\-999" Source/`에서 `PE_GridMath.h`의 정의와 `PE_SkillData.cpp`의 `999999`(무관한 최소/최대 초기값)만 남는다.

---

# 단계 3 — 상태이상 시스템

현재 캐릭터 상태는 `UACStatComponent`의 HP / MaxHP / AP / MaxAP / MoveRange와 `APE_CharacterBase::bIsPushable`뿐이다. 기획 문서가 요구하는 감전 · 빙결 · 실명 · 침묵 · 치유불가 · 기절 · 화상 · 정화를 담을 구조가 없다.

### 3-1. `UPE_StatusComponent` 신규

**작업** — `APE_CharacterBase`에 부착하는 상태이상 컴포넌트 생성.

```
FPEStatusEffect
├─ FGameplayTag  StatusTag       // Status.* 태그
├─ int32         Stacks          // 중첩 수
├─ int32         RemainingTurns  // 남은 턴 (-1 = 영구)
└─ AActor*       Source          // 부여 주체
```

**구현 범위**
- `TArray<FPEStatusEffect>` 보관 + 리플리케이션
- 부여 / 제거 / 중첩 / 갱신 API
- 태그 질의 API (`HasStatus`, `GetStackCount`)
- 변경 시 UI 갱신용 델리게이트

**대상** — `Public/Components/PE_StatusComponent.h`, `Private/Components/PE_StatusComponent.cpp`, `Public/Characters/PE_CharacterBase.h`

**완료 기준** — 서버에서 부여한 상태이상이 클라이언트에 복제되고 태그로 질의된다.

**비고** — GAS는 도입하지 않는다. 기존 `UPE_SkillEffectModule` 체계를 유지한다.

---

### 3-2. `Status.*` 게임플레이 태그 등록

**작업** — `PE_GameplayTags`에 상태이상 태그 계층 추가.

| 분류 | 태그 |
|---|---|
| 행동 제약 | `Status.Control.Stun`, `Status.Control.Root`, `Status.Control.Silence`, `Status.Control.Blind` |
| 지속 피해 | `Status.DoT.Burn`, `Status.DoT.Shock` |
| 수치 변조 | `Status.Mod.Vulnerable`, `Status.Mod.Amplify` |
| 회복 제약 | `Status.Deny.Heal` |
| 보호 | `Status.Guard.Ward` |

**배경** — `Element.*` 계층(Normal 4종 · Special 4종 · Complex 2종 · 상위 지형 5종)은 이미 네이티브 태그로 등록되어 있다.

**대상** — `Private/Core/PE_GameplayTags.cpp`, `Public/Core/PE_GameplayTags.h`

---

### 3-3. `EnvironmentTurn` 페이즈 처리

**작업** — 환경 페이즈에서 다음을 처리한다.
1. 모든 캐릭터의 상태이상 `RemainingTurns` 감소 및 만료 제거
2. 지속 피해(`Status.DoT.*`) 적용
3. 처리 완료까지 액션 큐 대기 (`BeginAction` / `EndAction` 사용)

**배경** — `EnvironmentTurn`은 주석에 *"환경 페이즈 (원소 타일 지속시간 감소, 화상 데미지 등)"* 로 역할이 명시되어 있으나, 현재는 `EndCurrentPhase()`만 호출하고 통과한다.

**대상** — `Private/Core/PE_TurnManagerComponent.cpp`

**완료 기준** — 화상 3턴을 부여하면 3턴간 피해가 들어오고 4턴째에 사라진다. 처리 중 턴이 넘어가지 않는다.

---

### 3-4. 상태이상 부여 모듈

**작업** — `UPE_SkillEffect_ApplyStatus` 추가. 에디터에서 태그 · 스택 · 지속 턴을 지정한다.

**대상** — `Public/CardSystem/PE_SkillEffectModule.h`

**완료 기준** — 데이터 에셋만으로 상태이상을 부여하는 카드를 만들 수 있다.

---

### 3-5. 상태이상의 행동 제약 반영

**작업** — 각 제약 태그를 실제 행동 경로에 연결한다.

| 태그 | 반영 위치 |
|---|---|
| `Status.Control.Stun` | 턴 진입 시 행동 스킵 (플레이어 · AI 양쪽) |
| `Status.Control.Root` | `UACGridMovementComponent` 이동 차단 |
| `Status.Control.Silence` | `UACSkillComponent` 시전 차단 |
| `Status.Control.Blind` | 대상·목표 타일을 무작위로 치환 |
| `Status.Deny.Heal` | `UACStatComponent::Heal` 차단 |
| `Status.Guard.Ward` | `TakeDamage` 1회 무효 후 스택 감소 |

**대상** — `Private/Components/ACStatComponent.cpp`, `Private/Components/ACSkillComponent.cpp`, `Private/Components/ACGridMovementComponent.cpp`, `Private/Core/PE_TurnManagerComponent.cpp`, `Private/Characters/PE_EnemyBase.cpp`

---

### 3-6. 스탯 확장

**작업** — `UACStatComponent`에 다음 추가.

| 스탯 | 형식 | 용도 |
|---|---|---|
| 속성별 피해 증폭 | `TMap<FGameplayTag, float>` | 기획 문서의 *"불속성 피해 증폭"*, *"물/얼음 속성 피해 증폭"* |
| 받는 피해 배율 | `float` | `Status.Mod.Vulnerable` 등이 작용하는 지점 |

**대상** — `Public/Components/ACStatComponent.h`, `Private/Components/ACStatComponent.cpp`

**비고** — 범용 공격력 스탯(힘·민첩류)은 추가하지 않는다. 강화는 속성별 증폭으로만 처리한다.

---

### 3-7. 상태이상 UI 표시

**작업** — 캐릭터 머리 위 아이콘 + 스택/턴 수 표시. `UPE_StatusComponent` 델리게이트에 연결.

**대상** — UI 블루프린트, `Public/Components/PE_StatusComponent.h`

---

# 단계 4 — 타일 속성 시스템

### 4-1. `AACTile` 속성 상태 추가

**작업** — 타일이 보유하는 속성 상태와 리플리케이션 추가. `bIsObstacle` / `OnRep_IsObstacle` 패턴을 따른다.

```
FPETileElementState
├─ FGameplayTag  ElementTag      // Element.* 태그
├─ int32         Stacks          // 중첩 (필살기 빌드업 판정용)
└─ int32         RemainingTurns  // 남은 턴
```

**대상** — `Public/Grid/ACTile.h`, `Private/Grid/ACTile.cpp`

**완료 기준** — 서버에서 부여한 타일 속성이 클라이언트에 복제된다.

---

### 4-2. `FPETileElementRules` 규칙 구조체

**작업** — 속성 반응 규칙을 상태 없는 정적 구조체로 구현. 서버 판정과 클라 시각화가 같은 함수를 통과하는 구조로 만든다. `FPETargetRules` · `FPEPushResolver` 선례를 따른다.

**구현 범위** — 기획 문서의 지형속성 규칙.

| 조합 | 결과 |
|---|---|
| 화염 + 물 | 증발 타일로 변환 (이후 화염/물에 덮이거나 1턴 후 소멸) |
| 물 + 번개 | 연결된 물 타일 영역 전체 감전 |
| 얼음 + 그 외 | 얼음 해제 + 강력한 피해 (빙결 부여) |
| 얼음 + 불/번개 | 얼음 해제 + 강력한 피해 + 증발 타일로 변환 |

**함수 구성**
- `Resolve(현재 상태, 부여 속성)` → 결과 상태 + 발생 효과
- `CollectConnected(Grid, StartPos, ElementTag)` → 연쇄 감전용 연결 영역 탐색

**대상** — `Public/Grid/PE_TileElementRules.h`, `Private/Grid/PE_TileElementRules.cpp`

**완료 기준** — 같은 함수가 서버 적용과 클라 예측 양쪽에서 호출되어 결과가 일치한다.

---

### 4-3. 타일 속성 부여 모듈

**작업** — `UPE_SkillEffect_ApplyTileElement` 추가. 단계 2-3에서 전달받는 `FPESkillPhasePlan::AffectedPositions`를 사용한다.

**대상** — `Public/CardSystem/PE_SkillEffectModule.h`

**완료 기준** — 데이터 에셋만으로 지형 변화 카드를 만들 수 있다.

---

### 4-4. 타일 속성 지속시간 처리

**작업** — `EnvironmentTurn`에 타일 속성 턴 감소 및 만료 처리 추가. 단계 3-3에서 만든 처리 지점을 사용한다.

**대상** — `Private/Core/PE_TurnManagerComponent.cpp`, `Private/Grid/ACGridSystem.cpp`

---

### 4-5. 타일 속성 시각화

**작업**
1. 타일 머티리얼에 속성 표현 파라미터 추가
2. 기존 하이라이트 색상과 속성 색상의 표현 분리 (하이라이트는 Emissive, 속성은 베이스 컬러/텍스처 등)
3. 속성별 나이아가라 이펙트 연결

**배경** — 현재 `UpdateVisuals`는 하이라이트 요청 색상을 더한 뒤 클램프하는 단일 경로다. 속성 색이 하이라이트와 같은 채널을 쓰면 구분이 불가능하다.

**대상** — `Private/Grid/ACTile.cpp`, 타일 머티리얼

**완료 기준** — 속성 타일 위에 이동 범위·스킬 범위를 칠해도 양쪽이 모두 식별된다.

---

### 4-6. 디버그 맵 툴 연결

**작업** — `PE_DebugMapToolWidget`의 TODO 두 곳을 실제 속성 부여 호출로 교체.

**대상** — `Private/UI/PE_DebugMapToolWidget.cpp:111`, `:114`

---

# 단계 5 — 시각화 정리

### 5-1. 페이즈별 스플라인 렌더

**작업** — 궤적 렌더를 `PhaseTrajectories[0]` 단일 사용에서 전체 페이즈 순회로 변경. 스플라인 컴포넌트를 페이즈 수만큼 동적으로 생성한다.

**배경** — `PhaseTrajectories`는 이미 모든 페이즈에 대해 계산되어 있고, 렌더만 첫 번째 페이즈를 사용한다. 다중 투사체 스킬의 궤적이 하나만 표시된다.

**대상** — `Private/Components/ACTargetingVisualizerComponent.cpp`, `Public/Components/ACTargetingVisualizerComponent.h`

**완료 기준** — 3연타 스킬 조준 시 궤적 3개와 착탄 구체 3개가 표시된다.

---

### 5-2. 타일 겹침 횟수 표시

**작업**
1. `AACTile::HighlightRequests`를 `TMap<AActor*, ETileHighlightType>`에서 겹침을 셀 수 있는 구조로 교체 (요청자 + 타입별 카운트)
2. 겹침 횟수에 따라 머티리얼 Emissive 강도 조절
3. 타일 중하단에 겹침 횟수를 숫자 UI로 표시 (항상 카메라를 향함)

**배경** — 현재 구조는 요청자당 값 하나만 보관하므로, 같은 요청자의 두 번째 요청이 첫 번째를 덮어쓴다. 다중 페이즈 스킬이 한 타일에 여러 번 겹쳐도 횟수를 알 수 없다.

**대상** — `Public/Grid/ACTile.h:124`, `Private/Grid/ACTile.cpp`, `Private/Grid/ACGridSystem.cpp`

**완료 기준** — 같은 타일에 2회 이상 겹치는 스킬 조준 시 숫자가 표시되고 밝기가 달라진다.

**비고** — 단계 4-5와 같은 파일을 건드리므로 연속 작업을 권장.

---

# 단계 6 — 카드 수치 서버 권위화 + 동적 수치

### 6-1. 덱 소유권 서버 이전

**작업**
1. `UPE_CardInstance` 소유를 서버(PlayerState 또는 서버측 덱 매니저)로 이전
2. 인스턴스별 핸들(`CardInstanceID`) 부여
3. 클라이언트 `UACDeckManagerComponent`는 표시·연출 전담으로 축소
4. RPC를 `UPE_SkillData*` 전달에서 `CardInstanceID` 전달로 변경

**배경** — `InitializeDeck`이 `APE_PlayerController::BeginPlay`에서 권한 가드 없이 호출되어 서버와 클라이언트가 각각 덱을 생성하고 각각 셔플한다. 서버 쪽 `RunRandomStream`은 `APE_BattleGameMode::InitializeShuffledSpawns`의 PlayerStart 셔플로 먼저 전진한 상태이므로, 양쪽 `DrawPile` 순서가 다르다. 현재는 `DrawCards`가 로컬 입력에서만 호출되어 서버 `DrawPile`이 소비되지 않기 때문에 증상이 없다.

**대상** — `Private/Core/PE_PlayerController.cpp:60`, `Private/Components/ACDeckManagerComponent.cpp`, `Private/Components/PE_CardCastComponent.cpp`, `Public/Core/PE_PlayerState.h`

**완료 기준** — 클라이언트 접속 테스트에서 서버와 클라이언트의 덱 순서 · 손패 구성이 일치한다.

---

### 6-2. `FPESkillContext` 도입

**작업** — 시전 시점의 상태를 담는 구조체를 만들고, 수치 산출과 이펙트 모듈이 이를 인자로 받도록 변경.

```
FPESkillContext
├─ 시전자 스탯 / 상태이상
├─ 시전자 격자 위치
├─ 손패 장수 / 덱 구성 / 버린 패 구성
├─ 이번 턴 사용한 카드 수
├─ 현재 턴 번호
├─ 영향 타일 집합 (단계 2-1의 FPESkillPhasePlan)
├─ 대상 타일의 속성 상태 (단계 4)
└─ 대상 목록
```

**변경 대상 시그니처**
- `UPE_CardInstance::GetCalculatedDamage()` → `GetCalculatedDamage(const FPESkillContext&)`
- `GetCalculatedAPCost()`, `GetCalculatedHeal()` 동일
- 사거리 산출 함수 신규 추가 (`GetCalculatedRange`)
- `UPE_SkillEffectModule::ApplyEffects`에 컨텍스트 전달

**배경** — 현재 `GetCalculated*()`는 인자를 받지 않으며 `CostModifier` · `DamageModifier` · `HealModifier` 세 개의 플랫 필드만 반영한다. 평가 시점마다 달라지는 조건부 수치를 표현할 수 없다. 사거리는 변동 경로 자체가 없다.

**대상** — `Public/CardSystem/PE_CardInstance.h`, `Private/CardSystem/PE_CardInstance.cpp`, `Public/CardSystem/PE_SkillEffectModule.h`, `Public/CardSystem/PE_SkillContext.h` (신규)

**완료 기준** — "손패 장수만큼 데미지 증가" 카드가 데이터 설정만으로 동작한다.

**비고** — 6-1과 같은 경로를 수정하므로 반드시 함께 진행한다. 분리하면 동일 경로를 두 번 수정하게 된다.

---

### 6-3. 모디파이어 체인

**작업** — `UPE_CardInstance`의 플랫 수정치 3개를 모디파이어 배열로 교체.

```
TArray<UPE_CardModifier*> Modifiers
└─ virtual void Evaluate(const FPESkillContext&, FPESkillNumbers& InOut)
```

마석 · 유물 · 임시 버프 · 조건부 스케일링을 모두 같은 인터페이스로 처리한다. 기존 플랫 필드는 무조건 적용 모디파이어로 흡수한다.

**대상** — `Public/CardSystem/PE_CardInstance.h`, `Public/CardSystem/PE_CardModifier.h` (신규)

**완료 기준** — 모디파이어 2개 이상을 붙인 카드의 최종 수치가 UI 텍스트와 실제 전투 결과에서 일치한다.

---

### 6-4. 전투 경로를 산출값으로 전환

**작업** — 실제 전투 경로에 남아 있는 원본 데이터 직접 참조를 산출값 호출로 교체.

| 위치 | 현재 | 변경 |
|---|---|---|
| `PE_CardCastComponent.cpp:52` | `SkillData->BaseAPCost` | `GetCalculatedAPCost(Context)` |
| `PE_CardCastComponent.cpp:169` | `SkillData->BaseRange` | `GetCalculatedRange(Context)` |
| `PE_CardCastComponent.cpp:337` | `SkillData->BaseDamage` | `GetCalculatedDamage(Context)` |
| `ACSkillComponent.cpp:79` | `SkillData->BaseRange` | 산출값 |
| `ACSkillComponent.cpp:97` | `SkillData->BaseAPCost` | 산출값 |
| `ACSkillComponent.cpp:186, 198` | 재검증 · AP 환불 | 산출값 |

**비고** — AI(`PE_EnemyBase`)와 `TryExecuteSkill`은 마석 연산이 없으므로 원본 데이터를 그대로 사용한다.

**완료 기준** — 카드 UI에 표시된 수치와 실제 전투 결과가 항상 일치한다.

---

### 6-5. 연사 횟수 런타임 결정

**작업** — 페이즈 반복 횟수를 컨텍스트에서 산출하도록 변경. `FPESkillHitPhase`에 `RepeatCount` 기본값을 두고, 모디파이어가 이를 변경할 수 있게 한다. 시각화는 반복 횟수만큼 궤적을 그린다(단계 5-1의 순회를 재사용).

**배경** — `HitPhases`는 `EditAnywhere` 고정 배열이므로 타격 횟수가 데이터에 고정된다.

**대상** — `Public/CardSystem/PE_SkillData.h`, `Private/Components/ACSkillComponent.cpp`, `Private/Components/ACTargetingVisualizerComponent.cpp`

**완료 기준** — "손패 장수만큼 발사" 카드의 실제 발사 수와 조준 시 표시되는 궤적 수가 일치한다.

---

# 단계 7 — 카드 표현 축 확장

### 7-1. 적 의도를 다음 턴 예고로 전환

**작업** — `NetMulticast_ShowSkillIntent` / `ShowMoveIntent` 호출 시점을 적 자기 턴 직전에서 **플레이어 턴 동안 표시**되도록 변경.

**구현 범위**
1. 적 턴 종료 시 다음 턴 행동을 미리 결정하여 보관
2. 플레이어 턴 진입 시 예고 표시
3. 예고 대상이 이동·밀치기로 변경되면 예고 갱신

**배경** — 렌더링(`HighlightArea`, `HighlightAoE`, `HighlightTarget`, `ClearAllHighlightsFor`)과 전용 색상(`EnemyInRangeColor`, `EnemySkillTargetColor`, `EnemyPathColor`)은 이미 구현되어 있다. 현재는 `ActionDelay` 시간 동안 자기 턴에만 표시되어 플레이어가 대응할 수 없다.

**대상** — `Private/Characters/PE_EnemyBase.cpp:139`, `:166`, `Private/Core/PE_TurnManagerComponent.cpp`

**완료 기준** — 플레이어 턴 중 적의 다음 공격 범위가 표시되고, 적을 밀치면 예고가 갱신된다.

---

### 7-2. 엄폐 규칙을 적 AI에 적용

**작업** — `FPESkillTrajectory::CanBeBlocked` 판정을 적 AI의 스킬 사용 판단에 반영. 앞이 막힌 경우 시전하지 않거나 위치를 변경하도록 한다.

**배경** — 현재 적 AI는 `FPETargetRules::Evaluate`로 사거리만 확인하고 궤적 차단을 검사하지 않는다. 플레이어 쪽은 `Blocked` 하이라이트까지 구현되어 있다.

**대상** — `Private/Characters/PE_EnemyBase.cpp`

**완료 기준** — 장애물 뒤에 서면 적의 직사 스킬이 시전되지 않는다.

---

### 7-3. 위치 기반 조건 모디파이어

**작업** — 단계 6-3의 모디파이어로 다음 조건을 구현.

| 조건 | 판정 근거 |
|---|---|
| 내가 서 있는 타일의 속성과 카드 속성이 일치 | 컨텍스트의 타일 속성 |
| 대상과의 거리 | 컨텍스트의 시전자 위치 |
| 직선상 대상 수 | 컨텍스트의 대상 목록 |
| 이번 턴 이동 여부 | 컨텍스트에 이동 플래그 추가 |

**대상** — `Public/CardSystem/PE_CardModifier.h`

---

### 7-4. 설치 · 지연 발동

**작업** — 타일에 배치되어 N턴 후 발동하는 효과 구현.

**구현 범위**
1. 타일 또는 GridSystem에 예약 효과 목록 보관
2. `EnvironmentTurn`에서 카운트 감소 및 발동
3. 발동 시 기존 이펙트 모듈 재사용
4. 예약 위치 시각화

**대상** — `Private/Grid/ACGridSystem.cpp`, `Private/Core/PE_TurnManagerComponent.cpp`, `Public/CardSystem/PE_SkillEffectModule.h`

---

### 7-5. 카드 트리거 확장

**작업** — `EPECardTriggerType` 열거형을 `FGameplayTag` + 이벤트 버스로 교체.

**배경** — 현재 값은 `None`, `OnTurnEnd` 두 개이며 `OnDrawn`, `OnDamaged`는 주석 처리되어 있다. 카드당 트리거 1개만 지정 가능하다.

**대상** — `Public/CardSystem/PE_DataTypes.h`, `Public/CardSystem/PE_CardData.h`, `Private/Components/PE_CardCastComponent.cpp`

**완료 기준** — 트리거 2개 이상을 가진 카드가 동작한다.

---

### 7-6. 휘발성 · 유지

**작업**
- 휘발성(Exhaust): 사용 후 덱·버린 패로 돌아가지 않고 제거
- 유지(Retain): 턴 종료 시 버려지지 않음

**배경** — 기획 문서의 시작 특성 *"수식가: 매 턴 휘발성 '수식' 카드를 한 장 얻습니다"* 가 휘발성을 요구한다. 현재 구현 없음.

**대상** — `Public/CardSystem/PE_CardData.h`, `Private/Components/ACDeckManagerComponent.cpp`

---

### 7-7. 덱 조작 효과

**작업** — 다음 이펙트 모듈 추가.

| 모듈 | 기능 |
|---|---|
| `UPE_SkillEffect_Draw` | 카드 N장 뽑기 |
| `UPE_SkillEffect_Discard` | 손패 버리기 (선택 / 무작위) |
| `UPE_SkillEffect_AddCard` | 손패 또는 덱에 카드 생성 |

**배경** — 기획 문서의 필살기 게이지형 3종(*해일* 등)이 *"카드가 손패로 들어오고"* 를 요구한다.

**대상** — `Public/CardSystem/PE_SkillEffectModule.h`, `Private/Components/ACDeckManagerComponent.cpp`

**선행** — 단계 6-1 (덱 소유권 확정 후 진행)

---

# 단계 8 — 유물 · 마석

### 8-1. 마석 소켓

**작업** — `UPE_CardInstance`에 마석 슬롯 추가. 마석을 모디파이어(단계 6-3)로 구현한다.

**구현 범위** — 기획 문서의 마석 효과 축: 밀치기 키워드 부여, 다중 속성, 지형 속성 유발, 다중 시전, 범위 확산 + AP/체력 패널티.

**대상** — `Public/CardSystem/PE_CardInstance.h` (`// TODO: 마석(Modifier) 부착/해제` 주석 위치)

---

### 8-2. 유물

**작업** — 런 단위 지속 효과. 모디파이어와 이벤트 버스(단계 7-5)에 연결.

---

### 8-3. 세션 · 저장

**작업** — 런 진행도, 덱 구성, 마석 부착 상태 직렬화.

**비고** — `UPE_CardInstance`의 수정치 필드 주석에 *"디스크에 세이브/로드 될 데이터들"* 로 의도가 명시되어 있다. 저장 형식은 단계 6-3 확정 후 결정한다.

---

# 단계 9 — 대기 · 잔여

### 9-1. 투사체 시작 위치 (소켓 + 오프셋)

**작업**
- 페이즈별 `MuzzleOffset` 추가 — **즉시 가능**
- 소켓 이름 지정 — **캐릭터 3D 에셋 · 애니메이션 선행**

**배경** — 현재 `GetMuzzleLocationForDirection`은 캡슐 높이 비율(`MuzzleHeightRatio`)과 전방 오프셋(`MuzzleForwardOffset`) 상수를 사용하며 모든 페이즈가 같은 값을 쓴다.

**대상** — `Private/CardSystem/PE_SkillTrajectory.cpp:100`, `Public/CardSystem/PE_SkillData.h`

---

### 9-2. 투사체 좌우 파라미터

**작업** — 포물선 최고점에서의 좌우 이동량 파라미터 추가. 시각화는 단계 5-1의 페이즈 순회에 반영.

**배경** — 현재 `ProjectileGravity`만 지정 가능하다.

**대상** — `Public/CardSystem/PE_SkillData.h`, `Private/CardSystem/PE_SkillTrajectory.cpp` (`Sweep`)

---

### 9-3. `PE_PlayerCharacter::Tick` 토글

**작업** — `PrimaryActorTick`을 상시 활성에서 타겟팅 중에만 활성으로 변경.

**대상** — `Private/Characters/PE_PlayerCharacter.cpp:17`

---

### 9-4. 하드코딩 상수 집약

**작업** — 산재된 상수를 `PE_Constants`로 모으거나 데이터화.

| 상수 | 위치 |
|---|---|
| 스윕 분할 20구간, 스피어 반경 5.f | `PE_SkillTrajectory.cpp` |
| `MuzzleHeightRatio`, `MuzzleForwardOffset` | `PE_SkillTrajectory.cpp` |
| `ActionInterval 0.2f` | `PE_GameState.cpp` |
| `SetLifeSpan 0.5f` | `PE_CharacterBase.cpp` |
| 캡슐 높이 배율 0.7f / 0.8f | `ACGridMovementComponent.cpp` |

---

### 9-5. HP ↔ AP 교환

**작업** — 체력을 지불해 AP를 얻는 경로 및 그 역방향 추가.

**배경** — 기획 문서에 마석 패널티로 *"AP 소모 증가 or 체력 소모"* 가 있고, 시작 특성 *"회광반조"* 가 저체력 빌드를 전제한다.

**대상** — `Private/Components/ACStatComponent.cpp`, `Public/CardSystem/PE_SkillEffectModule.h`

---

### 9-6. 상위 지형 속성 (필살기)

**작업** — 기획 문서의 필살기 7종과 빌드업 3방식.

| 빌드업 | 판정 |
|---|---|
| 공간 연결형 | 동일/유사 속성 타일의 연결 또는 특정 형태 판정 |
| 단일 타일 압축 | 한 칸에 동일 속성 N스택 |
| 원소 게이지형 | 특정 속성 타일 N회 생성 시 손패에 필살기 카드 생성 |

**선행** — 단계 4 완료. 태그(`Element.Special.Hellfire` 등 5종)는 이미 등록되어 있다.

**비고** — 세 가지 빌드업 판정이 각각 별개 시스템이므로 단계 4가 안정화된 후 별도 단계로 진행한다.

---

# 부록 A — 제외 항목

| 항목 | 판단 |
|---|---|
| 방어도(Block) 시스템 | 추가하지 않음. 방어는 엄폐(7-2) · 행동 제약(3-5) · 1회 무효(`Status.Guard.Ward`) · 성역형 지역 효과로 처리 |
| 범용 공격력 스탯 (힘 · 민첩류) | 추가하지 않음. 강화는 속성별 피해 증폭(3-6)으로 처리 |
| GAS (Gameplay Ability System) | 도입하지 않음. 기존 `UPE_SkillEffectModule` 체계 유지 |

---

# 부록 B — 완료 항목

| 항목 | 커밋 |
|---|---|
| 액션 큐 토큰화 + 워치독 (P0-1) | — |
| 턴 종료 진입점 일원화 (P0-3) | — |
| GridSystem 점유 레지스트리 (P2-3) | — |
| 스윕 궤적 공용화 (P2-1) | — |
| 밀치기 시스템화 (P2-2) | `95fe107` |
| 타게팅 규칙 통합 (P2-4) | `f5bfe52`, `00f1f52` |
| PlayerController 카드 시전 분리 (P2-4) | `8e4bf79` |
| 덱 셔플 Fisher-Yates 교체 | `89ca440` |
| 관통 투사체 | — |
| 직선/관통 사거리 끝단 조준 | — |
| Custom AoE 회전 (타일 집합 + 폭발 비주얼 스냅 일치) | — |
| 다중 페이즈 타격 + 페이즈별 액션 큐 토큰 | — |
| 드로우 애니메이션 | `701359c` |

**잔여 `GetAllActorsOfClass` 3곳** — 모두 의도된 사용.

| 위치 | 용도 |
|---|---|
| `PE_BattleGameMode.cpp:42` | PlayerStart 수집 (1회) |
| `PE_TurnManagerComponent.cpp:154` | 적 큐 구성 (턴당 1회) |
| `ACGridSystem.cpp:466` | 점유 검증기 (`#if !UE_BUILD_SHIPPING`) |
