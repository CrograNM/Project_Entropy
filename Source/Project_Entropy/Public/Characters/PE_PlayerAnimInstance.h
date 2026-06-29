// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PE_PlayerAnimInstance.generated.h"

class APE_PlayerCharacter;
class UCharacterMovementComponent;

/**
 * 
 */
UCLASS()
class PROJECT_ENTROPY_API UPE_PlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaTime) override;

protected:
	/** 캐릭터 참조 */
	UPROPERTY(BlueprintReadOnly, Category = "Character")
	TObjectPtr<APE_PlayerCharacter> PlayerCharacter;

	/** 캐릭터의 무브먼트 컴포넌트 참조 */
	UPROPERTY(BlueprintReadOnly, Category = "Character")
	TObjectPtr<UCharacterMovementComponent> CharacterMovement;

	/** 애니메이션 블렌딩에 사용할 수평(지면) 속도 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float GroundSpeed;
};

