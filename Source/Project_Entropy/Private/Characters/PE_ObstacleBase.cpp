// Copyright CrograNM

#include "Characters/PE_ObstacleBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/ACStatComponent.h"

APE_ObstacleBase::APE_ObstacleBase()
{
	PrimaryActorTick.bCanEverTick = false;

	
	TeamID = -1;			// 제 3세력(중립)으로 설정하여 광역기 등에 무조건 데미지를 받도록 세팅
	bIsPushable = false;	// 장애물은 기본적으로 밀리지 않음 (에디터에서 true로 바꿀 수 있음)

	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionProfileName(TEXT("BlockAll"));
	}
}

void APE_ObstacleBase::HandleDeath()
{
	Super::HandleDeath();

	UE_LOG(LogTemp, Warning, TEXT("[Obstacle] %s 파괴됨!"), *GetName());

	// TODO: 장애물 파괴 전용 나이아가라 이펙트 및 폭발음 재생 추가 가능

	// 체력이 다하면 즉시 맵에서 소멸하여 길을 열어줌
	Destroy();
}