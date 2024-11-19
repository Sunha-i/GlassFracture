// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriangulationTypes.h"

#include "ShatterableGlass.generated.h"

UCLASS()
class GLASSFRACTURE_API AShatterableGlass : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AShatterableGlass();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Glass;

public:
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:
	UPROPERTY(VisibleAnywhere)	FVector LocalMinBound;
	UPROPERTY(VisibleAnywhere)	FVector LocalMaxBound;

	TArray<Piece> PatternCells;
	TArray<Piece> GridPolygons;

	void CreateFracturePattern(const FVector& ImpactPosition);
	void CreateGridPolygons(int32 rows, int32 cols);
	void VisualizePieces(const TArray<Piece>& Pieces, bool bRandomizeColor, float Duration);
};
