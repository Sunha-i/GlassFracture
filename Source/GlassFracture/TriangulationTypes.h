#pragma once

#include "CoreMinimal.h"

struct Point
{
	float x;
	float z;

	Point(float _x, float _z) : x(_x), z(_z) {}

	bool operator==(const Point& point) const {
		return (x == point.x && z == point.z);
	}
};

FORCEINLINE uint32 GetTypeHash(const Point& point)
{
	return HashCombine(GetTypeHash(point.x), GetTypeHash(point.z));
}

struct Edge
{
	Point v0;
	Point v1;

	Edge(const Point& _v0, const Point& _v1) : v0(_v0), v1(_v1) {}

	bool operator==(const Edge& edge) const {
		return (v0 == edge.v0 && v1 == edge.v1) || (v0 == edge.v1 && v1 == edge.v0);
	}
};

FORCEINLINE uint32 GetTypeHash(const Edge& edge)
{
	uint32 Hash1 = HashCombine(GetTypeHash(edge.v0), GetTypeHash(edge.v1));
	uint32 Hash2 = HashCombine(GetTypeHash(edge.v1), GetTypeHash(edge.v0));

	return FMath::Min(Hash1, Hash2);
}

struct Circle
{
	Point center;
	float radius;

	Circle(const Point& _center, float _radius)
		: center(_center), radius(_radius) {}
};

struct Piece
	/* Pieces are convex components.The term 'cells' is used for the fracture pattern,
	while the term 'convex' refers to the convex parts of the compounds. */
{
	TArray<Point> points;
	TArray<Edge> edges;

	Piece(const TArray<Edge>& _edges, const TArray<Point>& _points)
		: edges(_edges), points(_points) {}

	Piece(const TArray<Point>& _points) : points(_points)
	{
		for (int32 i = 0; i < points.Num(); i++) {
			edges.Add(Edge(points[i], points[(i + 1) % points.Num()]));
		}
	}
};