// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PE_PlayerCharacter.generated.h"

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

protected:
	/** --- Camera --- */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> TopDownCamera;

public:
	/** --- Getter --- */
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetTopDownCamera() const { return TopDownCamera; }
	
	/** ------ Move: On Grid ------ */
public:
	/** 캐릭터가 소유한 현재 그리드 위치 */
	FIntPoint GetGridPosition() const { return GridPosition; }
	void SetGridPosition(FIntPoint NewPos) { GridPosition = NewPos; }

	/** 주어진 타일 경로를 따라 순차 이동을 시작하는 함수 */
	void MoveAlongPath(const TArray<class AACTile*>& InPath);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Grid")
	FIntPoint GridPosition;

private:
	void ProcessNextPathStep();

	UPROPERTY()
	TArray<AACTile*> SavedPath;
	
	int32 CurrentPathIndex = 0;
	bool bIsMovingOnGrid = false;
	FVector TargetWorldLocation;
};

