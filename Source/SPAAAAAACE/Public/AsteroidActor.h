/**
 * AsteroidActor - Procedural Asteroid Generation System
 * 
 * This file defines the procedural asteroid generation system that creates
 * realistic 3D asteroid meshes with physics simulation.
 * 
 * Key Features:
 * - Procedural mesh generation using icosphere subdivision
 * - Multi-layer noise deformation for realistic asteroid shapes
 * - Physics simulation with calculated mass and collision
 * - Configurable size, density, and noise parameters
 * - Blueprint integration for game events
 * 
 * The system generates asteroids by starting with a base icosphere,
 * applying multiple layers of noise deformation, and creating physics
 * collision based on the final mesh geometry.
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "AsteroidActor.generated.h"

/**
 * FAsteroidStats - Asteroid Statistics Structure
 * 
 * Contains calculated statistics for a generated asteroid.
 * These values are computed during generation and stored for
 * physics simulation and gameplay purposes.
 */
USTRUCT(BlueprintType)
struct FAsteroidStats
{
    GENERATED_BODY()

    /**
     * Radius - Asteroid Radius
     * 
     * The average radius of the asteroid in centimeters.
     * This is calculated from the generated mesh vertices.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float Radius = 0.0f;

    /**
     * Volume - Asteroid Volume
     * 
     * The calculated volume of the asteroid in cubic centimeters.
     * This is used for mass calculations and physics simulation.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    double Volume = 0.0;

    /**
     * Mass - Asteroid Mass
     * 
     * The calculated mass of the asteroid in kilograms.
     * This is computed from volume and density for physics simulation.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    double Mass = 0.0;

    /**
     * NoiseLayerSeeds - Noise Layer Seeds
     * 
     * The random seeds used for each noise layer during generation.
     * These are stored for reproducibility - the same seeds will
     * generate the same asteroid shape.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<int32> NoiseLayerSeeds;
};

/**
 * FNoiseLayer - Noise Layer Configuration
 * 
 * Defines a single layer of noise deformation for asteroid generation.
 * Multiple layers can be combined to create complex, realistic asteroid shapes.
 */
USTRUCT(BlueprintType)
struct FNoiseLayer
{
    GENERATED_BODY()

    /**
     * Scale - Noise Scale
     * 
     * Controls the frequency/wavelength of the noise.
     * Higher values create smaller, more detailed features.
     * Lower values create larger, smoother features.
     * 
     * Default: 0.1 (creates medium-scale surface features)
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise")
    float Scale = 0.1f;

    /**
     * Intensity - Noise Intensity
     * 
     * Controls how much the noise affects the asteroid shape.
     * Higher values create more dramatic surface variations.
     * Lower values create subtle surface details.
     * 
     * Default: 1.0 (moderate surface variation)
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise")
    float Intensity = 1.0f;

    /**
     * Seed - Layer-Specific Seed
     * 
     * Random seed for this noise layer. This allows for reproducible
     * asteroid generation while maintaining variation between layers.
     * 
     * -1 = Use global seed + random offset
     * Any other value = Use this specific seed
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise")
    int32 Seed = -1;
};

/**
 * FOnAsteroidGenerated - Asteroid Generation Event Delegate
 * 
 * Multicast delegate that is broadcast when an asteroid is successfully generated.
 * This allows other systems to respond to asteroid creation events.
 * 
 * @param Stats - The calculated statistics of the generated asteroid
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAsteroidGenerated, const FAsteroidStats&, Stats);

/**
 * AAsteroidActor - Procedural Asteroid Actor
 * 
 * This actor generates procedural 3D asteroid meshes with physics simulation.
 * It creates realistic asteroid shapes using icosphere subdivision and
 * multi-layer noise deformation.
 * 
 * Generation Process:
 * 1. Create base icosphere mesh
 * 2. Subdivide for detail
 * 3. Apply noise layers for shape variation
 * 4. Scale to desired size
 * 5. Calculate physics properties
 * 6. Create collision mesh
 */
UCLASS()
class SPAAAAAACE_API AAsteroidActor : public AActor
{
    GENERATED_BODY()

public:
    /**
     * Constructor
     * 
     * Initializes the asteroid actor with default settings and creates
     * the procedural mesh component.
     */
    AAsteroidActor();

protected:
    /**
     * BeginPlay - Asteroid Initialization
     * 
     * Called when the asteroid spawns. Triggers the procedural generation
     * process to create the asteroid mesh and physics.
     */
    virtual void BeginPlay() override;

public:
    // ============================================================================
    // COMPONENTS
    // ============================================================================
    
    /**
     * ProcMesh - Procedural Mesh Component
     * 
     * The procedural mesh component that holds the generated asteroid geometry.
     * This component handles both rendering and physics collision for the asteroid.
     * 
     * The mesh is generated procedurally using triangle collision for accurate
     * physics simulation of the irregular asteroid shape.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UProceduralMeshComponent* ProcMesh;

    // ============================================================================
    // ASTEROID GENERATION CONFIGURATION
    // ============================================================================
    
    /**
     * Subdivisions - Icosphere Subdivision Level
     * 
     * Controls the level of detail in the base icosphere mesh.
     * Higher values create more detailed meshes but increase generation time.
     * 
     * Default: 2 (good balance of detail and performance)
     * Typical range: 1-4 (1=low detail, 4=high detail)
     */
    UPROPERTY(EditAnywhere, Category = "Asteroid Generation")
    int32 Subdivisions = 2;

    /**
     * MinRadius - Minimum Asteroid Radius
     * 
     * The minimum radius for generated asteroids in centimeters.
     * This is the lower bound for random size selection.
     * 
     * Default: 250.0cm (2.5 meters)
     */
    UPROPERTY(EditAnywhere, Category = "Asteroid Generation")
    float MinRadius = 250.0f;

    /**
     * MaxRadius - Maximum Asteroid Radius
     * 
     * The maximum radius for generated asteroids in centimeters.
     * This is the upper bound for random size selection.
     * 
     * Default: 1000.0cm (10 meters)
     */
    UPROPERTY(EditAnywhere, Category = "Asteroid Generation")
    float MaxRadius = 1000.0f;

    /**
     * Density - Asteroid Material Density
     * 
     * The density of the asteroid material in kg/m³.
     * This is used to calculate mass for physics simulation.
     * 
     * Default: 7874.0 kg/m³ (steel density - realistic for metallic asteroids)
     * Typical range: 2000-8000 kg/m³ (rock to metal density)
     */
    UPROPERTY(EditAnywhere, Category = "Asteroid Generation")
    float Density = 7874.0f;

    /**
     * GlobalSeed - Global Random Seed
     * 
     * The main random seed for asteroid generation.
     * This controls the overall shape and size of the asteroid.
     * 
     * -1 = Use random seed (different each time)
     * Any other value = Use this specific seed (reproducible)
     */
    UPROPERTY(EditAnywhere, Category = "Asteroid Generation")
    int32 GlobalSeed = -1;

    /**
     * NoiseLayers - Noise Layer Configuration
     * 
     * Array of noise layers that define the asteroid's surface deformation.
     * Each layer adds different scales and types of surface variation.
     * Multiple layers can be combined for complex, realistic asteroid shapes.
     */
    UPROPERTY(EditAnywhere, Category = "Asteroid Generation")
    TArray<FNoiseLayer> NoiseLayers;

    /**
     * MaxDisplacementFraction - Maximum Displacement Limit
     * 
     * Limits how much the noise can deform the asteroid shape.
     * This prevents extreme deformations that might break the mesh.
     * 
     * Default: 0.5 (50% of base radius maximum displacement)
     * Range: 0.0-1.0 (0% to 100% of base radius)
     */
    UPROPERTY(EditAnywhere, Category = "Asteroid Generation", meta = (ClampMin = "0.0"))
    float MaxDisplacementFraction = 0.5f;

    /**
     * bEnablePhysics - Physics Simulation
     * 
     * Whether to enable physics simulation for the asteroid.
     * When enabled, the asteroid will have collision and respond to forces.
     * 
     * Default: true (physics enabled)
     */
    UPROPERTY(EditAnywhere, Category = "Asteroid Generation")
    bool bEnablePhysics = true;

    // ============================================================================
    // EVENTS
    // ============================================================================
    
    /**
     * OnAsteroidGenerated - Asteroid Generation Event
     * 
     * Multicast delegate that is broadcast when an asteroid is successfully generated.
     * This allows other systems to respond to asteroid creation events.
     * 
     * Use this to trigger gameplay events, spawn effects, or update game state
     * when new asteroids are created.
     */
    UPROPERTY(BlueprintAssignable, Category = "Asteroid")
    FOnAsteroidGenerated OnAsteroidGenerated;

    // ============================================================================
    // PUBLIC INTERFACE
    // ============================================================================
    
    /**
     * GetMass - Get Asteroid Mass
     * 
     * Returns the calculated mass of the asteroid in kilograms.
     * This is computed from volume and density during generation.
     * 
     * @return Asteroid mass in kilograms
     */
    UFUNCTION(BlueprintCallable, Category = "Asteroid")
    double GetMass() const { return AsteroidStats.Mass; }

    /**
     * GetAsteroidStats - Get Asteroid Statistics
     * 
     * Returns the complete statistics structure for the asteroid.
     * This includes radius, volume, mass, and noise layer seeds.
     * 
     * @return Complete asteroid statistics
     */
    UFUNCTION(BlueprintCallable, Category = "Asteroid")
    FAsteroidStats GetAsteroidStats() const { return AsteroidStats; }

private:
    // ============================================================================
    // INTERNAL STATE
    // ============================================================================
    
    /**
     * AsteroidStats - Asteroid Statistics Storage
     * 
     * Internal storage for calculated asteroid statistics.
     * This is populated during generation and used for physics setup.
     */
    FAsteroidStats AsteroidStats;

    // ============================================================================
    // MESH GENERATION
    // ============================================================================
    
    /**
     * BuildBaseIcosphere - Create Base Icosphere Mesh
     * 
     * Creates the base icosphere mesh and subdivides it to the specified level.
     * This forms the foundation for the asteroid shape.
     * 
     * @param Vertices - Output array for mesh vertices
     * @param Triangles - Output array for mesh triangles
     * @param SubdivisionsLevel - Number of subdivision levels
     */
    void BuildBaseIcosphere(TArray<FVector>& Vertices, TArray<int32>& Triangles, int32 SubdivisionsLevel);
    
    /**
     * SubdivideIcosphere - Subdivide Icosphere Mesh
     * 
     * Subdivides the icosphere mesh to increase detail.
     * Each subdivision level approximately quadruples the triangle count.
     * 
     * @param Vertices - Mesh vertices (modified in place)
     * @param Triangles - Mesh triangles (modified in place)
     */
    void SubdivideIcosphere(TArray<FVector>& Vertices, TArray<int32>& Triangles);
    
    /**
     * GetMiddlePoint - Find Middle Point of Edge
     * 
     * Finds or creates the middle point of an edge between two vertices.
     * This is used during icosphere subdivision to maintain mesh topology.
     * 
     * @param p1 - First vertex index
     * @param p2 - Second vertex index
     * @param Vertices - Vertex array
     * @param Cache - Cache for middle points to avoid duplicates
     * @return Index of middle point vertex
     */
    int32 GetMiddlePoint(int32 p1, int32 p2, TArray<FVector>& Vertices, TMap<int64, int32>& Cache);

    // ============================================================================
    // NOISE AND DEFORMATION
    // ============================================================================
    
    /**
     * ApplyNoiseLayers - Apply Noise Deformation
     * 
     * Applies multiple layers of noise to deform the asteroid shape.
     * Each layer adds different scales and types of surface variation.
     * 
     * @param Vertices - Mesh vertices (modified in place)
     * @param LayerSeeds - Random seeds for each noise layer
     * @param MaxDisplacementFrac - Maximum displacement limit
     */
    void ApplyNoiseLayers(TArray<FVector>& Vertices, const TArray<int32>& LayerSeeds, float MaxDisplacementFrac);

    // ============================================================================
    // UTILITIES
    // ============================================================================
    
    /**
     * NormalizeVertices - Normalize Vertex Positions
     * 
     * Normalizes all vertices to unit length, creating a perfect sphere.
     * This is done before applying noise deformation.
     * 
     * @param Vertices - Mesh vertices (modified in place)
     */
    void NormalizeVertices(TArray<FVector>& Vertices);
    
    /**
     * CreateMeshFromData - Create Procedural Mesh
     * 
     * Creates the final procedural mesh from vertex and triangle data.
     * This handles both rendering and physics collision setup.
     * 
     * @param Vertices - Mesh vertices
     * @param Triangles - Mesh triangles
     * @param bCreateCollision - Whether to create physics collision
     */
    void CreateMeshFromData(const TArray<FVector>& Vertices, const TArray<int32>& Triangles, bool bCreateCollision);

    // ============================================================================
    // GENERATION LIFECYCLE
    // ============================================================================
    
    /**
     * GenerateAsteroid - Main Generation Function
     * 
     * Orchestrates the entire asteroid generation process:
     * 1. Create base icosphere
     * 2. Apply noise deformation
     * 3. Scale to desired size
     * 4. Calculate statistics
     * 5. Create mesh and physics
     * 6. Broadcast generation event
     */
    void GenerateAsteroid();

    // ============================================================================
    // STATISTICS CALCULATION
    // ============================================================================
    
    /**
     * CalculateStats - Calculate Asteroid Statistics
     * 
     * Calculates the final statistics for the generated asteroid.
     * This includes radius, volume, and mass calculations.
     * 
     * @param ChosenRadius - The selected radius for the asteroid
     */
    void CalculateStats(float ChosenRadius);

    /**
     * CalculateVolumeFromRadius - Calculate Volume from Radius
     * 
     * Calculates the volume of a sphere with the given radius.
     * This is used for mass calculations.
     * 
     * @param Radius - Sphere radius in centimeters
     * @return Volume in cubic centimeters
     */
    double CalculateVolumeFromRadius(double Radius) const;
};
