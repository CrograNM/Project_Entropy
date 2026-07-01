// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PE_PlayerCharacter.generated.h"

class UACGridMovementComponent;
class USpringArmComponent;
class UCameraComponent;

UCLASS()
class PROJECT_ENTROPY_API APE_PlayerCharacter : public ACharacter
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
	FORCEINLINE UACGridMovementComponent* GetGridMovementComponent() const { return GridMovement; }
	
protected:
	/** --- Camera --- */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> TopDownCamera;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UACGridMovementComponent> GridMovement;
};

