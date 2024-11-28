// Fill out your copyright notice in the Description page of Project Settings.


#include "ShatterableGlass.h"
#include "PolygonClipper.h"

// Sets default values
AShatterableGlass::AShatterableGlass()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Glass = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GLASS"));
	Glass->SetupAttachment(Root);
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

	// Test ProcMesh for Partial Fracture
	ProcMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	ProcMesh->SetupAttachment(Root);

	ProcMesh->SetCollisionProfileName(TEXT("BlockAll"));
	ProcMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	ProcMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ProcMesh->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);

	ProcMesh->bUseComplexAsSimpleCollision = false;
	ProcMesh->bAlwaysCreatePhysicsState = true;

	ProcMesh->SetSimulatePhysics(true);
	ProcMesh->SetEnableGravity(true);
	ProcMesh->SetMassOverrideInKg(NAME_None, 50.0f);
	ProcMesh->RecreatePhysicsState();

	ProcMesh->OnComponentHit.AddDynamic(this, &AShatterableGlass::OnHit);
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
	VisualizePieces(GridPolygons, false, 5.0f);
}

void AShatterableGlass::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor && (OtherActor != this) && OtherComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cube hit by %s at location %s"), *OtherActor->GetName(), *Hit.ImpactPoint.ToString());

		if (HitComp == Glass)
		{
			UE_LOG(LogTemp, Warning, TEXT("Hit! > Glass component"));

			FVector WorldHitLocation = Hit.ImpactPoint;
			FVector LocalHitPosition = Glass->GetComponentTransform().InverseTransformPosition(WorldHitLocation);
			UE_LOG(LogTemp, Warning, TEXT("Hit Point in Glass Local Space: %s"), *LocalHitPosition.ToString());
			UE_LOG(LogTemp, Warning, TEXT("Hit Point in World Space: %s"), *WorldHitLocation.ToString());
			UE_LOG(LogTemp, Warning, TEXT("Actor Location: %s"), *GetActorLocation().ToString());

			DrawDebugSphere(GetWorld(), WorldHitLocation, 8.0f, 12, FColor::White, false, 5.0f);

			CreateFracturePattern(WorldHitLocation);
			VisualizePieces(PatternCells, false, 10.0f);

			TArray<Piece> ClippedPieces;
			TMap<int32, TArray<int32>> CellToPiecesMap;

			int32 PieceIndex = 0;
			for (int32 i = 0; i < GridPolygons.Num(); ++i) {
				const Piece& Subject = GridPolygons[i];

				for (int32 j = 0; j < PatternCells.Num(); ++j) {
					const Piece& Clip = PatternCells[j];

					TArray<Point> ClippedPoints = PolygonClipper::PerformClipping(Subject.points, Clip.points);

					if (ClippedPoints.Num() > 0) {
						ClippedPieces.Add(Piece(ClippedPoints));
						UE_LOG(LogTemp, Warning, TEXT("Piece %d generated clipped piece %d"), j, PieceIndex);

						if (!CellToPiecesMap.Contains(j)) {
							CellToPiecesMap.Add(j, TArray<int32>());
						}
						CellToPiecesMap[j].Add(PieceIndex);
						PieceIndex++;
					}
				}
			}
			UE_LOG(LogTemp, Warning, TEXT("number of clipped pieces: %d"), ClippedPieces.Num());
			VisualizePieces(ClippedPieces, true, 20.0f);

			GenerateCube();
		}
		else if (HitComp == ProcMesh)
		{
			UE_LOG(LogTemp, Warning, TEXT("Hit! > ProceduralMesh component"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Hit! > Other component"));
		}
	}
}

void AShatterableGlass::GenerateCube()
{
	if (Glass)
	{
		Glass->OnComponentHit.RemoveDynamic(this, &AShatterableGlass::OnHit);
		Glass->DestroyComponent();
		Glass = nullptr;
	}

	float Size = 100.0f;

	TArray<FVector> Vertices1 = {
		FVector(0, 0, 0),
		FVector(0, Size, 0),
		FVector(Size, Size, 0),
		FVector(Size, 0, 0),
		FVector(0, 0, Size),
		FVector(0, Size, Size),
		FVector(Size, Size, Size),
		FVector(Size, 0, Size)
	};

	TArray<FVector> Vertices2;
	for (const FVector& Vertex : Vertices1)
	{
		Vertices2.Add(Vertex + FVector(0, Size, 0));
	}

	TArray<int32> Triangles = {
		0, 2, 1, 0, 3, 2,	// Front
		4, 5, 6, 4, 6, 7,	// Back
		0, 1, 5, 0, 5, 4,	// Left
		2, 3, 7, 2, 7, 6,	// Right
		1, 2, 6, 1, 6, 5,	// Top
		0, 4, 7, 0, 7, 3	// Bottom
	};

	ProcMesh->AddCollisionConvexMesh(Vertices1);
	ProcMesh->AddCollisionConvexMesh(Vertices2);

	ProcMesh->CreateMeshSection(0, Vertices1, Triangles, TArray<FVector>(), TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>(), true);
	ProcMesh->CreateMeshSection(1, Vertices2, Triangles, TArray<FVector>(), TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>(), true);

	//ProcMesh->RecreatePhysicsState();
	//ProcMesh->ContainsPhysicsTriMeshData(true);
	//ProcMesh->UpdateCollision();
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
		{ Edge(Point(TopLeft.X, TopLeft.Z), Point(Center.X, Center.Z)),
		  Edge(Point(Center.X, Center.Z), Point(TopRight.X, TopRight.Z)),
		  Edge(Point(TopRight.X, TopRight.Z), Point(TopLeft.X, TopLeft.Z)) },
		{ Point(TopLeft.X, TopLeft.Z), Point(TopRight.X, TopRight.Z), Point(Center.X, Center.Z) }));

	PatternCells.Add(Piece(
		{ Edge(Point(TopRight.X, TopRight.Z), Point(Center.X, Center.Z)),
		  Edge(Point(Center.X, Center.Z), Point(BottomRight.X, BottomRight.Z)),
		  Edge(Point(BottomRight.X, BottomRight.Z), Point(TopRight.X, TopRight.Z)) },
		{ Point(TopRight.X, TopRight.Z), Point(BottomRight.X, BottomRight.Z), Point(Center.X, Center.Z) }));

	PatternCells.Add(Piece(
		{ Edge(Point(BottomRight.X, BottomRight.Z), Point(Center.X, Center.Z)),
		  Edge(Point(Center.X, Center.Z), Point(BottomLeft.X, BottomLeft.Z)),
		  Edge(Point(BottomLeft.X, BottomLeft.Z), Point(BottomRight.X, BottomRight.Z)) },
		{ Point(BottomRight.X, BottomRight.Z), Point(BottomLeft.X, BottomLeft.Z), Point(Center.X, Center.Z) }));

	PatternCells.Add(Piece(
		{ Edge(Point(BottomLeft.X, BottomLeft.Z), Point(Center.X, Center.Z)),
		  Edge(Point(Center.X, Center.Z), Point(TopLeft.X, TopLeft.Z)),
		  Edge(Point(TopLeft.X, TopLeft.Z), Point(BottomLeft.X, BottomLeft.Z)) },
		{ Point(BottomLeft.X, BottomLeft.Z), Point(TopLeft.X, TopLeft.Z), Point(Center.X, Center.Z) }));
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
			Point TopLeft(LocalMinBound.X + col * cellWidth, LocalMinBound.Z + (row + 1) * cellHeight);
			Point TopRight(LocalMinBound.X + (col + 1) * cellWidth, LocalMinBound.Z + (row + 1) * cellHeight);
			Point BottomLeft(LocalMinBound.X + col * cellWidth, LocalMinBound.Z + row * cellHeight);
			Point BottomRight(LocalMinBound.X + (col + 1) * cellWidth, LocalMinBound.Z + row * cellHeight);

			// Create edges and vertices for the piece
			TArray<Point> cellVertices = { TopLeft, TopRight, BottomRight, BottomLeft };
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

void AShatterableGlass::GeneratePieceMeshes(const TArray<Piece>& Pieces, const TMap<int32, TArray<int32>>& CellToPiecesMap)
{
	UMaterialInterface* GlassMaterial = nullptr;
	if (Glass)
	{
		GlassMaterial = Glass->GetMaterial(0);
		Glass->DestroyComponent();
		Glass = nullptr;
	}

	for (const auto& Pair : CellToPiecesMap)
	{
		int32 CellIndex = Pair.Key;
		const TArray<int32>& PieceIndices = Pair.Value;

		// Dynamically create a procedural mesh component for each cell piece
		FString PieceName = FString::Printf(TEXT("CellPiece_%d"), CellIndex);
		UProceduralMeshComponent* PieceMesh = NewObject<UProceduralMeshComponent>(this, *PieceName);
		PieceMesh->RegisterComponent();
		PieceMesh->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);

		// Set collision profile
		PieceMesh->SetCollisionProfileName(TEXT("BlockAll"));
		PieceMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
		PieceMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		PieceMesh->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);

		// Configure collision settings
		PieceMesh->bUseComplexAsSimpleCollision = false;
		PieceMesh->SetSimulatePhysics(true);
		PieceMesh->bAlwaysCreatePhysicsState = true;

		int32 SectionIndex = 0;
		for (const int32 PieceIndex : PieceIndices)
		{
			const Piece& Piece = Pieces[PieceIndex];

			TArray<int32> TriangleIndices;
			TArray<FVector> MeshVertices;
			FanTriangulation(Piece.points, TriangleIndices, MeshVertices);

			PieceMesh->CreateMeshSection(
				SectionIndex,                  // Section index
				MeshVertices,                  // Vertex data for the mesh
				TriangleIndices,               // Triangle faces
				TArray<FVector>(),             // Empty normals array
				TArray<FVector2D>(),           // Empty UVs array
				TArray<FColor>(),              // Empty vertex colors array
				TArray<FProcMeshTangent>(),    // Empty tangents array
				true                           // Enable collision
			);

			PieceMesh->AddCollisionConvexMesh(MeshVertices);
			if (GlassMaterial) {
				PieceMesh->SetMaterial(SectionIndex, GlassMaterial);
			}

			SectionIndex++;
		}

		// Apply an impulse in a randomly varied direction based on the Y-axis.
		FVector ImpactDirection = FVector(0.0f, 1.0f, 0.0f) + FMath::VRand() * 0.2f;
		ImpactDirection = ImpactDirection.GetSafeNormal();
		float ImpulseStrength = 500.0f;
		PieceMesh->AddImpulse(ImpactDirection * ImpulseStrength, NAME_None, true);
		PieceMesh->WakeRigidBody();
	}
}

void AShatterableGlass::FanTriangulation(const Piece& Piece, TArray<int32>& Triangles, TArray<FVector>& MeshVertices)
{
	const TArray<Point>& Points = Piece.points;

	// Front face vertices & triangles
	int32 FrontFaceOffset = MeshVertices.Num();
	for (const Point& point : Points)
	{
		MeshVertices.Add(FVector(point.x, 0.0f, point.z));
	}
	for (int32 i = 1; i < Points.Num() - 1; i++)
	{
		Triangles.Add(FrontFaceOffset);
		Triangles.Add(FrontFaceOffset + i);
		Triangles.Add(FrontFaceOffset + i + 1);
	}

	// Back face (reverse winding order)
	int32 BackFaceOffset = MeshVertices.Num();
	for (const Point& point : Points)
	{
		MeshVertices.Add(FVector(point.x, 0.0f, point.z));
	}
	for (int32 i = 1; i < Points.Num() - 1; i++)
	{
		Triangles.Add(BackFaceOffset);
		Triangles.Add(BackFaceOffset + i + 1);
		Triangles.Add(BackFaceOffset + i);
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
