# Project Entropy

![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.7-0e1128?logo=unrealengine&logoColor=white)
![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey)
![Multiplayer](https://img.shields.io/badge/Multiplayer-Steam%20Online%20Subsystem-1b2838?logo=steam&logoColor=white)
![Development](https://img.shields.io/badge/Development-Solo%20(1인%20개발)-orange)
![License](https://img.shields.io/badge/License-All%20Rights%20Reserved-red)

> 3D 타일 기반 로그라이크 턴제 카드 게임. 카드/스킬/액션 큐 시스템을 설계하고, 반복적인 리팩토링을 통해 데이터 기반 콘텐츠 확장이 가능한 구조로 개발 중인 1인 개발 프로젝트입니다.

> (26.09.17) - 아직 개발 단계에 있으므로 이하 내용과 현재 프로젝트 간에 차이가 있을 수 있습니다.

---

## ■ 핵심 기술 포인트

- **모듈형 스킬 이펙트 시스템** — `UPE_SkillEffectModule` 기반으로 데미지/넉백 등 효과를 "장착형 모듈"로 분리. 로직별 전용 클래스를 늘리지 않고, 데이터 에셋(`UPE_SkillData`)에서 모듈을 조합하는 것만으로 신규 스킬 제작 가능.
- **토큰 기반 액션 큐** — `APE_GameState`의 `TQueue<FPESkillActionPayload>`와 액션 토큰(`BeginAction`/`EndAction`)으로 스킬 결제→애니메이션→판정의 비동기 순서를 서버가 강제. 하나의 스킬이 만드는 모든 파생 액션(투사체, 폭발, 연쇄 넉백)이 토큰을 반납해야 다음 카드가 실행되며, 워치독과 파괴 시점 정산으로 보고 유실에 의한 큐 정지를 차단.
- **예측-판정 일치형 충돌 시스템** — 클라이언트 예측 시각화(스플라인)와 서버 실제 판정이 동일한 `SweepSingleByChannel` 스윕 알고리즘을 공유해, 직선/곡사/관통 스킬 모두 "보이는 대로 맞는" 결과를 보장.
- **PVP 멀티플레이 동기화** — Advanced Sessions + Steam Online Subsystem 기반 로비/매치메이킹, 시드 기반 랜덤 동기화, 팀 ID 기반 피아식별 및 턴 종료 만장일치 시스템.
- **상태 머신 기반 카드 상호작용** — 명시적 상태(`EPEInteractionState`)로 드래그/캐스팅/취소를 관리해 비동기 콜백 타이밍 버그를 구조적으로 차단하고, 나이아가라 VFX·커스텀 머티리얼로 완성한 손맛 있는 카드 연출(Juicy UX).

---

## ■ 스크린샷 / 데모

<!-- TODO: 실제 스크린샷/GIF로 교체 -->
| 카드 시스템 | 스킬 & VFX | PVP 대전 |
|---|---|---|
| ![카드 시스템 데모](_Docs/Images/PE-card-system.gif) | ![스킬 이펙트 데모](_Docs/Images/PE-skill-vfx.gif) | ![PVP 대전 데모](_Docs/Images/PE-pvp-battle.gif) |

<!-- TODO: 데모 버전 개발이 끝나면 플레이 영상 링크/썸네일 추가 -->
> ▶ **플레이 영상:** (링크 예정)

---

## 프로젝트 개요
- **장르:** 3D 타일 기반 로그라이크 턴제 카드 게임
- **주요 메커니즘:** 이동 행동 한번에 1AP 사용, 스킬을 통한 지형속성 부여와 속성 시너지, PVP 모드 존재
- **개발 인원:** 1인 개발

## 개발 기간 & 담당 역할
- **개발 기간:** 2026-04-24 ~ 2026-09-17 (최초 커밋 기준)
- **담당 역할 (1인 개발 — 전 영역 단독 수행, AI 활용됨)**
  - **기획:** 게임 시스템 설계, 세계관/시나리오, 지형 속성·마석 시스템 등 밸런스 기획 문서화
  - **프로그래밍:** 그리드/턴/카드/스킬/액션 큐 시스템, 멀티플레이(세션·동기화·PVP) 로직 C++ 구현
  - **데이터 설계:** `UPrimaryDataAsset` 기반 카드·스킬 데이터 파이프라인 설계 (`UPE_CardData`, `UPE_SkillData`, `FPESkillHitPhase`) 및 모듈형 이펙트 데이터 구조 설계
  - **VFX/머티리얼:** 나이아가라 이펙트, 카드 등급별 머티리얼(Rainbow/체크무늬 등) 제작

## 기술 스택
- Unreal Engine 5.7
- C++
- Advanced Sessions
- Steam Socket / Steam Online Subsystem

## 현재 진행 상황 및 한계점

### 진행 상황
- 그리드 시스템, 카드 시스템, 스킬 시스템 대부분 구현 완료
- 데이터 기반 작업도 거의 끝
- 멀티플레이 동기화는 개발과 함께 테스트 및 진행 중
- Juicy 연출, 깔끔한 카드 조작감

### 남은 개발 사항
- 타일 속성 시스템 구현 필요
- 캐릭터 모델 및 애니메이션 필요, 동시에 스킬 사용 연출과의 동기화 필요
- 노드 기반 맵 이동, 거점 업그레이드 등 데모를 위한 게임 플로우 제작 필요
- 유물 및 룬 개발 필요 (캐릭터 및 카드 업그레이드 시스템)
- 상점 및 각종 이벤트 제작 필요

### 한계점
- 아티스트의 부재
- 모델, 애니메이션은 어떻게든 한다고 쳐도 다양한 스킬만큼의 VFX를 어떻게 구하느냐, 만든다면 어떻게 만드느냐
- 속성별로 VFX 제작 모듈을 만들어야 하는가? 아티스트를 구하진 않을 거다, 시간을 갈아 넣으면 되는걸까?

## 트러블슈팅 내역

### 1. 카드 시스템 — 상태 기반 카드 상호작용

**문제** -
연속으로 카드를 시전하면 방금 새로 손패에 들어온(또는 다음으로 잡은) 카드가 사용자가 조작하지도 않았는데 자동으로 소멸(버려짐)하는 버그가 있었다.

**원인** -
당시 `UACCardInteractionComponent::OnCastingReadyFinished()` / `OnInstantCastFinished()`는 인자 없이 호출되어 "지금 콜백을 보낸 카드"와 "지금 `CastingCard`로 잡혀있는 카드"가 같은지 검증하지 않았다. 블루프린트 애니메이션 콜백은 비동기로 늦게 도착할 수 있어서, 이전 시전의 콜백이 늦게 들어오면 그 시점에 `CastingCard`로 잡혀 있던 **다음** 카드를 대상으로 산화 로직이 실행됐다.

**해결** -
콜백에 발신 카드를 인자로 넘기고, 현재 상태의 카드와 일치하는지 검사하도록 수정했다.

```cpp
void UACCardInteractionComponent::OnInstantCastFinished(APE_CardActor* CallerCard)
{
    if (CastingCard && CastingCard == CallerCard)  // 발신자 검증 추가
    {
        CastingCard->SetActorEnableCollision(false); // 재클릭 원천 차단
        CastingCard->PlayDiscardAnimation();
        DeckManager->DiscardCard(CastingCard);
    }
}
```

이 사건을 계기로 상호작용 로직 전체를 **명시적 상태 머신**으로 재설계했다. 현재 `Source/Project_Entropy/Public/Components/ACCardInteractionComponent.h`의 `EPEInteractionState`(Hovering / Selecting / Disabled)가 그 결과이며, 각 진입점이 현재 상태를 검사해 어긋난 시점의 호출을 원천 차단한다.

```cpp
void UACCardInteractionComponent::GrabCard()
{
    if (CurrentState != EPEInteractionState::Hovering) return; // 상태 불일치 시 무시
    ...
}
```

**결과** -
"발신자가 지금 상태의 대상과 같은가"를 매번 검증하는 패턴이 구조적으로 강제되면서, 비동기 콜백/애니메이션 타이밍이 어긋나 엉뚱한 카드가 처리되는 종류의 버그가 재발하지 않는 환경이 됐다.

---

### 2. 데이터 기반 스킬/카드 시스템 — 모듈형 이펙트 구조

**문제** -
스킬 하나하나를 데미지, 넉백, 범위 스폰 등 로직별 전용 클래스(`PE_SkillLogicBase`, `PE_SkillLogic_AoE`, `PE_SkillLogic_Push`, `PE_SkillLogic_SpawnActor`)로 만들다 보니, "AoE + 넉백"처럼 두 효과를 조합한 스킬을 만들려면 클래스를 새로 파거나 로직을 상속/복붙해야 해서 조합이 늘어날수록 유지보수가 어려워졌다.

**원인** -
스킬의 "형태"(범위/투사체)와 "효과"(데미지/넉백/상태이상)가 하나의 클래스 안에 결합되어 있어, 새 조합마다 클래스 수가 곱으로 늘어나는 구조였다.

**해결** -
공통 인터페이스만 정의하고 실제 효과는 서브클래스가 구현하도록 바꿨다.

```cpp
UCLASS(Abstract, DefaultToInstanced, EditInlineNew, Blueprintable)
class UPE_SkillEffectModule : public UObject
{
    // TargetLocation(월드)과 TargetGridPos(논리 격자)는 같은 지점을 가리킨다
    virtual void ApplyEffects(AActor* Instigator, const TSet<APE_CharacterBase*>& Targets,
        const FVector& TargetLocation, FIntPoint TargetGridPos,
        const UPE_SkillData* InSkillData, float CalculatedDamage)
        PURE_VIRTUAL(UPE_SkillEffectModule::ApplyEffects, );
};
```

월드 좌표와 함께 논리 격자 좌표를 넘기는 이유는, 밀치기처럼 칸 단위로 계산하는 모듈이 월드 좌표에서 칸을 역산하지 않고 조준 미리보기와 실제 적용이 **같은 격자 좌표를 입력으로** 받게 하기 위해서다.

각 타격 페이즈(`FPESkillHitPhase`)가 이 모듈을 **배열로 장착**하도록 데이터 구조를 바꿨다.

```cpp
UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Phase|Effects")
TArray<TObjectPtr<UPE_SkillEffectModule>> EffectModules;
```

실행부(`ACSkillComponent::CommitQueuedSkill`)는 어떤 조합이든 동일하게 순회만 하면 된다.

```cpp
for (UPE_SkillEffectModule* Module : ExecPhase.EffectModules)
    if (Module) Module->ApplyEffects(Caster, AffectedTargets, PhaseTargetLoc, TargetPos, SkillData, FinalDamage);
```

**결과** - 
데미지(`UPE_SkillEffect_Damage`), 넉백(`UPE_SkillEffect_Push`)처럼 효과 하나당 모듈 클래스 하나만 만들면 되고, 새 스킬은 코드 수정 없이 `UPE_SkillData` 에셋에서 페이즈별로 원하는 모듈을 조합해 넣는 것만으로 완성된다. 실제로 이후 레이저·파도·화염구 등 신규 스킬들이 새 클래스 추가 없이 기존 Damage/Push 모듈 조합 + `HitPhases` 데이터 설정만으로 제작됐다. 이후 넉백 모듈은 밀치기 규칙과 실행을 전담 시스템에 넘기고 요청만 만드는 어댑터로 줄었다 (5번 항목 참고).

---

### 3. 액션 큐 시스템

**문제** -
(1) 카드를 연속으로 빠르게 시전하면 카드가 실행되지 못하고 소멸하는 버그, (2) 멀티플레이 환경에서 한 캐릭터의 "결제 → 스킬 애니메이션 → 실제 판정" 진행 도중 다른 스킬이 끼어들어 순서·피격 결과가 꼬이는 문제가 있었다.

**원인** -
스킬 실행이 결제(즉시) → 클라이언트 스킬 애니메이션(비동기 RPC) → 실제 타격(타이머/멀티캐스트)의 여러 비동기 단계로 쪼개져 있는데, 이 단계들의 순서를 보장하는 장치가 없었다. 특히 스킬 하나가 여러 히트 페이즈나 파생 밀치기까지 만들어낼 수 있어서, 그중 하나가 끝나기도 전에 다음 스킬의 결제/실행이 시작될 수 있었다.

**해결** -
`APE_GameState`에 서버 전용 큐(`TQueue<FPESkillActionPayload>`)를 두고, 진행 중인 파생 액션이 전부 끝나야 다음 큐 항목으로 넘어가게 만들었다. 처음에는 정수 카운터(`ReportActionStarted()` +1 / `ReportActionEnded()` −1)로 구현했지만, 이 방식에서 **게임이 완전히 멈추는 데드락**이 발견되어 토큰 방식으로 다시 설계했다.

카운터의 +1과 −1이 **서로 다른 객체의 생존에 의존**한 것이 원인이었다. 밀치기는 시작할 때 +1을 하고, −1은 밀려나는 캐릭터의 이동 컴포넌트가 도착하는 순간에만 했다. 그런데 연쇄 밀치기 도중 앞 캐릭터의 충돌 피해로 사망한 대상은 0.5초 뒤 파괴되므로, 자기 도착 시점 전에 사라지면 −1이 영영 호출되지 않는다. 카운터가 0으로 내려가지 않으니 다음 액션이 시작되지 않고, 턴 종료까지 막혀 게임이 정지했다.

그래서 익명의 카운터를 **발급과 반납을 추적할 수 있는 토큰**으로 바꿨다.

```cpp
// APE_GameState — 토큰마다 발급 시각과 발생 지점을 기록
TMap<int32, FPEPendingAction> PendingActions;   // { ActionLogID, StartTime, Context }

int32 BeginAction(const FString& Context, int32 ActionLogID);

void APE_GameState::EndAction(int32 TokenID, int32 ActionLogID)
{
    if (PendingActions.Remove(TokenID) == 0)
    {
        // 중복 반납은 카운터를 음수로 망가뜨리는 대신 흔적만 남기고 흘려보낸다
        UE_LOG(LogTemp, Warning, TEXT("[ActionQueue] 유효하지 않은 토큰 반납 시도: Token=%d"), TokenID);
    }
    RemoveActionLog(ActionLogID);

    if (PendingActions.Num() == 0)  // 파생된 모든 액션이 반납됐을 때만 다음 카드로
    {
        GetWorld()->GetTimerManager().SetTimer(ActionDelayTimerHandle, this,
            &APE_GameState::ProcessNextAction, ActionInterval, false);
    }
}
```

반납이 유실될 수 있는 지점에는 세 겹의 안전망을 걸었다.

| 위치 | 상황 | 대응 |
|---|---|---|
| `UACGridMovementComponent::EndPlay` | 넉백 도중 사망해 액터가 파괴됨 | 보고하지 못한 넉백의 도착을 대신 보고 → 연쇄 토큰이 정상 반납됨 |
| `APE_SkillActionActor::EndPlay` | 투사체가 타격 판정 전에 파괴됨 | 중복 방지 플래그로 정확히 1회 정산 |
| 워치독 (1초 주기) | 그 밖의 모든 유실 | 5초(`ActionTimeout`) 넘게 반납이 없으면 강제 해제 + `Context`를 담은 Error 로그 |

API 이름도 `ReportActionStarted/Ended`에서 `BeginAction/EndAction`으로 바꿔, 옛 호출부가 전부 컴파일 에러로 드러나게 해서 하나도 빠짐없이 옮겼다.

**결과** -
카드 연속 시전 시 소멸 버그가 해결됐고, 멀티 환경에서도 "한 스킬의 전체 연쇄 결과가 끝나야 다음 스킬 처리 시작"이 보장돼 간섭 문제가 사라졌다. 토큰 전환 이후로는 대상이 도중에 파괴돼도 큐가 멈추지 않는다. 정상 동작 중에는 `[ActionQueue]` 경고가 나오지 않도록 설계되어, 경고가 찍히면 그 `Context=` 문자열(`SkillBody:<시전자>/<스킬ID>` 등)이 곧 반납이 새는 지점을 가리킨다. 부수적으로 큐에 쌓인 항목을 `FPEActionLogData` + `OnActionQueueUpdated` 델리게이트로 UI에 방송해, 플레이어가 지금 무엇이 대기 중인지 볼 수 있는 액션 로그 UI까지 파생됐다.

---

### 4. 현실적 스킬 충돌 판정 (직선/곡사/관통)

**문제** -
초기에는 스킬을 "직선"이냐 "곡사"냐로만 대충 분류해 중간 대상을 건너뛸 수 있는지를 판단했다. 관통 스킬, 캐릭터/장애물의 실제 크기, 스킬의 예측 시각화(스플라인)와 실제 판정 결과의 일치 여부까지는 제대로 다루지 못했다.

**원인** -
판정이 그리드 좌표 간 거리 계산 수준의 근사치였고, 실제 3D 캡슐 콜리전(캐릭터 몸통 크기, 장애물)과는 무관하게 동작해서 "화면에 보이는 궤적"과 "실제 맞는 대상"이 어긋날 수 있었다.

**해결** -
서버 실행부(`ACSkillComponent::CommitQueuedSkill`)와 클라이언트 예측 시각화(`ACTargetingVisualizerComponent`) **양쪽 모두** 동일한 알고리즘을 쓰도록 통일했다. 시작점~목표점을 20구간으로 나눠 매 구간 `SweepSingleByChannel`로 구체 콜리전을 쏴서 캐릭터/장애물과 먼저 충돌하는지 검사하고, 맞으면 그 지점에서 멈춰 해당 대상을 타격 대상으로 확정한다.

```cpp
// 서버 실제 판정 / 클라이언트 예측 스플라인이 공유하는 알고리즘
FCollisionShape SweepShape = FCollisionShape::MakeSphere(5.f);
for (int32 step = 1; step <= NumSegments; ++step)
{
    FVector NextPos = FMath::Lerp(StartLoc, TargetLoc, Alpha);
    FHitResult HitResult;
    if (GetWorld()->SweepSingleByChannel(HitResult, LastPos, NextPos, FQuat::Identity,
        ECC_Visibility, SweepShape, Params))
    {
        TargetLoc = HitResult.Location;         // 도중에 가로챈 지점으로 확정
        TargetChar = Cast<APE_CharacterBase>(HitResult.GetActor());
        break;
    }
    LastPos = NextPos;
}
```

**결과** -
직선/곡사/관통(`bDestroyOnHit = false`) 스킬 전부가 하나의 스윕 알고리즘으로 처리 가능해졌고, 클라이언트가 미리 그려주는 예측 궤적(스플라인)과 서버의 실제 타격 결과가 같은 로직으로 계산되므로 "예측과 다르게 맞았다/빗나갔다"는 괴리가 사라졌다.

---

### 5. 연쇄 밀치기 시각적 타이밍 불일치 해결 및 시스템화

**문제** -
`[1, 2, _, _, 3]`처럼 일렬로 선 적 중 1과 2가 밀치기에 동시에 맞으면, 범위 밖의 3이 2에게 **닿기도 전에 혼자 밀려나는** 것처럼 보였다. 또 밀치기 로직 전체가 스킬 효과 모듈(`UPE_SkillEffect_Push`) 안에 있어서, 함정·장판·돌진처럼 스킬이 아닌 주체는 밀치기를 일으킬 방법이 없었다.

**원인** -
연쇄 대상의 출발 시각을 실제 충돌이 아니라 **미리 계산한 예약 시간**으로 정하고 있었다.

```cpp
// 기존: 부딪힌 칸 번호로 "이때쯤 닿겠지"를 미리 계산해 예약
const float TimePerTile = 100.f / TaskMove->GetGridMoveSpeed();
PendingPushes.Add({ CollidedChar, Task.RemainingDist - Step, Task.PushDir,
                    Task.Delay + (Step * TimePerTile) });
```

이 예측은 이동 컴포넌트의 실제 보간과 두 곳에서 어긋났다.
- `Step`은 부딪힌 칸까지 센 값이라 실제 이동한 칸 수(`Step - 1`)보다 한 칸 많다.
- 넉백의 마지막 칸은 오버슈트 이징(`Alpha = 1 + c3(t-1)³ + c1(t-1)²`, c1=3, c3=4)을 타므로, 시각적 접촉은 마지막 칸 이동 시간의 **25% 지점**(`t = 1 - c1/c3`)에서 일어난다. 예측식은 100%를 가정했다.

이동 속도 1000 기준으로 2가 3에 실제로 닿는 시각은 **0.125초**, 3의 예약 출발은 **0.300초**였다. 3은 오히려 **늦게** 출발했는데, 2가 완전히 멈춘(0.2초) 뒤에 혼자 움직이니 충돌과 끊겨 "닿지도 않았는데 밀린다"로 보였다. 하필 0.300초가 1에게 받힌 2의 두 번째 밀림과 겹쳐 "2와 동시에 밀린다"로도 읽혔다.

근본 원인은 공식의 숫자가 아니라 **밀치기 모듈이 이동 컴포넌트의 타이밍 모델(칸 수·이징 곡선·타일 간격)을 손으로 한 벌 더 들고 있었다**는 점이다. 식을 고쳐도 이징 계수나 타일 간격 하나만 바뀌면 다시 어긋나는 구조였다.

**해결** -
"언제 닿을지 예측"하는 대신 **닿은 뒤에 보고받도록** 바꿨다. 이동 컴포넌트는 이미 정확한 충돌 프레임(`Alpha`가 1.0을 넘는 순간)에 넉백 페이로드를 발화하고 있었으므로, 그 자리에서 이벤트 하나를 브로드캐스트하면 된다.

```cpp
// UACGridMovementComponent — 넉백 1건당 정확히 1회. 대상이 도중에 파괴돼도 EndPlay에서 보장
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGridKnockbackSettled, APE_CharacterBase*, Mover, int32, ChainID);

void UACGridMovementComponent::ExecuteKnockbackPayload()
{
    ...
    OnKnockbackSettled.Broadcast(OwnerChar, Payload.ChainID);  // 연쇄는 오직 이 이벤트로만 출발
}
```

`MoveAlongPath`의 `Delay` 인자는 완전히 삭제해 타이밍의 출처를 하나로 만들었다. 동시에 밀치기를 스킬에서 떼어내 세 계층으로 나눴다.

```
UPE_SkillEffect_Push          어댑터 — 에디터 설정값을 FPEPushRequest로 바꿔 넘길 뿐 (215줄 → 30줄)
        │ EnqueuePush
UPE_PushCoordinatorComponent  실행 주체 (GameState 부착) — 연쇄 전개 · 충돌 피해 · 액션 큐 토큰 소유
        │ ResolveSingle
FPEPushResolver               규칙의 단일 출처 — 상태 없는 순수 함수, 월드를 바꾸지 않음
```

서버는 도착 이벤트마다 `ResolveSingle`을 한 번씩 부르고, 클라이언트 조준 미리보기는 같은 함수를 시계 없이 끝까지 반복(`SimulateChain`)한다. 규칙이 한 곳뿐이라 미리보기와 실제 결과가 구조적으로 갈라질 수 없다. 전장 판정도 배치를 통째로 복사하지 않고 `AACGridSystem` 위의 얇은 뷰(`FPEPushField`)로 읽어, 밀치기마다 전장 전체를 스냅샷 뜨던 비용이 사라졌다.

이벤트 기반으로 바꾸면서 새로 생기는 위험 두 가지도 설계로 막았다.

- **연쇄 도중 액션 큐가 비는 문제** — 대상마다 토큰을 발급하면 한 링크가 끝나고 다음 링크가 시작되기 전 프레임에 토큰 수가 0이 되어 다음 스킬이 끼어든다. 연쇄 전체에 **토큰 1개**를 발급하고, 대기 요청과 이동 중인 대상이 모두 빌 때만 반납한다.
- **동기 재진입** — 0칸 밀치기는 `MoveAlongPath` 안에서 즉시 도착 이벤트가 되돌아와 코디네이터가 자기 자신을 다시 호출한다. 재진입은 표시만 하고 바깥 루프가 이어서 처리하도록 평탄화했다.

```cpp
void UPE_PushCoordinatorComponent::Drain()
{
    if (bIsDraining) { bDrainRequested = true; return; }  // 재진입은 표시만 하고 복귀
    bIsDraining = true;

    do
    {
        bDrainRequested = false;
        TArray<int32> ChainIDs;
        ActiveChains.GenerateKeyArray(ChainIDs);  // 전개 중 연쇄가 추가/종료될 수 있으므로 키를 먼저 뜬다

        for (int32 ChainID : ChainIDs)
        {
            const FPEPushChain* Chain = ActiveChains.Find(ChainID);
            if (!Chain || Chain->Pending.IsEmpty()) continue;
            DispatchGeneration(ChainID);
            bDrainRequested = true;
        }
    } while (bDrainRequested);

    bIsDraining = false;
    CloseSettledChains();  // 대기·이동 중이 모두 빈 연쇄만 토큰 반납
}
```

**결과** -
- `[1, 2, _, _, 3]`에서 3이 2와 부딪히는 바로 그 프레임에 출발한다. 예측값 자체가 없으므로 이동 속도·타일 간격을 바꿔도 어긋날 대상이 없다.
- `FPEPushRequest`만 만들면 스킬이 아닌 주체도 같은 경로로 밀치기를 일으킬 수 있다 (`EPEPushCause { Skill, Collision, Environment }`).
- 조준 미리보기와 서버 실행이 같은 `ResolveSingle`을 통과해 "예측과 다르게 밀렸다"는 괴리가 사라졌다.
- 연쇄가 토큰 1개로 묶여, 밀리던 대상이 충돌 피해로 사망해도 액션 큐가 멈추거나 다음 스킬이 끼어들지 않는다.

## License
이 저장소는 오픈소스가 아닙니다. 포트폴리오 열람 목적으로만 코드 확인이 가능하며, 명시적 서면 허가 없이 복제·수정·배포·상업적/비상업적 사용을 금지합니다. 자세한 내용은 [LICENSE](LICENSE)를 참고하세요.
