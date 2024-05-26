// Fill out your copyright notice in the Description page of Project Settings.


#include "ShatterableGlass.h"

// Sets default values
AShatterableGlass::AShatterableGlass()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Glass = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GLASS"));
	Glass->SetWorldScale3D(FVector(3.0f, 0.0f, 3.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh>
		SM_PLANE(TEXT("/Game/LevelPrototyping/Meshes/SM_Cube.SM_Cube"));
	if (SM_PLANE.Succeeded())
	{
		Glass->SetStaticMesh(SM_PLANE.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterial>
		MT_GLASS(TEXT("/Game/Fracture/M_Glass.M_Glass"));
	if (MT_GLASS.Succeeded())
	{
		Glass->SetMaterial(0, MT_GLASS.Object);
	}
}

// Called when the game starts or when spawned
void AShatterableGlass::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AShatterableGlass::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

