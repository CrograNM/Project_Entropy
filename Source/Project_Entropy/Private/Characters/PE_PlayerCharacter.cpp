// Copyright CrograNM


#include "Characters/PE_PlayerCharacter.h"

// Sets default values
APE_PlayerCharacter::APE_PlayerCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APE_PlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APE_PlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void APE_PlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

