// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Characters/PE_CharacterBase.h"
#include "Core/PE_TurnManagerComponent.h"
#include "PE_PlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UACCameraControlComponent;

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
	FORCEINLINE UACCameraControlComponent* GetCameraControlComponent() const { return CameraControlComponent; } 
	
protected:
	/** --- Camera --- */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> CameraBase;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> TopDownCamera;
	
	// 카메라 제어 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UACCameraControlComponent> CameraControlComponent;
};

