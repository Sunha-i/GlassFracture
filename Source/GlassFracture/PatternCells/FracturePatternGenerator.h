// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GlassFracture/TriangulationTypes.h"
#include "Engine/DataTable.h"

/**
 * 
 */
class GLASSFRACTURE_API FracturePatternGenerator
{
public:
	static TArray<Piece> CreateSpiderwebPieces(const FVector& ImpactLocation, const FVector& ActorLocation,
			const UDataTable* PolygonDataTable, const UDataTable* VertexDataTable);
	static TArray<Piece> CreateDiagonalPieces(const FVector& ImpactLocation, const FVector& HalfSize, const FVector& ActorLocation);

private:
	static UDataTable* LoadFracturePatternDataTable(const FString& DataTablePath);
	static TArray<int32> ConvertStringToIntArray(const FString& StringData);
};
