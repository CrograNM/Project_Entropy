// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ACCameraControlComponent.generated.h"

class USceneComponent;
class USpringArmComponent;
class UCameraComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECT_ENTROPY_API UACCameraControlComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UACCameraControlComponent();

	// 캐릭터의 BeginPlay에서 물리적 카메라 컴포넌트들을 넘겨받아 연동하는 함수
	void InitCameraComponents(USceneComponent* InCameraBase, USpringArmComponent* InCameraBoom, UCameraComponent* InTopDownCamera);

	// --- 카메라 조작 핵심 함수들 ---
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
	virtual void BeginPlay() override;
	
	// 제어할 물리적 컴포넌트 포인터
	UPROPERTY()
	TObjectPtr<USceneComponent> CameraBase;
	UPROPERTY()
	TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY()
	TObjectPtr<UCameraComponent> TopDownCamera;

	/** --- 카메라 세팅 변수들 --- */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	bool bIsCameraFree = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float CameraPanSpeed = 15.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float CameraRotationSpeed = 2.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float CameraHeightSpeed = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float CameraBoundsPadding = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float MaxCameraHeightOffset = 1000.f;

	// 동적 한계 구역 저장
	FVector CameraMinBound = FVector(-1000.f, -1000.f, 0.f);
	FVector CameraMaxBound = FVector(1000.f, 1000.f, 1000.f);
};
