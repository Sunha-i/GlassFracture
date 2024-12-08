// Fill out your copyright notice in the Description page of Project Settings.


#include "DelaunayTriangulator.h"

TArray<Triangle> DelaunayTriangulator::ComputeTriangulation(const TArray<Point>& PointList)
{
	TArray<Triangle> Triangulation;

	// Step 1: Add super-triangle (bounding triangle large enough to contain all points)
	Triangle st = MakeSuperTriangle(PointList);
	Triangulation.Add(st);

	// Step 2: Triangulate each vertex
	for (const Point& point : PointList) {
		AddPoint(point, Triangulation);
	}

	// Step 3: Remove triangles that share edges with super-triangle
	Triangulation.RemoveAll([&st](const Triangle& triangle) {
		return (triangle.v0 == st.v0 || triangle.v0 == st.v1 || triangle.v0 == st.v2 ||
				triangle.v1 == st.v0 || triangle.v1 == st.v1 || triangle.v1 == st.v2 ||
				triangle.v2 == st.v0 || triangle.v2 == st.v1 || triangle.v2 == st.v2);
		});

	return Triangulation;
}

Triangle DelaunayTriangulator::MakeSuperTriangle(const TArray<Point>& pointList)
{
	double minx = std::numeric_limits<double>::infinity();
	double minz = std::numeric_limits<double>::infinity();
	double maxx = -std::numeric_limits<double>::infinity();
	double maxz = -std::numeric_limits<double>::infinity();

	for (const Point& point : pointList) {
		minx = FMath::Min(minx, point.x);
		minz = FMath::Min(minz, point.z);
		maxx = FMath::Max(maxx, point.x);
		maxz = FMath::Max(maxz, point.z);
	}

	double dx = (maxx - minx) * 10;
	double dz = (maxz - minz) * 10;

	Point v0(minx - dx, minz - dz * 3);
	Point v1(minx - dx, maxz + dz);
	Point v2(maxx + dx * 3, maxz + dz);

	return Triangle(v0, v1, v2);
}

void DelaunayTriangulator::AddPoint(const Point& point, TArray<Triangle>& triangulation)
{
	TArray<Edge> edges;

	triangulation.RemoveAll([&](const Triangle& triangle) {
		if (triangle.inCircumcircle(point)) {
			edges.Add(Edge(triangle.v0, triangle.v1));
			edges.Add(Edge(triangle.v1, triangle.v2));
			edges.Add(Edge(triangle.v2, triangle.v0));
			return true;
		}
		return false;
		});

	edges = UniqueEdges(edges);

	for (const Edge& edge : edges) {
		triangulation.Add(Triangle(edge.v0, edge.v1, point));
	}
}

TArray<Edge> DelaunayTriangulator::UniqueEdges(const TArray<Edge>& edges)
{
	TArray<Edge> uniqueEdges;

	for (size_t i = 0; i < edges.Num(); ++i) {
		bool isUnique = true;
		for (size_t j = 0; j < edges.Num(); ++j) {
			if (i != j && edges[i] == edges[j]) {
				isUnique = false;
				break;
			}
		}
		if (isUnique) {
			uniqueEdges.Add(edges[i]);
		}
	}

	return uniqueEdges;
}
