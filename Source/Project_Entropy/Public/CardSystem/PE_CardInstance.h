// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PE_CardInstance.generated.h"

class UPE_CardData;

/**
 * 런타임에 존재하는 카드의 실제 인스턴스 객체
 * 원본 데이터(DataAsset)를 참조하며, 마석(Modifier)이나 버프에 의해 변동된 수치를 저장하고 반환
 */
UCLASS(BlueprintType, Blueprintable)
class PROJECT_ENTROPY_API UPE_CardInstance : public UObject
{
	GENERATED_BODY()

public:
	UPE_CardInstance();

	UFUNCTION(BlueprintCallable, Category = "Card Instance")
	void Initialize(UPE_CardData* InBaseData);

	/** --- Getter --- */
	UFUNCTION(BlueprintCallable, Category = "Card Instance")
	UPE_CardData* GetBaseCardData() const { return BaseCardData; }

	UFUNCTION(BlueprintCallable, Category = "Card Instance|Stats")
	int32 GetCalculatedAPCost() const;	// 런타임 변동 수치까지 반영된 최종 AP 코스트 반환

	UFUNCTION(BlueprintCallable, Category = "Card Instance|Stats")
	float GetCalculatedDamage() const;	// 런타임 변동 수치까지 반영된 최종 데미지 반환

	UFUNCTION(BlueprintCallable, Category = "Card Instance|Stats")
	float GetCalculatedHeal() const;	// 런타임 변동 수치까지 반영된 최종 힐량 반환

	// TODO: 마석(Modifier) 부착/해제 및 저장용 구조체 연동 로직 추가 예정
	// UFUNCTION(BlueprintCallable, Category = "Card Instance|Modifier")
	// void ApplyMagicStone(...);

private:
	/** 원본 읽기 전용 데이터 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card Instance|Data", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPE_CardData> BaseCardData;

	// --- 런타임 변동 수치 캐싱 (디스크에 세이브/로드 될 데이터들) ---
	UPROPERTY(VisibleAnywhere, Category = "Card Instance|Modifiers")
	int32 CostModifier = 0; // AP 코스트 증감 수치 (예: -1이면 코스트 1 감소)

	UPROPERTY(VisibleAnywhere, Category = "Card Instance|Modifiers")
	float DamageModifier = 0.f; // 추가 데미지

	UPROPERTY(VisibleAnywhere, Category = "Card Instance|Modifiers")
	float HealModifier = 0.f; // 추가 힐량
};