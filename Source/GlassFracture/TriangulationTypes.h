#pragma once

#include "CoreMinimal.h"

struct Vertex
{
	float x;
	float z;

	Vertex(float _x, float _z) : x(_x), z(_z) {}
};

struct Edge
{
	Vertex v0;
	Vertex v1;

	Edge(const Vertex& _v0, const Vertex& _v1) : v0(_v0), v1(_v1) {}
};

struct Piece
	/* Pieces are convex components.The term 'cells' is used for the fracture pattern,
	while the term 'convex' refers to the convex parts of the compounds. */
{
	TArray<Vertex> vertices;
	TArray<Edge> edges;

	Piece(const TArray<Edge>& _edges, const TArray<Vertex>& _vertices)
		: edges(_edges), vertices(_vertices) {}
};