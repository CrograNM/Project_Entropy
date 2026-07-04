// Copyright CrograNM

#include "Components/ACCameraControlComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Grid/ACGridSystem.h"
#include "Kismet/GameplayStatics.h"

UACCameraControlComponent::UACCameraControlComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UACCameraControlComponent::BeginPlay()
{
	Super::BeginPlay();

	// 컴포넌트 스스로 GridSystem을 찾아 동적 구역을 설정
	if (AActor* FoundGridActor = UGameplayStatics::GetActorOfClass(GetWorld(), AACGridSystem::StaticClass()))
	{
		if (AACGridSystem* GridSystem = Cast<AACGridSystem>(FoundGridActor))
		{
			FVector GridMin, GridMax;
			GridSystem->GetGridWorldBounds(GridMin, GridMax);

			CameraMinBound.X = GridMin.X - CameraBoundsPadding;
			CameraMinBound.Y = GridMin.Y - CameraBoundsPadding;
			CameraMaxBound.X = GridMax.X + CameraBoundsPadding;
			CameraMaxBound.Y = GridMax.Y + CameraBoundsPadding;

			CameraMinBound.Z = GridMin.Z;
			CameraMaxBound.Z = GridMax.Z + MaxCameraHeightOffset;
		}
	}
}

void UACCameraControlComponent::InitCameraComponents(USceneComponent* InCameraBase, USpringArmComponent* InCameraBoom, UCameraComponent* InTopDownCamera)
{
	// 해당 함수는 캐릭터의 BeginPlay에서 호출하도록 설계되어 있음.
	CameraBase = InCameraBase;
	CameraBoom = InCameraBoom;
	TopDownCamera = InTopDownCamera;
}

void UACCameraControlComponent::PanCamera(FVector2D PanInput)
{
	if (!CameraBase || !TopDownCamera || PanInput.IsNearlyZero()) return;
	
	FVector Forward = TopDownCamera->GetForwardVector();
	Forward.Z = 0.f; 
	Forward.Normalize();

	FVector Right = TopDownCamera->GetRightVector();
	Right.Z = 0.f; 
	Right.Normalize();

	FVector MoveDelta = (Forward * PanInput.Y + Right * PanInput.X) * CameraPanSpeed;
	CameraBase->AddWorldOffset(MoveDelta);
	
	FVector ClampedLoc = CameraBase->GetComponentLocation();
	ClampedLoc.X = FMath::Clamp(ClampedLoc.X, CameraMinBound.X, CameraMaxBound.X);
	ClampedLoc.Y = FMath::Clamp(ClampedLoc.Y, CameraMinBound.Y, CameraMaxBound.Y);
	CameraBase->SetWorldLocation(ClampedLoc);
}

void UACCameraControlComponent::RotateCamera(FVector2D RotateInput)
{
	if (!CameraBase || !CameraBoom || RotateInput.IsNearlyZero()) return;

	CameraBase->AddWorldRotation(FRotator(0.f, RotateInput.X * CameraRotationSpeed, 0.f));

	FRotator BoomRot = CameraBoom->GetRelativeRotation();
	BoomRot.Pitch = FMath::Clamp(BoomRot.Pitch + (RotateInput.Y * CameraRotationSpeed), -85.f, -20.f);
	CameraBoom->SetRelativeRotation(BoomRot);
}

void UACCameraControlComponent::AdjustCameraHeight(float HeightInput)
{
	if (!CameraBase || FMath::IsNearlyZero(HeightInput)) return;
	
	FVector MoveDelta = FVector(0.f, 0.f, HeightInput * CameraHeightSpeed);
	CameraBase->AddWorldOffset(MoveDelta);
	
	FVector ClampedLoc = CameraBase->GetComponentLocation();
	ClampedLoc.Z = FMath::Clamp(ClampedLoc.Z, CameraMinBound.Z, CameraMaxBound.Z);
	CameraBase->SetWorldLocation(ClampedLoc);
}

void UACCameraControlComponent::ResetCameraPosition()
{
	if (bIsCameraFree || !CameraBase || !CameraBoom) return;
	
	CameraBase->SetRelativeLocation(FVector::ZeroVector);
	CameraBase->SetRelativeRotation(FRotator::ZeroRotator);
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
}

void UACCameraControlComponent::ToggleCameraFreeMode()
{
	SetCameraFreeMode(!bIsCameraFree);
}

void UACCameraControlComponent::SetCameraFreeMode(bool bEnable)
{
	if (bIsCameraFree == bEnable || !CameraBase) return;
	
	bIsCameraFree = bEnable;
	AActor* OwnerActor = GetOwner();

	if (bIsCameraFree)
	{
		CameraBase->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	}
	else
	{
		if (OwnerActor)
		{
			CameraBase->AttachToComponent(OwnerActor->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
		}
		ResetCameraPosition();
	}
}