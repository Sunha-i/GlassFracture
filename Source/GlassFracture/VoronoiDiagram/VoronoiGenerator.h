// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GlassFracture/TriangulationTypes.h"

/**
 * 
 */
class GLASSFRACTURE_API VoronoiGenerator
{
public:
	static TArray<Piece> GenerateVoronoiCells(const TArray<Point>& RandomPoints, const FVector& LocalMinBound, const FVector& LocalMaxBound);

private:
	static TArray<Piece> CreateVoronoiPieces(const TMap<Point, TArray<Point>>& VoronoiCells, const TArray<Point>& BoundingBox);
};
