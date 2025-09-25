/**
 * ShipPawn Implementation
 * 
 * This file contains the implementation of the main player ship class.
 * It handles component creation, camera systems, and ship behavior.
 * 
 * Key Systems:
 * - Component hierarchy setup (physics, visual, camera)
 * - Camera positioning and look-at logic
 * - Asset loading from configuration
 * - Debug and monitoring systems
 */

#include "ShipPawn.h"

// Core Unreal Engine includes
#include "Components/StaticMeshComponent.h"  // For ship meshes
#include "Camera/CameraComponent.h"          // For camera functionality
#include "Components/SceneComponent.h"       // For positioning components
#include "Components/PrimitiveComponent.h"   // For physics components

// Game-specific includes
#include "SHIP_BASICS.h"                     // Ship gameplay logic
#include "AgnosticController.h"              // Input handling
#include "ExhaustBellController.h"

// Engine utilities
#include "Engine/StreamableManager.h"        // Asset loading
#include "Engine/AssetManager.h"             // Asset management
#include "Engine/Engine.h"                   // Debug output

/**
 * Log Category Definition
 * 
 * Creates a dedicated log category for ShipPawn messages.
 * This allows filtering log output to only show ship-related messages
 * when debugging. Use UE_LOG(LogShipPawn, Warning, TEXT("Message")) to log.
 */
DEFINE_LOG_CATEGORY_STATIC(LogShipPawn, Log, All);

/**
 * AShipPawn Constructor
 * 
 * Initializes all ship components and establishes the component hierarchy.
 * This is where the ship's "skeleton" is built - all the components that
 * make up the ship are created and configured here.
 * 
 * Component Creation Order:
 * 1. Root components (physics body)
 * 2. Visual components (meshes)
 * 3. Camera components (positioning and cameras)
 * 4. Gameplay components (logic and behavior)
 * 
 * Note: Physics properties are set in PostInitializeComponents() because
 * all components must be fully constructed before physics can be configured.
 */
AShipPawn::AShipPawn()
{
	// ============================================================================
	// ROOT COMPONENT SETUP
	// ============================================================================
	
	/**
	 * ShipRoot - Master Container Component
	 * 
	 * Creates the root scene component that acts as a parent container
	 * for all ship components. This provides a common transform origin
	 * and makes the component hierarchy more organized.
	 */
	ShipRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ShipRoot"));

	/**
	 * BuggyColliderMesh - Physics Body (Root Component)
	 * 
	 * This is the ship's physics representation and the root component
	 * of the entire actor. The ship's world position and rotation come
	 * from this component, making it the "anchor" of the ship.
	 * 
	 * Why it's the root:
	 * - Physics simulation drives the ship's movement
	 * - Collision detection happens here
	 * - All other components follow this component's transform
	 */
	BuggyColliderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BuggyColliderMesh"));
	SetRootComponent(BuggyColliderMesh); // This makes it the actor's root
	ShipRoot->SetupAttachment(BuggyColliderMesh);

	// ============================================================================
	// PHYSICS BODY CONFIGURATION
	// ============================================================================
	
	/**
	 * Physics Simulation Setup
	 * 
	 * Enables rigid body physics simulation for the collision mesh.
	 * This allows the ship to:
	 * - Respond to forces and torques
	 * - Collide with other objects
	 * - Maintain momentum and inertia
	 * - Be affected by gravity (if enabled)
	 */
	BuggyColliderMesh->SetSimulatePhysics(true);
	
	/**
	 * Collision Configuration
	 * 
	 * Sets up collision detection for the physics body:
	 * - QueryAndPhysics: Can be queried for overlaps AND participate in physics
	 * - ECC_Pawn: Collision channel for player-controlled objects
	 * 
	 * This allows the ship to:
	 * - Detect when it hits other objects
	 * - Trigger collision events
	 * - Be detected by other systems (camera, AI, etc.)
	 */
	BuggyColliderMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BuggyColliderMesh->SetCollisionObjectType(ECC_Pawn);
	
	/**
	 * Visual Configuration
	 * 
	 * The collision mesh is visible in the editor for alignment purposes
	 * but hidden during gameplay to avoid visual clutter. Players only
	 * see the high-resolution visual mesh, not the simple collision mesh.
	 * 
	 * Debug visibility can be enabled via bShowColliderInGame property.
	 */
	BuggyColliderMesh->SetVisibility(true, true);  // Visible in editor
	BuggyColliderMesh->SetHiddenInGame(true);       // Hidden during play
	
	/**
	 * Physics Properties Note
	 * 
	 * Mass, damping, and other physics properties are set in
	 * PostInitializeComponents() because all components must be
	 * fully constructed before physics can be configured.
	 * 
	 * This prevents initialization order issues and ensures
	 * consistent physics behavior.
	 */
	// Physics properties moved to PostInitializeComponents()
	// BuggyColliderMesh->SetMassOverrideInKg(NAME_None, 1000.0f, true);
	// BuggyColliderMesh->SetAngularDamping(0.1f);
	// BuggyColliderMesh->SetLinearDamping(0.01f);

	// ============================================================================
	// VISUAL REPRESENTATION SETUP
	// ============================================================================
	
	/**
	 * ShipVisual - High-Resolution Visual Mesh
	 * 
	 * This is the detailed mesh that players actually see. It's separate
	 * from the physics body to allow for:
	 * - Complex geometry without physics performance impact
	 * - Different LOD (Level of Detail) levels
	 * - Visual effects and animations
	 * - Independent scaling and rotation
	 * 
	 * The visual mesh has no collision because physics is handled by
	 * the BuggyColliderMesh. This two-mesh system provides the best
	 * of both worlds: accurate physics and detailed visuals.
	 */
	ShipVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipVisual"));
	ShipVisual->SetupAttachment(BuggyColliderMesh);
	ShipVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // ============================================================================
    // CAMERA SYSTEM SETUP - Two-Component Stick Approach
    // ============================================================================
	
    /**
     * CameraPivot - Rotation Point of the Camera Stick
     * Position at ship center, used as hinge for the stick rotation
     */
    CameraPivot = CreateDefaultSubobject<USceneComponent>(TEXT("CameraPivot"));
    CameraPivot->SetupAttachment(BuggyColliderMesh);
    CameraPivot->SetRelativeLocation(ChaseCamera.PivotOffset);

    /**
     * CameraStick - End Point of the Camera Stick
     * Distance from pivot defines boom length and height
     */
    CameraStick = CreateDefaultSubobject<USceneComponent>(TEXT("CameraStick"));
    CameraStick->SetupAttachment(CameraPivot);
    CameraStick->SetRelativeLocation(ChaseCamera.StickOffset);

	/**
	 * FOLLOW_CAM - Third-Person Chase Camera
	 * 
	 * The main camera that follows behind the ship. It's attached to
	 * CameraStick and automatically looks at the ship center each frame.
	 * 
	 * Configuration:
	 * - bUsePawnControlRotation = false: Camera doesn't follow controller input
	 * - SetUsingAbsoluteRotation = true: Camera rotation is set explicitly
	 * - SetActive(true): This camera is active by default
	 * - Relative location/rotation: Camera is at the stick's origin
	 */
    FOLLOW_CAM = CreateDefaultSubobject<UCameraComponent>(TEXT("FOLLOW_CAM"));
    FOLLOW_CAM->SetupAttachment(CameraStick);
    FOLLOW_CAM->bUsePawnControlRotation = false;
    FOLLOW_CAM->SetUsingAbsoluteRotation(true);
    FOLLOW_CAM->SetRelativeRotation(FRotator::ZeroRotator);
    FOLLOW_CAM->SetRelativeLocation(FVector::ZeroVector);
    FOLLOW_CAM->SetActive(true);

    // ChaseCam2: velocity-aligned rig
    CameraPivot2 = CreateDefaultSubobject<USceneComponent>(TEXT("CameraPivot2"));
    CameraPivot2->SetupAttachment(BuggyColliderMesh);
    CameraPivot2->SetRelativeLocation(ChaseCamera2.PivotOffset);

    CameraStick2 = CreateDefaultSubobject<USceneComponent>(TEXT("CameraStick2"));
    CameraStick2->SetupAttachment(CameraPivot2);
    CameraStick2->SetRelativeLocation(ChaseCamera2.StickOffset);

    FOLLOW_CAM2 = CreateDefaultSubobject<UCameraComponent>(TEXT("FOLLOW_CAM2"));
    FOLLOW_CAM2->SetupAttachment(CameraStick2);
    FOLLOW_CAM2->bUsePawnControlRotation = false;
    FOLLOW_CAM2->SetUsingAbsoluteRotation(true);
    FOLLOW_CAM2->SetRelativeRotation(FRotator::ZeroRotator);
    FOLLOW_CAM2->SetRelativeLocation(FVector::ZeroVector);
    FOLLOW_CAM2->SetActive(false);

	/**
	 * NoseStick - First-Person Camera Positioning Component
	 * 
	 * Similar to CameraStick but for the first-person cockpit view.
	 * Positioned at the front of the ship to simulate the pilot's
	 * perspective.
	 * 
	 * Default Position: (100, 0, 20) = 1m forward, 20cm above ship center
	 */
	NoseStick = CreateDefaultSubobject<USceneComponent>(TEXT("NoseStick"));
	NoseStick->SetupAttachment(BuggyColliderMesh);
	NoseStick->SetRelativeLocation(FVector(100.0f, 0.0f, 20.0f));

	/**
	 * NOSE_CAM - First-Person Cockpit Camera
	 * 
	 * The cockpit camera that provides a first-person view from the
	 * ship's front. Used for immersive piloting experience.
	 * 
	 * Configuration:
	 * - SetActive(false): Inactive by default (chase camera is primary)
	 * - bUsePawnControlRotation = false: Camera rotation handled by code
	 */
	NOSE_CAM = CreateDefaultSubobject<UCameraComponent>(TEXT("NOSE_CAM"));
	NOSE_CAM->SetupAttachment(NoseStick);
	NOSE_CAM->bUsePawnControlRotation = false;
	NOSE_CAM->SetActive(false);

	// ============================================================================
	// GAMEPLAY SYSTEMS SETUP
	// ============================================================================
	
	/**
	 * ShipBasics - Gameplay Logic Component
	 * 
	 * Handles all ship gameplay mechanics including:
	 * - Input processing (thrust, rotation, boost)
	 * - Physics force application
	 * - Speed limiting and damping
	 * - Special maneuvers (orient opposite)
	 * 
	 * This component is the "brain" of the ship, processing player
	 * input and converting it into physics forces and torques.
	 */
	ShipBasics = CreateDefaultSubobject<USHIP_BASICS>(TEXT("ShipBasics"));

    // Exhaust bell controller
    ExhaustBellController = CreateDefaultSubobject<UExhaustBellController>(TEXT("ExhaustBellController"));

	// ============================================================================
	// ACTOR CONFIGURATION
	// ============================================================================
	
	/**
	 * AutoPossessPlayer - Automatic Player Control
	 * 
	 * Automatically assigns this ship to Player 0 (the first player)
	 * when it spawns. This means the ship will immediately be
	 * controllable without manual possession setup.
	 */
	AutoPossessPlayer = EAutoReceiveInput::Player0;
	
	/**
	 * SpawnCollisionHandlingMethod - Spawn Behavior
	 * 
	 * Determines what happens if the ship spawns inside another object:
	 * - AdjustIfPossibleButAlwaysSpawn: Try to move to a clear spot,
	 *   but spawn anyway even if collision occurs
	 * 
	 * This prevents spawn failures in crowded areas while still
	 * attempting to find a good spawn location.
	 */
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	/**
	 * Tick Configuration
	 * 
	 * Sets up the ship's update cycle:
	 * - bCanEverTick = true: Ship needs to update every frame
	 * - TickGroup = TG_PostPhysics: Update after physics simulation
	 * 
	 * This ensures camera positioning happens after the ship has
	 * moved, providing smooth and accurate camera behavior.
	 */
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	/**
	 * Debug State Initialization
	 * 
	 * Initializes debug-related variables to their default values.
	 * These are used for timing debug displays and other monitoring.
	 */
	CameraDebugAccum = 0.0f;
}

/**
 * BeginPlay - Ship Initialization
 * 
 * Called when the ship spawns in the world. This function handles:
 * - Applying collider alignment corrections
 * - Wiring component references for gameplay systems
 * - Activating the follow camera
 * - Loading mesh assets from configuration
 * - Setting up debug visibility
 * 
 * This is where the ship becomes fully functional and ready for play.
 */
void AShipPawn::BeginPlay()
{
	Super::BeginPlay();

	// ============================================================================
	// COLLIDER ALIGNMENT CORRECTION
	// ============================================================================
	
	/**
	 * Apply Runtime Collider Offsets
	 * 
	 * Corrects misaligned collision meshes by applying rotation and
	 * position offsets. This is useful when imported meshes have
	 * different coordinate systems than expected.
	 * 
	 * These offsets are applied at runtime rather than in the constructor
	 * to ensure all components are fully initialized first.
	 */
    if (BuggyColliderMesh)
	{
		BuggyColliderMesh->AddLocalRotation(ColliderRotationOffset);
		BuggyColliderMesh->AddLocalOffset(ColliderLocationOffset);

        // Apply center of mass offset (local space)
        if (!CenterOfMassOffset.IsNearlyZero())
        {
            BuggyColliderMesh->SetCenterOfMass(CenterOfMassOffset, NAME_None);
        }
	}

	// ============================================================================
	// COMPONENT REFERENCE WIRING
	// ============================================================================
	
	/**
	 * Wire ShipBasics Component References
	 * 
	 * Ensures the gameplay logic component has proper references to
	 * the physics body and visual root. This allows ShipBasics to:
	 * - Apply forces to the physics body
	 * - Access visual components for effects
	 * - Coordinate between physics and visuals
	 */
	if (ShipBasics)
	{
		// Wire physics body reference if not already set
		if (!ShipBasics->ControlledBody)
		{
			ShipBasics->ControlledBody = BuggyColliderMesh;
		}
		
		// Wire visual root reference (prefer ShipVisual, fallback to ShipRoot)
		if (!ShipBasics->VisualRoot)
		{
			ShipBasics->VisualRoot = ShipVisual ? static_cast<USceneComponent*>(ShipVisual) : static_cast<USceneComponent*>(ShipRoot);
		}
	}

	// ============================================================================
	// CAMERA SYSTEM ACTIVATION
	// ============================================================================
	
	/**
	 * Activate Follow Camera
	 * 
	 * Ensures the follow camera is active and properly configured.
	 * This is the default camera that players see when the ship spawns.
	 */
	if (FOLLOW_CAM)
	{
		FOLLOW_CAM->SetActive(true);
		
		/**
		 * Clear Blueprint Offsets
		 * 
		 * Ensures the camera is positioned exactly where the CameraStick
		 * places it, without any additional Blueprint-applied offsets
		 * that might interfere with the positioning system.
		 */
		FOLLOW_CAM->SetRelativeLocation(FVector::ZeroVector);
		FOLLOW_CAM->SetRelativeRotation(FRotator::ZeroRotator);
		
		UE_LOG(LogShipPawn, Log, TEXT("FOLLOW_CAM activated at BeginPlay"));
	}
	
	/**
	 * Apply Initial Camera Mode
	 * 
	 * Sets up the camera system according to the current camera mode.
	 * The bInstant parameter ensures immediate camera activation without
	 * transition delays.
	 */
	ApplyCameraMode(true);

	// ============================================================================
	// DEBUG VISIBILITY SETUP
	// ============================================================================
	
	/**
	 * Configure Collider Debug Visibility
	 * 
	 * Shows or hides the collision mesh based on the debug setting.
	 * This is useful for:
	 * - Debugging collision issues
	 * - Verifying mesh alignment
	 * - Performance testing
	 */
	if (BuggyColliderMesh)
	{
		BuggyColliderMesh->SetHiddenInGame(!bShowColliderInGame);
	}

	// ============================================================================
	// ASSET LOADING FROM CONFIGURATION
	// ============================================================================
	
	/**
	 * Load Mesh Assets from Configuration
	 * 
	 * Allows setting mesh assets via config file instead of Blueprint.
	 * This is useful for:
	 * - Version control (no Blueprint dependencies)
	 * - Runtime mesh swapping
	 * - Automated testing
	 * - Different builds with different assets
	 * 
	 * The assets are loaded synchronously to ensure they're available
	 * immediately when the ship spawns.
	 */
	if (ColliderMeshAsset.IsValid() || VisualMeshAsset.IsValid())
	{
		FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
		
		// Load collision mesh if specified
		if (ColliderMeshAsset.IsValid())
		{
			UStaticMesh* ColliderMesh = Cast<UStaticMesh>(Streamable.LoadSynchronous(ColliderMeshAsset));
			if (ColliderMesh && BuggyColliderMesh)
			{
				BuggyColliderMesh->SetStaticMesh(ColliderMesh);
			}
		}
		
		// Load visual mesh if specified
		if (VisualMeshAsset.IsValid())
		{
			UStaticMesh* VisualMesh = Cast<UStaticMesh>(Streamable.LoadSynchronous(VisualMeshAsset));
			if (VisualMesh && ShipVisual)
			{
				ShipVisual->SetStaticMesh(VisualMesh);
			}
		}
	}
}

/**
 * PostInitializeComponents - Physics Configuration
 * 
 * Called after all components are fully constructed. This is where
 * physics properties are set because all components must exist
 * before physics can be configured.
 * 
 * This function sets up the ship's physics behavior:
 * - Mass: 1000kg (realistic for a small spacecraft)
 * - Angular damping: 0.1 (slight rotation damping for stability)
 * - Linear damping: 0.0 (true space physics - no air resistance)
 */
void AShipPawn::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (BuggyColliderMesh)
	{
		// Set ship mass to 1000kg (realistic for small spacecraft)
		BuggyColliderMesh->SetMassOverrideInKg(NAME_None, 1000.0f, true);
		
		// Angular damping: slight rotation damping for stability
		BuggyColliderMesh->SetAngularDamping(0.1f);
		
		// Linear damping: 0.0 for true space physics (no air resistance)
		BuggyColliderMesh->SetLinearDamping(0.0f);
	}
}

void AShipPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!CameraStick || !FOLLOW_CAM || !BuggyColliderMesh) { return; }

    if (CameraMode == ECameraMode::Chase)
	{
		// Simple camera stick approach - camera follows ship orientation
		// CameraStick position and camera offset are set in Blueprint
		// Camera naturally looks toward ship center since it's attached to the stick
		
        // Make camera look toward ship center, with optional roll match
        const FVector CamWorldPos = FOLLOW_CAM->GetComponentLocation();
        const FVector ShipCenter = GetActorLocation();
        FRotator LookAtRot = (ShipCenter - CamWorldPos).Rotation();

        if (bChaseCamMatchRoll)
        {
            // Replace camera roll with ship's roll about its forward (X) axis
            const float ShipRoll = GetActorRotation().Roll;
            LookAtRot.Roll = ShipRoll;
        }
        FOLLOW_CAM->SetWorldRotation(LookAtRot);

		// On-screen debug
		if (bShowCameraDebug && GEngine)
		{
			CameraDebugAccum += DeltaSeconds;
			if (CameraDebugAccum >= FMath::Max(0.05f, CameraDebugInterval))
			{
				CameraDebugAccum = 0.0f;
				const FVector StickLoc = CameraStick->GetRelativeLocation();
				const FVector CamLoc = FOLLOW_CAM->GetRelativeLocation();
				const FString Msg = FString::Printf(TEXT("Stick=(%.0f,%.0f,%.0f) Cam=(%.0f,%.0f,%.0f)"),
					StickLoc.X, StickLoc.Y, StickLoc.Z,
					CamLoc.X, CamLoc.Y, CamLoc.Z);
				GEngine->AddOnScreenDebugMessage(/*Key=*/1, /*Time=*/CameraDebugInterval, FColor::Cyan, Msg);
			}
		}
		return;
	}

    if (CameraMode == ECameraMode::Chase2)
    {
        if (FOLLOW_CAM2 && CameraPivot2 && BuggyColliderMesh)
        {
            // Align pivot to current velocity direction; fallback to actor forward when nearly stopped
            const FVector Vel = BuggyColliderMesh->GetPhysicsLinearVelocity();
            const bool bHasVel = Vel.SizeSquared() > FMath::Square(10.f); // >10 cm/s
            const FVector Dir = bHasVel ? Vel.GetSafeNormal() : GetActorForwardVector();
            const FRotator Target = Dir.Rotation();
            CameraPivot2->SetWorldRotation(Target);

            // Camera looks back at ship center from the stick end
            const FVector CamPos = FOLLOW_CAM2->GetComponentLocation();
            const FVector ShipCenter = GetActorLocation();
            const FRotator LookAt = (ShipCenter - CamPos).Rotation();
            FOLLOW_CAM2->SetWorldRotation(LookAt);
        }
        return;
    }
}

void AShipPawn::ToggleCameraMode()
{
    // Cycle: Chase -> Chase2 -> Nose -> Chase ...
    switch (CameraMode)
    {
        case ECameraMode::Chase:  CameraMode = ECameraMode::Chase2; break;
        case ECameraMode::Chase2: CameraMode = ECameraMode::Nose;   break;
        default:                  CameraMode = ECameraMode::Chase;  break;
    }
	ApplyCameraMode(true);
}

void AShipPawn::ZeroShipRotation()
{
	if (ShipBasics)
	{
		ShipBasics->ZeroAngularVelocity();
	}
}

void AShipPawn::ApplyCameraMode(bool bInstant)
{
    const bool bUseNose  = (CameraMode == ECameraMode::Nose);
    const bool bUseChase2 = (CameraMode == ECameraMode::Chase2);

    if (NOSE_CAM)     { NOSE_CAM->SetActive(bUseNose); }
    if (FOLLOW_CAM)   { FOLLOW_CAM->SetActive(!bUseNose && !bUseChase2); }
    if (FOLLOW_CAM2)  { FOLLOW_CAM2->SetActive(bUseChase2); }

    if (NoseStick)
	{
		NoseStick->SetRelativeLocation(FVector(NoseOffsetForward, 0.f, NoseOffsetUp));
		if (bInstant)
		{
			NoseStick->SetWorldRotation(GetActorRotation());
		}
	}
}

void AShipPawn::TickCameraTrack(float DeltaSeconds, bool bTrackHeld)
{
    if (!CameraPivot) return;

    if (bTrackHeld)
    {
        if (!bCameraTrackActive)
        {
            bCameraTrackActive = true;
            CameraTrackAccumulated = 0.0f;
        }

        const FRotator Current = CameraPivot->GetComponentRotation();
        const FRotator Target = GetActorRotation();
        const float RemainingYaw = FMath::Abs(FMath::FindDeltaAngleDegrees(Current.Yaw, Target.Yaw));
        const float SegmentSeconds = FMath::Clamp((RemainingYaw / 180.0f) * CameraTrackMaxSeconds, 0.01f, CameraTrackMaxSeconds);
        const float InterpSpeed = (SegmentSeconds > 0.f) ? (1.0f / SegmentSeconds) : 1000.f;

        const FRotator NewRot = FMath::RInterpTo(Current, Target, DeltaSeconds, InterpSpeed);
        CameraPivot->SetWorldRotation(NewRot);

        CameraTrackAccumulated += DeltaSeconds;
    }
    else
    {
        // Freeze while not held
        bCameraTrackActive = false;
    }
}


