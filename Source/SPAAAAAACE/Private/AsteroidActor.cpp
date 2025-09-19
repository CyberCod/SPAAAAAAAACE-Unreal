/**
 * AsteroidActor Implementation
 * 
 * This file contains the implementation of the procedural asteroid generation system.
 * It creates realistic 3D asteroid meshes with physics simulation using icosphere
 * subdivision and multi-layer noise deformation.
 * 
 * Key Systems:
 * - Icosphere mesh generation and subdivision
 * - Multi-layer noise deformation
 * - Physics collision and simulation setup
 * - Statistics calculation (radius, volume, mass)
 * - Blueprint event integration
 */

#include "AsteroidActor.h"

// Core engine includes
#include "Kismet/KismetMathLibrary.h"  // Math utilities
#include "Engine/World.h"              // World access
#include "DrawDebugHelpers.h"          // Debug drawing

/**
 * AAsteroidActor Constructor
 * 
 * Initializes the asteroid actor with default settings and creates
 * the procedural mesh component. Sets up physics collision and
 * creates default noise layers if none are provided.
 */
AAsteroidActor::AAsteroidActor()
{
    // Disable ticking - asteroids are static after generation
    PrimaryActorTick.bCanEverTick = false;

    // ============================================================================
    // PROCEDURAL MESH COMPONENT SETUP
    // ============================================================================
    
    /**
     * Create Procedural Mesh Component
     * 
     * Creates the procedural mesh component that will hold the generated
     * asteroid geometry. This component handles both rendering and physics.
     */
    ProcMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProcMesh"));
    RootComponent = ProcMesh;

    // ============================================================================
    // PHYSICS COLLISION SETUP
    // ============================================================================
    
    /**
     * Configure Physics Collision
     * 
     * Sets up the physics collision for the asteroid:
     * - bUseComplexAsSimpleCollision = false: Use convex collision for simulation
     *   (Chaos physics does not simulate ComplexAsSimple collision)
     * - QueryAndPhysics: Enable both collision detection and physics simulation
     * - ECC_WorldDynamic: Set collision object type for world objects
     * - Movable: Allow the asteroid to move and rotate
     */
    ProcMesh->bUseComplexAsSimpleCollision = false;
    ProcMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    ProcMesh->SetCollisionObjectType(ECC_WorldDynamic);
    ProcMesh->SetMobility(EComponentMobility::Movable);

    // ============================================================================
    // DEFAULT NOISE LAYER SETUP
    // ============================================================================
    
    /**
     * Create Default Noise Layer
     * 
     * If no noise layers are provided, create a default one to ensure
     * the asteroid has some surface variation. This prevents completely
     * smooth spheres from being generated.
     */
    if (NoiseLayers.Num() == 0)
    {
        FNoiseLayer Layer;
        Layer.Scale = 0.1f;      // Medium-scale surface features
        Layer.Intensity = 1.0f;  // Moderate surface variation
        Layer.Seed = -1;         // Use random seed
        NoiseLayers.Add(Layer);
    }
}

void AAsteroidActor::BeginPlay()
{
    Super::BeginPlay();
    GenerateAsteroid();
}

void AAsteroidActor::GenerateAsteroid()
{
    // Pick global seed
    int32 UsedGlobalSeed = GlobalSeed;
    if (UsedGlobalSeed < 0)
    {
        UsedGlobalSeed = FMath::Rand();
    }

    // Prepare per-layer seeds
    TArray<int32> LayerSeeds;
    LayerSeeds.Reserve(NoiseLayers.Num());
    FRandomStream GlobalRand(UsedGlobalSeed);
    for (int32 i = 0; i < NoiseLayers.Num(); ++i)
    {
        int32 seed = NoiseLayers[i].Seed;
        if (seed < 0)
        {
            // create deterministic but distinct per-layer seed
            seed = GlobalRand.RandRange(0, INT32_MAX);
        }
        LayerSeeds.Add(seed);
    }

    // Build normalized base icosphere
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    BuildBaseIcosphere(Vertices, Triangles, Subdivisions);

    // Apply noise layers with independent seeds and clamp
    ApplyNoiseLayers(Vertices, LayerSeeds, MaxDisplacementFraction);

    // Normalize base mesh again to ensure radius==1 before scaling -> ensures volume calc correctness
    NormalizeVertices(Vertices);

    // Choose radius
    float ChosenRadius = FMath::RandRange(MinRadius, MaxRadius);

    // Scale to radius
    for (FVector& V : Vertices)
    {
        V *= ChosenRadius;
    }

    // Create mesh section without generating tri-mesh collision
    CreateMeshFromData(Vertices, Triangles, false);

    // Build convex collision from the generated vertices
    ProcMesh->ClearCollisionConvexMeshes();
    ProcMesh->AddCollisionConvexMesh(Vertices);
    ProcMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    // Calculate stats and set mass on procedural mesh
    CalculateStats(ChosenRadius);
    AsteroidStats.NoiseLayerSeeds = LayerSeeds; // record seeds for reproducibility

    // If physics enabled, configure mass; procedural mesh may need SetSimulatePhysics on attached primitive in some setups
    if (bEnablePhysics)
    {
        ProcMesh->SetSimulatePhysics(true);
        ProcMesh->SetMassOverrideInKg(NAME_None, AsteroidStats.Mass, true);
    }

    // Broadcast event
    OnAsteroidGenerated.Broadcast(AsteroidStats);

    // Log
    UE_LOG(LogTemp, Log, TEXT("Asteroid Generated: Radius=%.2f, Volume=%.6g m^3, Mass=%.6g kg"),
        AsteroidStats.Radius, AsteroidStats.Volume, AsteroidStats.Mass);
}

// ------------------------- Geometry generation -------------------------
void AAsteroidActor::BuildBaseIcosphere(TArray<FVector>& Vertices, TArray<int32>& Triangles, int32 SubdivisionsLevel)
{
    Vertices.Empty();
    Triangles.Empty();

    // create icosahedron
    const float t = (1.0f + FMath::Sqrt(5.0f)) / 2.0f;

    Vertices.Add(FVector(-1,  t,  0));
    Vertices.Add(FVector( 1,  t,  0));
    Vertices.Add(FVector(-1, -t,  0));
    Vertices.Add(FVector( 1, -t,  0));
    Vertices.Add(FVector( 0, -1,  t));
    Vertices.Add(FVector( 0,  1,  t));
    Vertices.Add(FVector( 0, -1, -t));
    Vertices.Add(FVector( 0,  1, -t));
    Vertices.Add(FVector( t,  0, -1));
    Vertices.Add(FVector( t,  0,  1));
    Vertices.Add(FVector(-t,  0, -1));
    Vertices.Add(FVector(-t,  0,  1));

    // faces
    int32 faceIndices[] = {
        0,11,5, 0,5,1, 0,1,7, 0,7,10, 0,10,11,
        1,5,9, 5,11,4, 11,10,2, 10,7,6, 7,1,8,
        3,9,4, 3,4,2, 3,2,6, 3,6,8, 3,8,9,
        4,9,5, 2,4,11, 6,2,10, 8,6,7, 9,8,1
    };

    const int32 FaceCount = sizeof(faceIndices) / (3 * sizeof(int32));
    for (int32 i = 0; i < sizeof(faceIndices) / sizeof(int32); ++i)
    {
        Triangles.Add(faceIndices[i]);
    }

    // normalize
    NormalizeVertices(Vertices);

    // Subdivide
    for (int32 i = 0; i < SubdivisionsLevel; ++i)
    {
        SubdivideIcosphere(Vertices, Triangles);
    }

    // Final normalization
    NormalizeVertices(Vertices);
}

void AAsteroidActor::SubdivideIcosphere(TArray<FVector>& Vertices, TArray<int32>& Triangles)
{
    TMap<int64, int32> middlePointIndexCache;
    TArray<int32> newTriangles;
    newTriangles.Reserve(Triangles.Num() * 4);

    auto MakeKey = [](int32 a, int32 b) -> int64 {
        int32 small = FMath::Min(a, b);
        int32 large = FMath::Max(a, b);
        return ( (int64)small << 32 ) | (uint32)large;
    };

    for (int32 i = 0; i + 2 < Triangles.Num(); i += 3)
    {
        int32 v1 = Triangles[i];
        int32 v2 = Triangles[i+1];
        int32 v3 = Triangles[i+2];

        int32 a = GetMiddlePoint(v1, v2, Vertices, middlePointIndexCache);
        int32 b = GetMiddlePoint(v2, v3, Vertices, middlePointIndexCache);
        int32 c = GetMiddlePoint(v3, v1, Vertices, middlePointIndexCache);

        newTriangles.Add(v1); newTriangles.Add(a); newTriangles.Add(c);
        newTriangles.Add(v2); newTriangles.Add(b); newTriangles.Add(a);
        newTriangles.Add(v3); newTriangles.Add(c); newTriangles.Add(b);
        newTriangles.Add(a);  newTriangles.Add(b); newTriangles.Add(c);
    }

    Triangles = MoveTemp(newTriangles);
}

// Return index of middle point and create if needed
int32 AAsteroidActor::GetMiddlePoint(int32 p1, int32 p2, TArray<FVector>& Vertices, TMap<int64, int32>& Cache)
{
    int32 small = FMath::Min(p1, p2);
    int32 large = FMath::Max(p1, p2);
    int64 key = ((int64)small << 32) | (uint32)large;

    int32* Found = Cache.Find(key);
    if (Found)
    {
        return *Found;
    }

    FVector point1 = Vertices[p1];
    FVector point2 = Vertices[p2];
    FVector middle = (point1 + point2) * 0.5f;
    middle.Normalize();

    int32 i = Vertices.Add(middle);
    Cache.Add(key, i);
    return i;
}

void AAsteroidActor::NormalizeVertices(TArray<FVector>& Vertices)
{
    for (FVector& V : Vertices)
    {
        V.Normalize();
    }
}

// ------------------------- Noise / Deformation -------------------------
void AAsteroidActor::ApplyNoiseLayers(TArray<FVector>& Vertices, const TArray<int32>& LayerSeeds, float MaxDisplacementFrac)
{
    if (Vertices.Num() == 0) return;

    // For each layer, apply per-vertex displacement
    for (int32 layerIndex = 0; layerIndex < NoiseLayers.Num(); ++layerIndex)
    {
        const FNoiseLayer& Layer = NoiseLayers[layerIndex];
        int32 seed = LayerSeeds.IsValidIndex(layerIndex) ? LayerSeeds[layerIndex] : FMath::Rand();
        FRandomStream layerRand(seed);

        // Prepare offsets to vary noise per-vertex
        float ox = layerRand.FRand() * 1000.0f;
        float oy = layerRand.FRand() * 1000.0f;
        float oz = layerRand.FRand() * 1000.0f;

        // Max absolute displacement in unit-sphere space
        float maxDisplacement = MaxDisplacementFrac;

        for (int32 i = 0; i < Vertices.Num(); ++i)
        {
            FVector& V = Vertices[i];

            // sample Perlin noise in 3D by sampling FMath::PerlinNoise3D which expects FVector
            FVector samplePoint = V * Layer.Scale + FVector(ox, oy, oz);
            float nx = FMath::PerlinNoise3D(samplePoint + FVector(0.0f, 0.0f, 0.0f));
            float ny = FMath::PerlinNoise3D(samplePoint + FVector(13.13f, 37.37f, 7.73f)); // arbitrary offsets for axis variation
            float nz = FMath::PerlinNoise3D(samplePoint + FVector(97.97f, 21.21f, 55.55f));

            FVector offset = FVector(nx, ny, nz) * Layer.Intensity * 0.5f; // scale it down a bit

            // Apply displacement along normal direction to preserve sphere-like behavior
            FVector normal = V;
            normal.Normalize();

            float displacement = FVector::DotProduct(offset, normal); // scalar displacement along normal
            displacement = FMath::Clamp(displacement, -maxDisplacement, maxDisplacement);

            V = V + normal * displacement;
        }
    }
}

// ------------------------- Mesh creation -------------------------
void AAsteroidActor::CreateMeshFromData(const TArray<FVector>& Vertices, const TArray<int32>& Triangles, bool bCreateCollision)
{
    // Compute normals (simple approach)
    TArray<FVector> Normals;
    Normals.SetNumZeroed(Vertices.Num());

    // Zeroed tangents/uvs/colors for simplicity
    TArray<FVector2D> UVs;
    UVs.SetNumZeroed(Vertices.Num());
    TArray<FProcMeshTangent> Tangents;
    Tangents.SetNumZeroed(Vertices.Num());
    TArray<FColor> Colors;
    Colors.SetNumZeroed(Vertices.Num());

    // Compute face normals and accumulate
    for (int32 i = 0; i + 2 < Triangles.Num(); i += 3)
    {
        int32 i0 = Triangles[i];
        int32 i1 = Triangles[i+1];
        int32 i2 = Triangles[i+2];

        if (!Vertices.IsValidIndex(i0) || !Vertices.IsValidIndex(i1) || !Vertices.IsValidIndex(i2))
        {
            continue;
        }

        FVector v0 = Vertices[i0];
        FVector v1 = Vertices[i1];
        FVector v2 = Vertices[i2];

        FVector faceNormal = FVector::CrossProduct(v1 - v0, v2 - v0).GetSafeNormal();
        Normals[i0] += faceNormal;
        Normals[i1] += faceNormal;
        Normals[i2] += faceNormal;
    }

    for (FVector& N : Normals)
    {
        N.Normalize();
    }

    // Create mesh section (section 0)
    ProcMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, bCreateCollision);

    if (bCreateCollision)
    {
        ProcMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        ProcMesh->SetCollisionObjectType(ECC_WorldDynamic);
    }
}

// ------------------------- Stats -------------------------
void AAsteroidActor::CalculateStats(float ChosenRadius)
{
    // radius in meters (user-provided units assumed m; adjust if your unit scale differs)
    AsteroidStats.Radius = ChosenRadius;

    AsteroidStats.Volume = CalculateVolumeFromRadius(ChosenRadius);
    AsteroidStats.Mass = AsteroidStats.Volume * (double)Density;

    // Seeds are filled by caller after generation
    AsteroidStats.NoiseLayerSeeds.Empty();
}

double AAsteroidActor::CalculateVolumeFromRadius(double Radius) const
{
    // Volume of a sphere
    return (4.0 / 3.0) * PI * Radius * Radius * Radius;
}
