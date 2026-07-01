// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Characters/PE_CharacterBase.h"
#include "PE_PlayerCharacter.generated.h"

class UACGridMovementComponent;
class USpringArmComponent;
class UCameraComponent;

UCLASS()
class PROJECT_ENTROPY_API APE_PlayerCharacter : public APE_CharacterBase
{
	GENERATED_BODY()

public:
	APE_PlayerCharacter();
	
protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** --- Getter --- */
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetTopDownCamera() const { return TopDownCamera; }
	
protected:
	/** --- Camera --- */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> TopDownCamera;
};

