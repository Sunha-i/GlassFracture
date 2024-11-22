#pragma once

#include "CoreMinimal.h"

struct Point
{
	float x;
	float z;

	Point(float _x, float _z) : x(_x), z(_z) {}
};

struct Edge
{
	Point v0;
	Point v1;

	Edge(const Point& _v0, const Point& _v1) : v0(_v0), v1(_v1) {}
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