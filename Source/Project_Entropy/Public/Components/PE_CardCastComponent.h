// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PE_CardCastComponent.generated.h"

class AACTile;
class APE_CardActor;
class APE_CharacterBase;
class APE_PlayerController;
class UPE_SkillData;

/*
	FPECardCastRequest - 시전 요청 1건의 추적 정보
	
	요청 ID로 이 카드를 다시 찾아냄
*/
USTRUCT()
struct FPECardCastRequest
{
	GENERATED_BODY()

	UPROPERTY() TObjectPtr<APE_CardActor> Card = nullptr;

	// 턴 종료 자동 발동처럼 AP를 쓰지 않는 시전인지 여부 (연출 종류가 갈립니다)
	UPROPERTY() bool bIsFreeCast = false;
};

/*
	UPE_CardCastComponent - 카드 시전의 클라이언트-서버 왕복을 소유
	
	요청 ID를 발급해 카드와 묶어두고, 
	서버의 확정/취소가 돌아올 때까지 그 매핑을 유지

	손패 위의 마우스 상태(UACCardInteractionComponent)와는 접점 3개로만 연결
	- 넘길 때   : Interaction::ReleaseCard -> TryExecuteCardDrop
	- 되돌릴 때 : CompleteCasting / CancelCasting
	
	타겟 규칙은 FPETargetRules가, 
	실제 판정은 UACSkillComponent가, 
	실행 순서는 APE_GameState(액션큐)가 가짐.

	이 컴포넌트가 갖는 것은 "어느 요청이 어느 카드였는가" 하나뿐
*/
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECT_ENTROPY_API UPE_CardCastComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPE_CardCastComponent();

	// ----- [시전 개시] -----

	// 카드를 시전 구역에 놓았을 때: AP와 타겟을 확인하고 통과하면 요청을 보냄
	UFUNCTION(BlueprintCallable, Category = "Card Cast")
	void TryExecuteCardDrop(APE_CardActor* DroppedCard);

	// 카드 시전 요청: 요청 ID를 발급해 카드와 묶고 서버로 보냄. 타겟 검증은 호출자가 이미 끝낸 상태여야 함
	UFUNCTION(BlueprintCallable, Category = "Card Cast")
	void SendCardCastRequest(UPE_SkillData* SkillData, AACTile* TargetTile, APE_CharacterBase* TargetCharacter, APE_CardActor* SourceCard, bool bIsFreeCast = false);

	// 카드 강제 시전: 무작위 유효 대상을 골라 무료 시전, 대상이 없으면 카드를 버림
	UFUNCTION(BlueprintCallable, Category = "Card Cast")
	void ForceTriggerCardLocally(APE_CardActor* TriggeredCard);


	// ----- [카드 연출 종료 통보 (APE_CardActor가 호출)] -----

	// 산화 연출 종료 --> 턴 종료 실패 카드면 다음 카드로, 시전 카드면 서버에 완료를 보고
	UFUNCTION(BlueprintCallable, Category = "Card Cast")
	void NotifyDiscardAnimFinished(APE_CardActor* Card);

	// 턴 종료 카드의 시전 준비 연출 종료 --> 이제 실제로 발동
	UFUNCTION(BlueprintCallable, Category = "Card Cast")
	void NotifyTurnEndReadyAnimFinished(APE_CardActor* Card);


	// ----- [서버 --> 소유 클라이언트] -----

	// 액션 큐 차례가 되었으니 카드 연출을 재생하라는 지시 
	UFUNCTION(Client, Reliable)
	void Client_PlayCardCastAnim(int32 CastRequestID);

	// 서버가 실제 판정까지 마쳤으므로 카드를 무덤으로 확정
	UFUNCTION(Client, Reliable)
	void Client_ConfirmCardCast(int32 CastRequestID);

	// 서버 검증에 실패했으므로 카드를 손패로 되돌림
	UFUNCTION(Client, Reliable)
	void Client_CancelCardCast(int32 CastRequestID);

	// 손패에 남은 턴 종료 발동 카드를 한 장씩 처리하라는 지시
	UFUNCTION(Client, Reliable)
	void Client_TriggerTurnEndCards();

protected:
	virtual void BeginPlay() override;

private:
	// ----- [클라이언트 --> 서버] -----

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestCardCast(UPE_SkillData* SkillData, AACTile* TargetTile, APE_CharacterBase* TargetCharacter, int32 CastRequestID, bool bIsFreeCast);

	// 클라이언트 연출이 끝났으니 실제 판정을 진행해도 된다는 보고
	UFUNCTION(Server, Reliable)
	void Server_NotifyCardCastAnimFinished(int32 CastRequestID);

	// 더 이상 발동할 턴 종료 카드가 없다는 보고
	UFUNCTION(Server, Reliable)
	void Server_TurnEndCardsFinished();

	// 사거리 안의 유효 대상 중 하나를 고름. 지정이 필요 없는 스킬은 대상 없이 true
	bool PickRandomValidTarget(UPE_SkillData* SkillData, AACTile*& OutTile, APE_CharacterBase*& OutChar);


	// ----- [상태] -----

	int32 NextCastRequestID = 0;

	// 요청 ID -> 그 요청을 만든 카드
	UPROPERTY()
	TMap<int32, FPECardCastRequest> PendingRequests;

	// 준비 연출이 끝나기를 기다리는 턴 종료 카드
	UPROPERTY()
	TObjectPtr<APE_CardActor> PendingTurnEndCard = nullptr;

	// 대상이 없어 폐기 중인 턴 종료 카드. (산화가 끝나야 다음 장으로 넘어감)
	UPROPERTY()
	TObjectPtr<APE_CardActor> FailedTurnEndCard = nullptr;

	UPROPERTY()
	TObjectPtr<APE_PlayerController> OwnerPC = nullptr;
};
