// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "PE_CheatManager.generated.h"

class UACStatComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMapToolStateChangedSignature, bool, bIsActive);

UCLASS()
class PROJECT_ENTROPY_API UPE_CheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	// UFUNCTION(exec)
	// void Help();
	
	/** ---- 명령어: 플레이어 스탯 조작 ---- */
	UFUNCTION(exec)
	void SetHP(float NewHP);

	UFUNCTION(exec)
	void SetMaxHP(float NewMaxHP);

	UFUNCTION(exec)
	void SetAP(int32 NewAP);

	UFUNCTION(exec)
	void SetMaxAP(int32 NewMaxAP);

	UFUNCTION(exec)
	void SetMoveRange(int32 NewRange);

	/** ---- Map Tool 활성화 상태 제어 ---- */
	UPROPERTY(BlueprintAssignable, Category = "Cheat|Events")
	FOnMapToolStateChangedSignature OnMapToolStateChanged;
	
	UFUNCTION(BlueprintCallable, Category = "Cheat|MapTool")
	void SetMapToolActive(bool bActive);

	UFUNCTION(BlueprintPure, Category = "Cheat|MapTool")
	bool IsMapToolActive() const { return bIsMapToolActive; }
	
private:
	// Map Tool 활성화 상태 변수
	bool bIsMapToolActive = false;
};