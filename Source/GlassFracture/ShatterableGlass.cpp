// Fill out your copyright notice in the Description page of Project Settings.


#include "ShatterableGlass.h"

// Sets default values
AShatterableGlass::AShatterableGlass()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Glass = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GLASS"));
	Glass->SetWorldScale3D(FVector(3.0f, 0.0f, 3.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh>
		SM_PLANE(TEXT("/Game/LevelPrototyping/Meshes/SM_Cube.SM_Cube"));
	if (SM_PLANE.Succeeded())
	{
		Glass->SetStaticMesh(SM_PLANE.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterial>
		MT_GLASS(TEXT("/Game/Fracture/M_Glass.M_Glass"));
	if (MT_GLASS.Succeeded())
	{
		Glass->SetMaterial(0, MT_GLASS.Object);
	}

	Glass->OnComponentHit.AddDynamic(this, &AShatterableGlass::OnHit);

	RootComponent = Glass;
}

// Called when the game starts or when spawned
void AShatterableGlass::BeginPlay()
{
	Super::BeginPlay();
	
	Glass->GetLocalBounds(LocalMinBound, LocalMaxBound);
	FVector Scale = Glass->GetComponentScale();
	LocalMinBound *= Scale;
	LocalMaxBound *= Scale;

	UE_LOG(LogTemp, Warning, TEXT("Min Bounds: %s, Max Bounds: %s"), *LocalMinBound.ToString(), *LocalMaxBound.ToString());

	CreateGridPolygons(4, 4);
	VisualizePieces(GridPolygons, true, 5.0f);
}

void AShatterableGlass::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor && (OtherActor != this) && OtherComp)
	{
		FVector WorldHitLocation = Hit.ImpactPoint;
		FVector LocalHitPosition = Glass->GetComponentTransform().InverseTransformPosition(WorldHitLocation);
		UE_LOG(LogTemp, Warning, TEXT("Hit Point in Glass Local Space: %s"), *LocalHitPosition.ToString());
		UE_LOG(LogTemp, Warning, TEXT("Hit Point in World Space: %s"), *WorldHitLocation.ToString());
		UE_LOG(LogTemp, Warning, TEXT("Actor Location: %s"), *GetActorLocation().ToString());

		DrawDebugSphere(GetWorld(), WorldHitLocation, 8.0f, 12, FColor::White, false, 5.0f);

		CreateFracturePattern(WorldHitLocation);
		VisualizePieces(PatternCells, false, 5.0f);
	}
}

void AShatterableGlass::CreateFracturePattern(const FVector& ImpactPosition)
{
	PatternCells.Empty();

	FVector Extent = LocalMaxBound - LocalMinBound;
	FVector Center = ImpactPosition - GetActorLocation();	// same as (LocalHitPosition * Scale)

	// Define vertices
	FVector TopLeft = Center + FVector(-Extent.X, 0.0f, Extent.Z);
	FVector TopRight = Center + FVector(Extent.X, 0.0f, Extent.Z);
	FVector BottomLeft = Center + FVector(-Extent.X, 0.0f, -Extent.Z);
	FVector BottomRight = Center + FVector(Extent.X, 0.0f, -Extent.Z);

	// Add pieces
	PatternCells.Add(Piece(
		{ Edge(Vertex(TopLeft.X, TopLeft.Z), Vertex(Center.X, Center.Z)),
		  Edge(Vertex(Center.X, Center.Z), Vertex(TopRight.X, TopRight.Z)),
		  Edge(Vertex(TopRight.X, TopRight.Z), Vertex(TopLeft.X, TopLeft.Z)) },
		{ Vertex(TopLeft.X, TopLeft.Z), Vertex(TopRight.X, TopRight.Z), Vertex(Center.X, Center.Z) }));

	PatternCells.Add(Piece(
		{ Edge(Vertex(TopRight.X, TopRight.Z), Vertex(Center.X, Center.Z)),
		  Edge(Vertex(Center.X, Center.Z), Vertex(BottomRight.X, BottomRight.Z)),
		  Edge(Vertex(BottomRight.X, BottomRight.Z), Vertex(TopRight.X, TopRight.Z)) },
		{ Vertex(TopRight.X, TopRight.Z), Vertex(BottomRight.X, BottomRight.Z), Vertex(Center.X, Center.Z) }));

	PatternCells.Add(Piece(
		{ Edge(Vertex(BottomRight.X, BottomRight.Z), Vertex(Center.X, Center.Z)),
		  Edge(Vertex(Center.X, Center.Z), Vertex(BottomLeft.X, BottomLeft.Z)),
		  Edge(Vertex(BottomLeft.X, BottomLeft.Z), Vertex(BottomRight.X, BottomRight.Z)) },
		{ Vertex(BottomRight.X, BottomRight.Z), Vertex(BottomLeft.X, BottomLeft.Z), Vertex(Center.X, Center.Z) }));

	PatternCells.Add(Piece(
		{ Edge(Vertex(BottomLeft.X, BottomLeft.Z), Vertex(Center.X, Center.Z)),
		  Edge(Vertex(Center.X, Center.Z), Vertex(TopLeft.X, TopLeft.Z)),
		  Edge(Vertex(TopLeft.X, TopLeft.Z), Vertex(BottomLeft.X, BottomLeft.Z)) },
		{ Vertex(BottomLeft.X, BottomLeft.Z), Vertex(TopLeft.X, TopLeft.Z), Vertex(Center.X, Center.Z) }));
}

void AShatterableGlass::CreateGridPolygons(int32 rows, int32 cols)
{
	GridPolygons.Empty();

	// Calculate cell size
	float cellWidth = (LocalMaxBound.X - LocalMinBound.X) / cols;
	float cellHeight = (LocalMaxBound.Z - LocalMinBound.Z) / rows;

	for (int32 row = 0; row < rows; ++row)
	{
		for (int32 col = 0; col < cols; ++col)
		{
			// Define vertices for each cell
			Vertex TopLeft(LocalMinBound.X + col * cellWidth, LocalMinBound.Z + (row + 1) * cellHeight);
			Vertex TopRight(LocalMinBound.X + (col + 1) * cellWidth, LocalMinBound.Z + (row + 1) * cellHeight);
			Vertex BottomLeft(LocalMinBound.X + col * cellWidth, LocalMinBound.Z + row * cellHeight);
			Vertex BottomRight(LocalMinBound.X + (col + 1) * cellWidth, LocalMinBound.Z + row * cellHeight);

			// Create edges and vertices for the piece
			TArray<Vertex> cellVertices = { TopLeft, TopRight, BottomRight, BottomLeft };
			TArray<Edge> cellEdges;
			for (int32 i = 0; i < cellVertices.Num(); ++i)
			{
				cellEdges.Add(Edge(cellVertices[i], cellVertices[(i + 1) % cellVertices.Num()]));
			}

			// Add the piece to the array
			GridPolygons.Add(Piece(cellEdges, cellVertices));
		}
	}
}

void AShatterableGlass::VisualizePieces(const TArray<Piece>& Pieces, bool bRandomizeColor, float Duration)
{
	FVector ActorLocation = GetActorLocation();
	FColor LineColor = FColor::Red;

	for (const Piece& Piece : Pieces)
	{
		if (bRandomizeColor) 
		{
			LineColor = FLinearColor::MakeRandomColor().ToFColor(true);
		}
		for (const Edge& Edge : Piece.edges)
		{
			FVector Start = ActorLocation + FVector(Edge.v0.x, 0.0f, Edge.v0.z);
			FVector End = ActorLocation + FVector(Edge.v1.x, 0.0f, Edge.v1.z);
			DrawDebugLine(GetWorld(), Start, End, LineColor, false, Duration, 0, 2.0f);
		}
	}
}
