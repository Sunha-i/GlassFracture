// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GlassFracture/TriangulationTypes.h"

/**
 * DelaunayTriangulator is a utility class for computing Delaunay triangulation based on the Bowyer-Watson algorithm.
 */
class GLASSFRACTURE_API DelaunayTriangulator
{
public:
	static TArray<Triangle> ComputeTriangulation(const TArray<Point>& PointList);

private:
	static Triangle MakeSuperTriangle(const TArray<Point>& pointList);
	static void AddPoint(const Point& point, TArray<Triangle>& triangulation);
	static TArray<Edge> UniqueEdges(const TArray<Edge>& edges);
};
