// Copyright CrograNM


#include "Characters/PE_PlayerAnimInstance.h"

#include "Characters/PE_PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UPE_PlayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
	// 소유하고 있는 폰을 플레이어 캐릭터 타입으로 캐스팅하여 가져옵니다.
	PlayerCharacter = Cast<APE_PlayerCharacter>(TryGetPawnOwner());
	if (PlayerCharacter)
	{
		CharacterMovement = PlayerCharacter->GetCharacterMovement();
	}
}

void UPE_PlayerAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);
	
	// Initialize에서 실패했을 경우를 대비한 방어 코드
	if (!PlayerCharacter)
	{
		PlayerCharacter = Cast<APE_PlayerCharacter>(TryGetPawnOwner());
		if (PlayerCharacter)
		{
			CharacterMovement = PlayerCharacter->GetCharacterMovement();
		}
	}
	if (!PlayerCharacter || !CharacterMovement) return;

	// 캐릭터: 수평 속도 크기 계산
	FVector Velocity = PlayerCharacter->GetVelocity();
	Velocity.Z = 0.f;
	GroundSpeed = Velocity.Size();
}

