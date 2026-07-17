// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "PE_CheatManager.generated.h"

class UACStatComponent;

UCLASS()
class PROJECT_ENTROPY_API UPE_CheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	// UFUNCTION(exec)
	// void Help();
	
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

	/** Map Tool 활성화 상태 제어 */
	UFUNCTION(BlueprintCallable, Category = "Cheat|MapTool")
	void SetMapToolActive(bool bActive);

	UFUNCTION(BlueprintPure, Category = "Cheat|MapTool")
	bool IsMapToolActive() const { return bIsMapToolActive; }
	
private:
	// 로컬 플레이어의 스탯 컴포넌트를 가져오는 헬퍼 함수
	UACStatComponent* GetPlayerStatComponent() const;
	
	// Map Tool 활성화 상태 변수
	bool bIsMapToolActive = false;
};