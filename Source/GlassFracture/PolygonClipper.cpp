// Fill out your copyright notice in the Description page of Project Settings.


#include "PolygonClipper.h"

/* Sutherland-Hodgman Polygon Clipping Algorithm */
TArray<Point> PolygonClipper::PerformClipping(const TArray<Point>& SubjectPolygon, const TArray<Point>& ClipPolygon)
{
	TArray<Point> outputPolygon = SubjectPolygon;

	for (int i = 0; i < ClipPolygon.Num(); ++i) {
		Point clipEdgeStart = ClipPolygon[i];
		Point clipEdgeEnd = ClipPolygon[(i + 1) % ClipPolygon.Num()];

		TArray<Point> inputPolygon = outputPolygon;
		outputPolygon.Empty();

		for (int32 j = 0; j < inputPolygon.Num(); ++j) {
			Point currPoint = inputPolygon[j];
			Point prevPoint = inputPolygon[(j - 1 + inputPolygon.Num()) % inputPolygon.Num()];

			bool currInside = IsInside(currPoint, clipEdgeStart, clipEdgeEnd);
			bool prevInside = IsInside(prevPoint, clipEdgeStart, clipEdgeEnd);

			if (currInside) {
				if (!prevInside) {
					outputPolygon.Add(ComputeIntersection(prevPoint, currPoint, clipEdgeStart, clipEdgeEnd));
				}
				outputPolygon.Add(currPoint);
			}
			else if (prevInside) {
				outputPolygon.Add(ComputeIntersection(prevPoint, currPoint, clipEdgeStart, clipEdgeEnd));
			}
		}
	}
	return outputPolygon;
}

/* Function to check if a point is inside the clipping boundary */
bool PolygonClipper::IsInside(const Point& point, const Point& edgeStart, const Point& edgeEnd)
{
	float crossProduct = (edgeEnd.x - edgeStart.x) * (point.z - edgeStart.z)
						- (edgeEnd.z - edgeStart.z) * (point.x - edgeStart.x);
	return crossProduct <= -KINDA_SMALL_NUMBER;
}

/* Function to find the intersection of a polygon edge with the clipping boundary */
Point PolygonClipper::ComputeIntersection(const Point& p1, const Point& p2, const Point& e1, const Point& e2)
{
	float a1 = p2.z - p1.z;
	float b1 = p1.x - p2.x;
	float c1 = a1 * p1.x + b1 * p1.z;

	float a2 = e2.z - e1.z;
	float b2 = e1.x - e2.x;
	float c2 = a2 * e1.x + b2 * e1.z;

	float det = a1 * b2 - a2 * b1;

	if (FMath::Abs(det) < KINDA_SMALL_NUMBER) {
		// Lines are nearly parallel, no intersection. Return an arbitrary point.
		return Point(0.0f, 0.0f);
	}
	float x = (b2 * c1 - b1 * c2) / det;
	float z = (a1 * c2 - a2 * c1) / det;

	return Point(x, z);
}
