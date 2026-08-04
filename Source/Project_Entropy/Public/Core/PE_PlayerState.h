// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "PE_PlayerState.generated.h"

UCLASS()
class PROJECT_ENTROPY_API APE_PlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	APE_PlayerState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void CopyProperties(APlayerState* PlayerState) override;

	UFUNCTION(BlueprintCallable, Category = "Team")
	int32 GetTeamID() const { return TeamID; }

	UFUNCTION(BlueprintCallable, Category = "Team")
	void SetTeamID(int32 InTeamID);

	// 클라이언트 선제 반영(Prediction)을 위한 로컬 전용 팀 설정
	UFUNCTION(BlueprintCallable, Category = "Team")
	void SetTeamIDLocal(int32 InTeamID);

protected:
	// 팀 ID (0: 아군/A팀, 1: 적군/B팀 등)
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_TeamID, Category = "Team")
	int32 TeamID;

	UFUNCTION()
	void OnRep_TeamID();
};