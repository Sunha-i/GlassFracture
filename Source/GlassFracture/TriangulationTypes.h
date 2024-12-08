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

	bool contains(const Point& point) const {
		float dx = center.x - point.x;
		float dz = center.z - point.z;
		return (std::sqrt(dx * dx + dz * dz) <= radius);
	}
};

struct Piece
	/* Pieces are convex components.The term 'cells' is used for the fracture pattern,
	while the term 'convex' refers to the convex parts of the compounds. */
{
	TArray<Point> points;
	TArray<Edge> edges;

	Piece(const TArray<Edge>& _edges, const TArray<Point>& _points)
		: points(_points), edges(_edges) {}

	Piece(const TArray<Point>& _points) : points(_points)
	{
		for (int32 i = 0; i < points.Num(); i++) {
			edges.Add(Edge(points[i], points[(i + 1) % points.Num()]));
		}
	}
};

struct Triangle
{
	Point v0, v1, v2;
	TArray<Edge> edges;

	Circle c;

	Triangle(const Point& _v0, const Point& _v1, const Point& _v2)
		: v0(_v0), v1(_v1), v2(_v2)
		, edges({ Edge(_v0, _v1), Edge(_v1, _v2), Edge(_v2, _v0) })
		, c(calcCircumcircle(_v0, _v1, _v2)) {}

	Circle calcCircumcircle(const Point& _v0, const Point& _v1, const Point& _v2)
	{
		float A = _v1.x - _v0.x;
		float B = _v1.z - _v0.z;
		float C = _v2.x - _v0.x;
		float D = _v2.z - _v0.z;

		float E = A * (_v0.x + _v1.x) + B * (_v0.z + _v1.z);
		float F = C * (_v0.x + _v2.x) + D * (_v0.z + _v2.z);

		float G = 2.0f * (A * (_v2.z - _v1.z) - B * (_v2.x - _v1.x));

		Point center(0, 0);
		float dx, dz;

		if (std::abs(G) < 1e-10)
		{
			float minx = std::min({ _v0.x, _v1.x, _v2.x });
			float minz = std::min({ _v0.z, _v1.z, _v2.z });
			float maxx = std::max({ _v0.x, _v1.x, _v2.x });
			float maxz = std::max({ _v0.z, _v1.z, _v2.z });

			center = Point((minx + maxx) / 2, (minz + maxz) / 2);

			dx = center.x - minx;
			dz = center.z - minz;
		}
		else {
			float cx = (D * E - B * F) / G;
			float cz = (A * F - C * E) / G;

			center = Point(cx, cz);

			dx = center.x - _v0.x;
			dz = center.z - _v0.z;
		}
		float radius = std::sqrt(dx * dx + dz * dz);

		return Circle(center, radius);
	}

	bool inCircumcircle(const Point& v) const {
		return c.contains(v);
	}
};