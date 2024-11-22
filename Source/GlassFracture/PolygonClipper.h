// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TriangulationTypes.h"

/**
 * PolygonClipper is a utility class for performing polygon clipping operations using the Sutherland-Hodgman algorithm.
 */
class GLASSFRACTURE_API PolygonClipper
{
public:
	static TArray<Point> PerformClipping(const TArray<Point>& SubjectPolygon, const TArray<Point>& ClipPolygon);

private:
	static bool IsInside(const Point& point, const Point& edgeStart, const Point& edgeEnd);
	static Point ComputeIntersection(const Point& v1, const Point& v2, const Point& e1, const Point& e2);
};
