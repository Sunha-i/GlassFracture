// Fill out your copyright notice in the Description page of Project Settings.


#include "VoronoiGenerator.h"
#include "DelaunayTriangulator.h"
#include "GlassFracture/PolygonClipper.h"

TArray<Piece> VoronoiGenerator::GenerateVoronoiCells(const TArray<Point>& RandomPoints, const FVector& LocalMinBound, const FVector& LocalMaxBound)
{
	TArray<Piece> VoronoiPieces;

	TArray<Point> BoundingBox = {
		Point(LocalMinBound.X, LocalMinBound.Z),	// Bottom-Left
		Point(LocalMinBound.X, LocalMaxBound.Z),	// Top-Left
		Point(LocalMaxBound.X, LocalMaxBound.Z),	// Top-Right
		Point(LocalMaxBound.X, LocalMinBound.Z)		// Bottom-Right
	};

	TArray<Triangle> DelaunayTriangles = DelaunayTriangulator::ComputeTriangulation(RandomPoints);
	TMap<Point, TArray<Point>> VoronoiCells;

	for (const Triangle& Triangle : DelaunayTriangles)
	{
		Point Circumcenter = Triangle.c.center;

		VoronoiCells.FindOrAdd(Triangle.v0).Add(Circumcenter);
		VoronoiCells.FindOrAdd(Triangle.v1).Add(Circumcenter);
		VoronoiCells.FindOrAdd(Triangle.v2).Add(Circumcenter);
	}

	for (auto& Cell : VoronoiCells)
	{
		Point Site = Cell.Key;
		TArray<Point>& Circumcenters = Cell.Value;

		Circumcenters.Sort([&Site](const Point& A, const Point& B) {
			double AngleA = FMath::Atan2(A.z - Site.z, A.x - Site.x);
			double AngleB = FMath::Atan2(B.z - Site.z, B.x - Site.x);
			return AngleA < AngleB;
		});
	}

	VoronoiPieces = CreateVoronoiPieces(VoronoiCells, BoundingBox);

	return VoronoiPieces;
}

TArray<Piece> VoronoiGenerator::CreateVoronoiPieces(const TMap<Point, TArray<Point>>& VoronoiCells, const TArray<Point>& BoundingBox)
{
	TArray<Piece> VoronoiPieces;

	for (const auto& Cell : VoronoiCells)
	{
		const Point& Site = Cell.Key;
		const TArray<Point>& Circumcenters = Cell.Value;

		TArray<Point> VoronoiPolygon;
		for (const Point& V : Circumcenters)
		{
			VoronoiPolygon.Add(Point(V.x, V.z));
		}

		TArray<Point> ClippedPolygon = PolygonClipper::PerformClipping(VoronoiPolygon, BoundingBox);

		if (ClippedPolygon.Num() > 2)
		{
			VoronoiPieces.Add(Piece(ClippedPolygon));
		}
	}

	return VoronoiPieces;
}
