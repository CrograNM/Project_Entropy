# Project Entropy

![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.7-0e1128?logo=unrealengine&logoColor=white)
![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey)
![Multiplayer](https://img.shields.io/badge/Multiplayer-Steam%20Online%20Subsystem-1b2838?logo=steam&logoColor=white)
![Development](https://img.shields.io/badge/Development-Solo%20(1인%20개발)-orange)
![License](https://img.shields.io/badge/License-All%20Rights%20Reserved-red)

> 3D 타일 기반 로그라이크 턴제 카드 게임. 카드/스킬/액션 큐 시스템을 설계하고, 반복적인 리팩토링을 통해 데이터 기반 콘텐츠 확장이 가능한 구조로 개발 중인 1인 개발 프로젝트입니다.

---

## ■ 핵심 기술 포인트

- **모듈형 스킬 이펙트 시스템** — `UPE_SkillEffectModule` 기반으로 데미지/넉백 등 효과를 "장착형 모듈"로 분리. 로직별 전용 클래스를 늘리지 않고, 데이터 에셋(`UPE_SkillData`)에서 모듈을 조합하는 것만으로 신규 스킬 제작 가능.
- **레퍼런스 카운팅 기반 액션 큐** — `APE_GameState`의 `TQueue<FPESkillActionPayload>`와 진행 카운터로 스킬 결제→애니메이션→판정의 비동기 순서를 서버가 강제. 하나의 스킬이 만드는 모든 파생 액션(투사체, 폭발, 연쇄 넉백)이 끝나야 다음 카드가 실행됨.
- **예측-판정 일치형 충돌 시스템** — 클라이언트 예측 시각화(스플라인)와 서버 실제 판정이 동일한 `SweepSingleByChannel` 스윕 알고리즘을 공유해, 직선/곡사/관통 스킬 모두 "보이는 대로 맞는" 결과를 보장.
- **PVP 멀티플레이 동기화** — Advanced Sessions + Steam Online Subsystem 기반 로비/매치메이킹, 시드 기반 랜덤 동기화, 팀 ID 기반 피아식별 및 턴 종료 만장일치 시스템.
- **상태 머신 기반 카드 상호작용** — 명시적 상태(`EPEInteractionState`)로 드래그/캐스팅/취소를 관리해 비동기 콜백 타이밍 버그를 구조적으로 차단하고, 나이아가라 VFX·커스텀 머티리얼로 완성한 손맛 있는 카드 연출(Juicy UX).

---

## ■ 스크린샷 / 데모

<!-- TODO: 실제 스크린샷/GIF로 교체 -->
| 카드 시스템 | 스킬 & VFX | PVP 대전 |
|---|---|---|
| ![카드 시스템 데모](docs/images/card-system.gif) | ![스킬 이펙트 데모](docs/images/skill-vfx.gif) | ![PVP 대전 데모](docs/images/pvp-battle.gif) |

<!-- TODO: 데모 버전 개발이 끝나면 플레이 영상 링크/썸네일 추가 -->
> ▶ **플레이 영상:** (링크 예정)

---

## 프로젝트 개요
- **장르:** 3D 타일 기반 로그라이크 턴제 카드 게임
- **주요 메커니즘:** 이동 행동 한번에 1AP 사용, 스킬을 통한 지형속성 부여와 속성 시너지, PVP 모드 존재
- **개발 인원:** 1인 개발

## 개발 기간 & 담당 역할
- **개발 기간:** 2026-04-24 ~ 2026-09-03 (약 4.3개월, 실 개발 활동일 기준 약 67일)
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

**문제**
연속으로 카드를 시전하면 방금 새로 손패에 들어온(또는 다음으로 잡은) 카드가 사용자가 조작하지도 않았는데 자동으로 소멸(버려짐)하는 버그가 있었다.

**원인**
당시 `UACCardInteractionComponent::OnCastingReadyFinished()` / `OnInstantCastFinished()`는 인자 없이 호출되어 "지금 콜백을 보낸 카드"와 "지금 `CastingCard`로 잡혀있는 카드"가 같은지 검증하지 않았다. 블루프린트 애니메이션 콜백은 비동기로 늦게 도착할 수 있어서, 이전 시전의 콜백이 늦게 들어오면 그 시점에 `CastingCard`로 잡혀 있던 **다음** 카드를 대상으로 산화 로직이 실행됐다.

**해결**
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

**결과**
"발신자가 지금 상태의 대상과 같은가"를 매번 검증하는 패턴이 구조적으로 강제되면서, 비동기 콜백/애니메이션 타이밍이 어긋나 엉뚱한 카드가 처리되는 종류의 버그가 재발하지 않는 환경이 됐다.

---

### 2. 데이터 기반 스킬/카드 시스템 — 모듈형 이펙트 구조

**문제**
스킬 하나하나를 데미지, 넉백, 범위 스폰 등 로직별 전용 클래스(`PE_SkillLogicBase`, `PE_SkillLogic_AoE`, `PE_SkillLogic_Push`, `PE_SkillLogic_SpawnActor`)로 만들다 보니, "AoE + 넉백"처럼 두 효과를 조합한 스킬을 만들려면 클래스를 새로 파거나 로직을 상속/복붙해야 해서 조합이 늘어날수록 유지보수가 어려워졌다.

**원인**
스킬의 "형태"(범위/투사체)와 "효과"(데미지/넉백/상태이상)가 하나의 클래스 안에 결합되어 있어, 새 조합마다 클래스 수가 곱으로 늘어나는 구조였다.

**해결**
공통 인터페이스만 정의하고 실제 효과는 서브클래스가 구현하도록 바꿨다.

```cpp
UCLASS(Abstract, DefaultToInstanced, EditInlineNew, Blueprintable)
class UPE_SkillEffectModule : public UObject
{
    virtual void ApplyEffects(AActor* Instigator, const TSet<APE_CharacterBase*>& Targets,
        const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage)
        PURE_VIRTUAL(UPE_SkillEffectModule::ApplyEffects, );
};
```

각 타격 페이즈(`FPESkillHitPhase`)가 이 모듈을 **배열로 장착**하도록 데이터 구조를 바꿨다.

```cpp
UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Phase|Effects")
TArray<TObjectPtr<UPE_SkillEffectModule>> EffectModules;
```

실행부(`ACSkillComponent::CommitQueuedSkill`)는 어떤 조합이든 동일하게 순회만 하면 된다.

```cpp
for (UPE_SkillEffectModule* Module : ExecPhase.EffectModules)
    if (Module) Module->ApplyEffects(Caster, AffectedTargets, PhaseTargetLoc, SkillData, FinalDamage);
```

**결과**
데미지(`UPE_SkillEffect_Damage`), 넉백(`UPE_SkillEffect_Push`)처럼 효과 하나당 모듈 클래스 하나만 만들면 되고, 새 스킬은 코드 수정 없이 `UPE_SkillData` 에셋에서 페이즈별로 원하는 모듈을 조합해 넣는 것만으로 완성된다. 실제로 이후 레이저·파도·화염구 등 신규 스킬들이 새 클래스 추가 없이 기존 Damage/Push 모듈 조합 + `HitPhases` 데이터 설정만으로 제작됐다.

---

### 3. 액션 큐 시스템

**문제**
(1) 카드를 연속으로 빠르게 시전하면 카드가 실행되지 못하고 소멸하는 버그, (2) 멀티플레이 환경에서 한 캐릭터의 "결제 → 애니메이션 → 실제 판정" 진행 도중 다른 스킬이 끼어들어 순서·피격 결과가 꼬이는 문제가 있었다.

**원인**
스킬 실행이 결제(즉시) → 클라이언트 애니메이션(비동기 RPC) → 실제 타격(타이머/멀티캐스트)의 여러 비동기 단계로 쪼개져 있는데, 이 단계들의 순서를 보장하는 장치가 없었다. 특히 스킬 하나가 여러 히트 페이즈나 파생 밀치기까지 만들어낼 수 있어서, 그중 하나가 끝나기도 전에 다음 스킬의 결제/실행이 시작될 수 있었다.

**해결**
`APE_GameState`에 서버 전용 큐와 레퍼런스 카운터를 두고, 진행 중인 파생 액션이 전부 끝나야 다음 큐 항목으로 넘어가게 만들었다.

```cpp
// APE_GameState
TQueue<FPESkillActionPayload> ActionQueue;
int32 PendingActionCount = 0;

void APE_GameState::ReportActionEnded(int32 ActionLogID)
{
    PendingActionCount--;
    RemoveActionLog(ActionLogID);
    if (PendingActionCount <= 0) // 파생된 모든 연산이 0이 될 때만
    {
        PendingActionCount = 0;
        GetWorld()->GetTimerManager().SetTimer(ActionDelayTimerHandle, this,
            &APE_GameState::ProcessNextAction, ActionInterval, false);
    }
}
```

스킬의 각 히트 페이즈, 그리고 넉백처럼 스킬에서 파생되는 2차 액션까지도 `ReportActionStarted()` / `ReportActionEnded()`로 카운트에 편입시켜, 하나의 카드가 만든 모든 부수 효과가 끝나야 다음 카드가 시작되도록 했다.

**결과**
카드 연속 시전 시 소멸 버그가 해결됐고, 멀티 환경에서도 "한 스킬의 전체 연쇄 결과가 끝나야 다음 스킬 처리 시작"이 보장돼 간섭 문제가 사라졌다. 부수적으로 큐에 쌓인 항목을 `FPEActionLogData` + `OnActionQueueUpdated` 델리게이트로 UI에 방송해, 플레이어가 지금 무엇이 대기 중인지 볼 수 있는 액션 로그 UI까지 파생됐다.

---

### 4. 현실적 스킬 충돌 판정 (직선/곡사/관통)

**문제**
초기에는 스킬을 "직선"이냐 "곡사"냐로만 대충 분류해 중간 대상을 건너뛸 수 있는지를 판단했다. 관통 스킬, 캐릭터/장애물의 실제 크기, 스킬의 예측 시각화(스플라인)와 실제 판정 결과의 일치 여부까지는 제대로 다루지 못했다.

**원인**
판정이 그리드 좌표 간 거리 계산 수준의 근사치였고, 실제 3D 캡슐 콜리전(캐릭터 몸통 크기, 장애물)과는 무관하게 동작해서 "화면에 보이는 궤적"과 "실제 맞는 대상"이 어긋날 수 있었다.

**해결**
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

**결과**
직선/곡사/관통(`bDestroyOnHit = false`) 스킬 전부가 하나의 스윕 알고리즘으로 처리 가능해졌고, 클라이언트가 미리 그려주는 예측 궤적(스플라인)과 서버의 실제 타격 결과가 같은 로직으로 계산되므로 "예측과 다르게 맞았다/빗나갔다"는 괴리가 사라졌다.

## License
이 저장소는 오픈소스가 아닙니다. 포트폴리오 열람 목적으로만 코드 확인이 가능하며, 명시적 서면 허가 없이 복제·수정·배포·상업적/비상업적 사용을 금지합니다. 자세한 내용은 [LICENSE](LICENSE)를 참고하세요.
