// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriangulationTypes.h"
#include "ProceduralMeshComponent.h"
#include "Engine/DataTable.h"

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

	UPROPERTY(VisibleAnywhere)
	UProceduralMeshComponent* ProcMesh;

public:
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:
	UPROPERTY(VisibleAnywhere)	FVector LocalMinBound;
	UPROPERTY(VisibleAnywhere)	FVector LocalMaxBound;

	UPROPERTY(EditAnywhere, Category = "FracturePattern")
	UDataTable* PolygonDataTable;

	UPROPERTY(EditAnywhere, Category = "FracturePattern")
	UDataTable* VertexDataTable;

	TArray<Piece> PatternCells;
	TArray<Piece> GridPolygons;

	void CreateGridPolygons(int32 rows, int32 cols);

	void GeneratePieceMeshes(const TArray<Piece>& Pieces, const TMap<int32, TArray<int32>>& CellToPiecesMap);
	void FanTriangulation(const Piece& Piece, TArray<int32>& Triangles, TArray<FVector>& MeshVertices);

	void VisualizePieces(const TArray<Piece>& Pieces, bool bRandomizeColor, float Duration);
	void GenerateCube();
};
