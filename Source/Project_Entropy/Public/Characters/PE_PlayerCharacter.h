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
	void AdjustCameraHeight(float HeightInput);
	
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void ResetCameraPosition();
	
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void ToggleCameraFreeMode();

	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetCameraFreeMode(bool bEnable);

	UFUNCTION(BlueprintCallable, Category = "Camera")
	bool IsCameraFree() const { return bIsCameraFree; }
	
protected:
	/** --- Camera --- */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> CameraBase;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> TopDownCamera;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	bool bIsCameraFree = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float CameraPanSpeed = 15.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float CameraRotationSpeed = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float CameraHeightSpeed = 10.f;
	
	// 타일 영역 끝에서 카메라가 얼마나 더 밖으로 나갈 수 있는지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float CameraBoundsPadding = 200.f;

	// 타일 바닥(Z) 기준으로 카메라가 올라갈 수 있는 최고 높이
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float MaxCameraHeightOffset = 1000.f;

	// 동적 카메라 한계선 저장 변수
	FVector CameraMinBound = FVector(-1000.f, -1000.f, 0.f);
	FVector CameraMaxBound = FVector(1000.f, 1000.f, 1000.f);
};

