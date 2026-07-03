// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Characters/PE_CharacterBase.h"
#include "Core/PE_TurnManagerComponent.h"
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
	
	/** 턴 매니저의 페이즈 변경 신호를 받을 콜백 함수 */
	UFUNCTION()
	void OnBattlePhaseChanged(EPEBattlePhase NewPhase);
	
public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** --- Getter --- */
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetTopDownCamera() const { return TopDownCamera; }
	
	// 배틀 모드 카메라 조작 함수
	void PanCamera(FVector2D PanInput);
	void RotateCamera(FVector2D RotateInput);
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void ResetCameraPosition();
	
protected:
	/** --- Camera --- */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> CameraBase;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> TopDownCamera;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float CameraPanSpeed = 30.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float CameraRotationSpeed = 2.f;
};

