// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PE_PlayerCharacter.generated.h"

UCLASS()
class PROJECT_ENTROPY_API APE_PlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APE_PlayerCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
