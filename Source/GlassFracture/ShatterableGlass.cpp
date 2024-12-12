// Fill out your copyright notice in the Description page of Project Settings.


#include "ShatterableGlass.h"
#include "PolygonClipper.h"
#include "PatternCells/FracturePatternGenerator.h"
#include "VoronoiDiagram/VoronoiGenerator.h"
#include "Kismet/GameplayStatics.h"

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
		GlassMaterial = MT_GLASS.Object;
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

	/*CreateGridPolygons(4, 4);
	VisualizePieces(GridPolygons, false, 1.0f);
	IntactPieces = GridPolygons;*/

	TArray<Point> RandomPoints = GenerateRandomPoints(50.0f, 70, 60.0f);
	TArray<Piece> VoronoiPolygons = VoronoiGenerator::GenerateVoronoiCells(RandomPoints, LocalMinBound, LocalMaxBound);
	VisualizePieces(VoronoiPolygons, true, 1.0f);
	IntactPieces = VoronoiPolygons;
}

void AShatterableGlass::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor && (OtherActor != this) && OtherComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cube hit by %s at location %s"), *OtherActor->GetName(), *Hit.ImpactPoint.ToString());
		UE_LOG(LogTemp, Warning, TEXT("Hit! > %s component"), *HitComp->GetName());

		FVector WorldHitLocation = Hit.ImpactPoint;
		FVector LocalHitPosition = HitComp->GetComponentTransform().InverseTransformPosition(WorldHitLocation);
		UE_LOG(LogTemp, Warning, TEXT("Hit Point in Glass Local Space: %s"), *LocalHitPosition.ToString());
		UE_LOG(LogTemp, Warning, TEXT("Hit Point in World Space: %s"), *WorldHitLocation.ToString());
		UE_LOG(LogTemp, Warning, TEXT("Actor Location: %s"), *GetActorLocation().ToString());

		DrawDebugSphere(GetWorld(), WorldHitLocation, 8.0f, 12, FColor::White, false, 0.0f);
		float ImpactRadius = 80.0f;
		DrawImpactCircle(WorldHitLocation, ImpactRadius, 0.0f);

		//PatternCells = FracturePatternGenerator::CreateDiagonalPieces(WorldHitLocation, LocalMaxBound - LocalMinBound, GetActorLocation());
		if (HitComp == Glass)
			PatternCells = FracturePatternGenerator::CreateSpiderwebPieces(LocalHitPosition * 3.0f, GetActorLocation(), PolygonDataTable, VertexDataTable);
		else if (HitComp == ProcMesh)
			PatternCells = FracturePatternGenerator::CreateSpiderwebPieces(LocalHitPosition, GetActorLocation(), PolygonDataTable, VertexDataTable);
		VisualizePieces(PatternCells, false, 0.0f);

		TArray<Piece> ClippedPieces;
		TArray<Piece> OutsidePieces;
		TMap<int32, TArray<int32>> CellToPiecesMap;

		int32 PieceIndex = 0;
		for (int32 i = 0; i < IntactPieces.Num(); ++i) {
			const Piece& Subject = IntactPieces[i];

			FVector Scale = HitComp->GetComponentScale();
			Point Center((LocalHitPosition * Scale).X, (LocalHitPosition * Scale).Z);
			ECircleIntersectionType IntersectionResult = CheckPieceCircleIntersection(Subject, FVector(Center.x, 0.0f, Center.z), ImpactRadius);

			if (IntersectionResult == ECircleIntersectionType::Outside) {
				OutsidePieces.Add(Subject);
				continue;
			}

			for (int32 j = 0; j < PatternCells.Num(); ++j) {
				const Piece& Clip = PatternCells[j];

				TArray<Point> ClippedPoints = PolygonClipper::PerformClipping(Subject.points, Clip.points);

				if (ClippedPoints.Num() > 0) {
					Piece NewPiece(ClippedPoints);
					ECircleIntersectionType ClippedIntersectionResult = CheckPieceCircleIntersection(NewPiece, FVector(Center.x, 0.0f, Center.z), ImpactRadius);

					switch (ClippedIntersectionResult) {
					case ECircleIntersectionType::Inside:
					case ECircleIntersectionType::Overlapping:
						ClippedPieces.Add(NewPiece);
						UE_LOG(LogTemp, Log, TEXT("Piece %d generated clipped piece %d"), j, PieceIndex);

						if (!CellToPiecesMap.Contains(j)) {
							CellToPiecesMap.Add(j, TArray<int32>());
						}
						CellToPiecesMap[j].Add(PieceIndex);
						PieceIndex++;
						break;
					case ECircleIntersectionType::Outside:
						UE_LOG(LogTemp, Log, TEXT("Clipped Piece is completely outside the circle."));
						OutsidePieces.Add(NewPiece);
						break;
					}
				}
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("number of clipped pieces: %d"), ClippedPieces.Num());
		VisualizePieces(ClippedPieces, true, 0.0f);

		if (Glass)
		{
			Glass->OnComponentHit.RemoveDynamic(this, &AShatterableGlass::OnHit);
			Glass->DestroyComponent();
			Glass = nullptr;
		}
		GeneratePieceMeshes(ClippedPieces, CellToPiecesMap);
		GeneratePieceMeshes(OutsidePieces);
		if (ShatterSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, ShatterSound, Hit.ImpactPoint);
		}
		IntactPieces = MoveTemp(OutsidePieces);
	}
}

AShatterableGlass::ECircleIntersectionType 
AShatterableGlass::CheckPieceCircleIntersection(const Piece& Piece, const FVector& CircleCenter, float Radius)
{
	bool bAllInside = true;
	bool bAnyInside = false;

	for (const Point& point : Piece.points)
	{
		float distanceSquared = FMath::Pow(point.x - CircleCenter.X, 2) + FMath::Pow(point.z - CircleCenter.Z, 2);
		if (distanceSquared <= Radius * Radius)
		{
			bAnyInside = true;
		}
		else
		{
			bAllInside = false;
		}
	}

	if (bAllInside) 
	{
		return ECircleIntersectionType::Inside;
	}
	if (bAnyInside)
	{
		return ECircleIntersectionType::Overlapping;
	}
	return ECircleIntersectionType::Outside;
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

void AShatterableGlass::GeneratePieceMeshes(const TArray<Piece>& Pieces)
{
	ProcMesh->ClearAllMeshSections();

	int32 SectionIndex = 0;
	for (const Piece& Piece : Pieces)
	{
		TArray<int32> TriangleIndices;
		TArray<FVector> MeshVertices;

		FanTriangulation(Piece.points, TriangleIndices, MeshVertices);

		ProcMesh->AddCollisionConvexMesh(MeshVertices);
		if (GlassMaterial) {
			ProcMesh->SetMaterial(SectionIndex, GlassMaterial);
		}
		ProcMesh->CreateMeshSection(SectionIndex, MeshVertices, TriangleIndices, TArray<FVector>(), TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>(), true);
		SectionIndex++;
	}

	ProcMesh->RecreatePhysicsState();
	//ProcMesh->ContainsPhysicsTriMeshData(true);
	//ProcMesh->UpdateCollision();
}

void AShatterableGlass::GeneratePieceMeshes(const TArray<Piece>& Pieces, const TMap<int32, TArray<int32>>& CellToPiecesMap)
{
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
		float ImpulseStrength = 300.0f;
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

template <typename T>
void AShatterableGlass::VisualizePieces(const TArray<T>& Pieces, bool bRandomizeColor, float Duration)
{
	FVector ActorLocation = GetActorLocation();
	FColor LineColor = FColor::Red;

	for (const T& Piece : Pieces)
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

void AShatterableGlass::DrawImpactCircle(const FVector& ImpactPosition, float Radius, float Duration, const FColor& Color, float Thickness, int32 NumSegments)
{
	TArray<FVector> CirclePoints;

	FVector ActorLocation = GetActorLocation();
	FVector Center = ImpactPosition - ActorLocation;	// same as (LocalHitPosition * Scale)

	// Calculate points on circumference
	for (int32 i = 0; i < NumSegments; ++i)
	{
		float Angle = 2.0f * PI * i / NumSegments;
		float X = Center.X + Radius * FMath::Cos(Angle);
		float Z = Center.Z + Radius * FMath::Sin(Angle);
		CirclePoints.Add(FVector(X, 0.0f, Z));
	}

	// Draw sphere
	for (int32 i = 0; i < CirclePoints.Num(); ++i)
	{
		const FVector& Start = ActorLocation + CirclePoints[i];
		const FVector& End = ActorLocation + CirclePoints[(i + 1) % CirclePoints.Num()];
		DrawDebugLine(GetWorld(), Start, End, Color, false, Duration, 0, Thickness);
	}
}

TArray<Point> AShatterableGlass::GenerateRandomPoints(float MinDistance, int32 NumPoints, float EdgeOffset)
{
	TArray<Point> RandomPoints;
	TArray<FVector> PoissonPoints;

	const int32 MaxAttempts = 30;

	FVector AdjustedMin = LocalMinBound - FVector(EdgeOffset, 0.0f, EdgeOffset);
	FVector AdjustedMax = LocalMaxBound + FVector(EdgeOffset, 0.0f, EdgeOffset);

	for (int32 i = 0; i < NumPoints; ++i)
	{
		FVector CandidatePoint;
		bool IsValid = false;

		for (int32 attempt = 0; attempt < MaxAttempts; ++attempt)
		{
			CandidatePoint = FVector(
				FMath::RandRange(AdjustedMin.X, AdjustedMax.X),
				0.0f,
				FMath::RandRange(AdjustedMin.Z, AdjustedMax.Z)
			);
			IsValid = true;
			for (const FVector& ExistingPoint : PoissonPoints)
			{
				if (FVector::DistSquared(CandidatePoint, ExistingPoint) < FMath::Square(MinDistance))
				{
					IsValid = false;
					break;
				}
			}
			if (IsValid) break;
		}
		if (IsValid)
		{
			PoissonPoints.Add(CandidatePoint);
			RandomPoints.Add(Point(CandidatePoint.X, CandidatePoint.Z));
			DrawDebugSphere(GetWorld(), CandidatePoint + GetActorLocation(), 4.0f, 12, FColor::Orange, false, 0.0f);
		}
	}

	return RandomPoints;
}