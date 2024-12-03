// Fill out your copyright notice in the Description page of Project Settings.


#include "FracturePatternGenerator.h"
#include "PolygonData.h"
#include "VertexData.h"
#include "Algo/Reverse.h"

TArray<Piece> FracturePatternGenerator::CreateSpiderwebPieces(const FVector& ImpactLocation, const FVector& ActorLocation, const UDataTable* PolygonDataTable, const UDataTable* VertexDataTable)
{
    TArray<Piece> Pieces;

    if (!PolygonDataTable || !VertexDataTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid DataTable(s) provided."));
        return Pieces;
    }

    FString CenterPointIdx = FString::FromInt(366);
    FVertexData* CenterPoint = VertexDataTable->FindRow<FVertexData>(FName(*CenterPointIdx), TEXT("" ));

    if (!CenterPoint)
    {
        UE_LOG(LogTemp, Warning, TEXT("Reference Index: %d not found in VertexDataTable"), 366);
        return Pieces;
    }

    const float ScaleFactor = 0.22f;
    FVector ReferenceLocation(CenterPoint->X * ScaleFactor, 0.0f, CenterPoint->Y * ScaleFactor);
    FVector Offset = ImpactLocation - ReferenceLocation;

    TArray<FPolygonData*> PolygonRows;
    PolygonDataTable->GetAllRows<FPolygonData>(TEXT(""), PolygonRows);

    for (int32 i = 0; i < PolygonRows.Num(); i++)
    {
        FPolygonData* PolygonRow = PolygonRows[i];
        if (PolygonRow)
        {
            TArray<int32> VertexIndices = ConvertStringToIntArray(PolygonRow->VertexIndices);
            Algo::Reverse(VertexIndices);

            TArray<Point> Points;
            TArray<Edge> Edges;

            for (int32 idx = 0; idx < VertexIndices.Num() - 1; idx++)
            {
                int32 Index = VertexIndices[idx];
                FString RowName = FString::FromInt(Index);
                FVertexData* VertexRow = VertexDataTable->FindRow<FVertexData>(FName(*RowName), TEXT(""));

                if (VertexRow)
                {
                    float ScaledX = VertexRow->X * ScaleFactor;
                    float ScaledY = VertexRow->Y * ScaleFactor;

                    FVector VertexPosition = FVector(ScaledX, 0.0f, ScaledY) + Offset;
                    Points.Add(Point(VertexPosition.X, VertexPosition.Z));
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Vertex Index: %d not found in VertexDataTable"), Index);
                }
            }

            Pieces.Add(Piece(Points));
        }
    }

    return Pieces;
}

UDataTable* FracturePatternGenerator::LoadFracturePatternDataTable(const FString& DataTablePath)
{
    ConstructorHelpers::FObjectFinder<UDataTable> DataTable(*DataTablePath);
    if (DataTable.Succeeded())
    {
        return DataTable.Object;
    }
    UE_LOG(LogTemp, Error, TEXT("Failed to load DataTable at path: %s"), *DataTablePath);

    return nullptr;
}

TArray<int32> FracturePatternGenerator::ConvertStringToIntArray(const FString& StringData)
{
    TArray<int32> ResultArray;
    TArray<FString> SplitStrings;
    StringData.ParseIntoArray(SplitStrings, TEXT(","), true);

    for (const FString& NumberStr : SplitStrings)
    {
        int32 Number = FCString::Atoi(*NumberStr);
        ResultArray.Add(Number);
    }

    return ResultArray;
}

TArray<Piece> FracturePatternGenerator::CreateDiagonalPieces(const FVector& ImpactLocation, const FVector& HalfSize, const FVector& ActorLocation)
{
    TArray<Piece> Pieces;

    FVector LocalImpactPoint = ImpactLocation - ActorLocation;

    // Define vertices
    FVector TopLeft = LocalImpactPoint + FVector(-HalfSize.X, 0.0f, HalfSize.Z);
    FVector TopRight = LocalImpactPoint + FVector(HalfSize.X, 0.0f, HalfSize.Z);
    FVector BottomLeft = LocalImpactPoint + FVector(-HalfSize.X, 0.0f, -HalfSize.Z);
    FVector BottomRight = LocalImpactPoint + FVector(HalfSize.X, 0.0f, -HalfSize.Z);
    FVector Center = (TopLeft + TopRight + BottomLeft + BottomRight) / 4;   // same as LocalImpactPoint

    // Add pieces (reversed order for clockwise direction)
    Pieces.Add(Piece(
        { Edge(Point(TopLeft.X, TopLeft.Z), Point(TopRight.X, TopRight.Z)),
          Edge(Point(TopRight.X, TopRight.Z), Point(Center.X, Center.Z)),
          Edge(Point(Center.X, Center.Z), Point(TopLeft.X, TopLeft.Z)) },
        { Point(Center.X, Center.Z), Point(TopLeft.X, TopLeft.Z), Point(TopRight.X, TopRight.Z) }));

    Pieces.Add(Piece(
        { Edge(Point(TopRight.X, TopRight.Z), Point(BottomRight.X, BottomRight.Z)),
          Edge(Point(BottomRight.X, BottomRight.Z), Point(Center.X, Center.Z)),
          Edge(Point(Center.X, Center.Z), Point(TopRight.X, TopRight.Z)) },
        { Point(Center.X, Center.Z), Point(TopRight.X, TopRight.Z), Point(BottomRight.X, BottomRight.Z) }));

    Pieces.Add(Piece(
        { Edge(Point(BottomRight.X, BottomRight.Z), Point(BottomLeft.X, BottomLeft.Z)),
          Edge(Point(BottomLeft.X, BottomLeft.Z), Point(Center.X, Center.Z)),
          Edge(Point(Center.X, Center.Z), Point(BottomRight.X, BottomRight.Z)) },
        { Point(Center.X, Center.Z), Point(BottomRight.X, BottomRight.Z), Point(BottomLeft.X, BottomLeft.Z) }));

    Pieces.Add(Piece(
        { Edge(Point(BottomLeft.X, BottomLeft.Z), Point(TopLeft.X, TopLeft.Z)),
          Edge(Point(TopLeft.X, TopLeft.Z), Point(Center.X, Center.Z)),
          Edge(Point(Center.X, Center.Z), Point(BottomLeft.X, BottomLeft.Z)) },
        { Point(Center.X, Center.Z), Point(BottomLeft.X, BottomLeft.Z), Point(TopLeft.X, TopLeft.Z) }));

    return Pieces;
}