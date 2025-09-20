#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ShipPawn.generated.h"

// Forward declarations to reduce compilation dependencies
class USceneComponent;
class UStaticMeshComponent;
class UCameraComponent;
class USHIP_BASICS;

/**
 * FChaseCameraSettings - Blueprint-tunable chase camera configuration
 */
USTRUCT(BlueprintType)
struct FChaseCameraSettings
{
	GENERATED_BODY()

	/** Pivot offset relative to the physics body (where the stick hinges) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Chase")
	FVector PivotOffset = FVector::ZeroVector;

	/** Stick end offset relative to the pivot (defines boom length/height) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Chase")
	FVector StickOffset = FVector(-1000.0f, 0.0f, 150.0f);
};

/**
 * Camera Mode Enumeration
 * 
 * Defines the two available camera perspectives for the ship:
 * - Chase: Third-person camera that follows behind the ship
 * - Nose: First-person camera positioned at the ship's front
 * 
 * This enum is exposed to Blueprint for runtime camera switching.
 */
UENUM(BlueprintType)
enum class ECameraMode : uint8
{
	Chase = 0 UMETA(DisplayName = "Chase"),  // Third-person follow camera
	Nose = 1 UMETA(DisplayName = "Nose")     // First-person cockpit camera
};

/**
 * ShipPawn - Main Player Ship Class
 * 
 * This is the core player-controlled ship in the space racing game. It handles:
 * - Physics simulation (rigid body movement, collision)
 * - Visual representation (high-res mesh separate from collision)
 * - Camera system (chase and nose cameras with simple positioning)
 * - Input processing (delegated to AgnosticController)
 * - Gameplay mechanics (delegated to SHIP_BASICS component)
 * 
 * Component Hierarchy:
 * BuggyColliderMesh (Root - Physics Body)
 * ├── ShipVisual (High-res visual mesh)
 * ├── CameraPivot (Stick rotation pivot at ship center)
 * │   └── CameraStick (Stick end; defines boom length)
 * │       └── FOLLOW_CAM (Chase camera mounted at stick end)
 * ├── NoseStick (Nose camera positioning)
 * │   └── NOSE_CAM (Cockpit camera)
 * └── ShipBasics (Gameplay logic component)
 * 
 * The ship uses a two-mesh system: a simple collision mesh for physics
 * and a detailed visual mesh for rendering. This allows for accurate
 * physics simulation while maintaining visual fidelity.
 */
UCLASS(config=Game)
class SPAAAAAACE_API AShipPawn : public APawn
{
	GENERATED_BODY()

public:
	/**
	 * Constructor
	 * 
	 * Initializes all ship components and sets up the component hierarchy.
	 * Physics properties are set in PostInitializeComponents() to ensure
	 * all components are fully constructed first.
	 */
	AShipPawn();

protected:
	/**
	 * BeginPlay - Called when the ship spawns in the world
	 * 
	 * - Applies collider alignment offsets
	 * - Wires component references for gameplay systems
	 * - Activates the follow camera
	 * - Loads mesh assets from config if specified
	 * - Sets up debug visibility
	 */
	virtual void BeginPlay() override;
	
	/**
	 * PostInitializeComponents - Called after all components are created
	 * 
	 * This is where physics properties are set because all components
	 * must be fully constructed before physics can be configured.
	 * Sets mass, damping, and other physics parameters.
	 */
	virtual void PostInitializeComponents() override;
	
	/**
	 * Tick - Called every frame
	 * 
	 * Handles camera positioning and look-at logic for the chase camera.
	 * The camera system uses a simple "stick" approach where the camera
	 * is positioned by a SceneComponent and looks at the ship center.
	 */
	virtual void Tick(float DeltaSeconds) override;

public:
	// ============================================================================
	// SHIP COMPONENT HIERARCHY
	// ============================================================================
	
	/**
	 * ShipRoot - Root Scene Component
	 * 
	 * Acts as a parent container for all ship components. This provides
	 * a common transform origin for the entire ship assembly. All other
	 * components are children of this root.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Ship|Components")
	TObjectPtr<USceneComponent> ShipRoot;

	/**
	 * BuggyColliderMesh - Physics Body Component
	 * 
	 * This is the ship's physics representation - a simple collision mesh
	 * that drives all physics simulation. It's the root component of the
	 * actor, meaning the ship's world position/rotation comes from this.
	 * 
	 * Properties:
	 * - Simulates physics (rigid body dynamics)
	 * - Has collision enabled for world interaction
	 * - Hidden in game by default (debug visibility available)
	 * - Mass: 1000kg (configurable)
	 * - Damping: Angular=0.1, Linear=0.0 (true space physics)
	 */
	UPROPERTY(VisibleAnywhere, Category = "Ship|Components")
	TObjectPtr<UStaticMeshComponent> BuggyColliderMesh;

	/**
	 * ShipVisual - High-Resolution Visual Mesh
	 * 
	 * The detailed visual representation of the ship that players see.
	 * This is separate from the physics body to allow for:
	 * - Complex geometry without physics performance impact
	 * - Different LOD levels for rendering optimization
	 * - Visual effects and animations
	 * 
	 * Properties:
	 * - No collision (physics handled by BuggyColliderMesh)
	 * - Attached to physics body for synchronized movement
	 * - Can be scaled/rotated independently for visual effects
	 */
	UPROPERTY(VisibleAnywhere, Category = "Ship|Components")
	TObjectPtr<UStaticMeshComponent> ShipVisual;

	/**
	 * CameraPivot - Rotation Point of the Camera Stick
	 * 
	 * Acts as the base/hinge point for an invisible camera stick.
	 * Rotating this component swings the camera in an arc around the
	 * pivot, maintaining the stick length.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Ship|Components")
	TObjectPtr<USceneComponent> CameraPivot;

	/**
	 * CameraStick - End Point of the Camera Stick
	 * 
	 * Represents the end of the invisible camera stick extending from
	 * CameraPivot. The distance Pivot→Stick defines the boom length.
	 * Default: (-1000, 0, 150)
	 */
	UPROPERTY(VisibleAnywhere, Category = "Ship|Components")
	TObjectPtr<USceneComponent> CameraStick;

	/**
	 * FOLLOW_CAM - Third-Person Chase Camera
	 * 
	 * The main camera that follows behind the ship. It's attached to
	 * CameraStick and automatically looks at the ship center each frame.
	 * 
	 * Behavior:
	 * - Positioned by CameraStick transform
	 * - Looks at ship center via look-at calculation
	 * - Active by default when ship spawns
	 * - Can be toggled with camera mode switching
	 */
	UPROPERTY(VisibleAnywhere, Category = "Ship|Components")
	TObjectPtr<UCameraComponent> FOLLOW_CAM;

	/**
	 * NoseStick - First-Person Camera Positioning Component
	 * 
	 * Similar to CameraStick but for the first-person cockpit view.
	 * Positioned at the front of the ship to simulate the pilot's
	 * perspective.
	 * 
	 * Default Position: (100, 0, 20) = 1m forward, 20cm above ship center
	 */
	UPROPERTY(VisibleAnywhere, Category = "Ship|Components")
	TObjectPtr<USceneComponent> NoseStick;

	/**
	 * NOSE_CAM - First-Person Cockpit Camera
	 * 
	 * The cockpit camera that provides a first-person view from the
	 * ship's front. Used for immersive piloting experience.
	 * 
	 * Behavior:
	 * - Positioned by NoseStick transform
	 * - Inherits ship rotation for natural cockpit feel
	 * - Inactive by default (chase camera is primary)
	 */
	UPROPERTY(VisibleAnywhere, Category = "Ship|Components")
	TObjectPtr<UCameraComponent> NOSE_CAM;

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
	UPROPERTY(VisibleAnywhere, Category = "Ship|Components")
	TObjectPtr<USHIP_BASICS> ShipBasics;

	// ============================================================================
	// ASSET CONFIGURATION
	// ============================================================================
	
	/**
	 * ColliderMeshAsset - Physics Mesh Asset Path
	 * 
	 * Allows setting the collision mesh asset via config file instead of Blueprint.
	 * This is useful for:
	 * - Version control (no Blueprint dependencies)
	 * - Runtime mesh swapping
	 * - Automated testing
	 * 
	 * Config Example (DefaultGame.ini):
	 * [/Script/SPAAAAAACE.ShipPawn]
	 * ColliderMeshAsset=/Game/Path/To/Meshes/buggycollidermesh.buggycollidermesh
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Ship|Assets", meta=(AllowedClasses="/Script/Engine.StaticMesh"))
	FSoftObjectPath ColliderMeshAsset;

	/**
	 * VisualMeshAsset - Visual Mesh Asset Path
	 * 
	 * Allows setting the visual mesh asset via config file instead of Blueprint.
	 * Same benefits as ColliderMeshAsset but for the high-res visual mesh.
	 * 
	 * Config Example (DefaultGame.ini):
	 * [/Script/SPAAAAAACE.ShipPawn]
	 * VisualMeshAsset=/Game/Path/To/Meshes/SHIP1_BUGGY.SHIP1_BUGGY
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Ship|Assets", meta=(AllowedClasses="/Script/Engine.StaticMesh"))
	FSoftObjectPath VisualMeshAsset;

	// ============================================================================
	// DEBUG AND ALIGNMENT
	// ============================================================================
	
	/**
	 * bShowColliderInGame - Debug Collision Mesh Visibility
	 * 
	 * When enabled, makes the collision mesh visible during gameplay.
	 * This is useful for:
	 * - Debugging collision issues
	 * - Verifying mesh alignment
	 * - Performance testing (collision vs visual mesh)
	 * 
	 * The collision mesh is normally hidden to avoid visual clutter.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Ship|Debug")
	bool bShowColliderInGame = false;

	/**
	 * ColliderRotationOffset - Physics Body Rotation Correction
	 * 
	 * Applied at runtime to correct misaligned collision meshes.
	 * This is useful when the imported collision mesh has a different
	 * forward/up axis than expected by the physics system.
	 * 
	 * Example: If collision mesh is rotated 90° from ship forward,
	 * set this to (0, 90, 0) to correct it.
	 */
	UPROPERTY(EditAnywhere, Category = "Ship|Collider")
	FRotator ColliderRotationOffset = FRotator::ZeroRotator;

	/**
	 * ColliderLocationOffset - Physics Body Position Correction
	 * 
	 * Applied at runtime to correct misaligned collision mesh positions.
	 * This is useful when the collision mesh center doesn't match the
	 * visual mesh center, causing physics/visual misalignment.
	 * 
	 * Example: If collision mesh is 50cm forward of visual center,
	 * set this to (-50, 0, 0) to correct it.
	 */
	UPROPERTY(EditAnywhere, Category = "Ship|Collider")
	FVector ColliderLocationOffset = FVector::ZeroVector;

	/**
	 * CenterOfMassOffset - Physics Center Of Mass Offset
	 *
	 * Shifts the rigidbody's center of mass without moving the mesh.
	 * Useful to change rotational behavior (e.g., more nose-stable).
	 */
	UPROPERTY(EditAnywhere, Category = "Ship|Collider")
	FVector CenterOfMassOffset = FVector::ZeroVector;

	// ============================================================================
	// CAMERA SYSTEM CONFIGURATION
	// ============================================================================
	
	/**
	 * ChaseCamera - Blueprint-tunable camera pivot/stick offsets
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Chase")
	FChaseCameraSettings ChaseCamera;

	/**
	 * CameraMode - Active Camera Perspective
	 * 
	 * Controls which camera is currently active:
	 * - Chase: Third-person follow camera (default)
	 * - Nose: First-person cockpit camera
	 * 
	 * Can be changed at runtime via ToggleCameraMode() or input binding.
	 */
	UPROPERTY(EditAnywhere, Category = "Camera|Mode")
	ECameraMode CameraMode = ECameraMode::Chase;

	/**
	 * Camera Positioning Note
	 * 
	 * Camera positioning is now handled by the CameraStick and NoseStick
	 * SceneComponents, which can be positioned/rotated/scaled in Blueprint.
	 * This provides much more flexibility than the previous SpringArm system.
	 */

	// ============================================================================
	// DEBUG AND MONITORING
	// ============================================================================
	
	/**
	 * bShowCameraDebug - Camera Debug Information Display
	 * 
	 * When enabled, displays real-time camera information on screen:
	 * - CameraStick position
	 * - Camera relative position
	 * - Ship velocity and movement data
	 * 
	 * This is useful for debugging camera positioning and behavior.
	 */
	UPROPERTY(EditAnywhere, Category = "Debug|Camera")
	bool bShowCameraDebug = true;

	/**
	 * CameraDebugInterval - Debug Display Update Rate
	 * 
	 * Controls how often the debug information is updated on screen.
	 * Lower values = more frequent updates (more CPU usage)
	 * Higher values = less frequent updates (better performance)
	 * 
	 * Minimum: 0.05 seconds (20 FPS update rate)
	 */
	UPROPERTY(EditAnywhere, Category = "Debug|Camera", meta=(ClampMin="0.05"))
	float CameraDebugInterval = 0.3f;

	// ============================================================================
	// NOSE CAMERA CONFIGURATION
	// ============================================================================
	
	/**
	 * NoseOffsetForward - Cockpit Camera Forward Offset
	 * 
	 * How far forward from the ship center to position the nose camera.
	 * This simulates the pilot's eye position in the cockpit.
	 * 
	 * Default: 100cm (1 meter forward from ship center)
	 */
	UPROPERTY(EditAnywhere, Category = "Camera|Nose")
	float NoseOffsetForward = 100.0f;

	/**
	 * NoseOffsetUp - Cockpit Camera Height Offset
	 * 
	 * How high above the ship center to position the nose camera.
	 * This simulates the pilot's eye height in the cockpit.
	 * 
	 * Default: 20cm (20 centimeters above ship center)
	 */
	UPROPERTY(EditAnywhere, Category = "Camera|Nose")
	float NoseOffsetUp = 20.0f;

	/**
	 * NoseRotationLerpSpeed - Cockpit Camera Rotation Smoothing
	 * 
	 * How quickly the nose camera rotates to match ship orientation.
	 * Higher values = more responsive (can feel twitchy)
	 * Lower values = smoother (can feel sluggish)
	 * 
	 * This affects the "feel" of the cockpit camera when the ship rotates.
	 */
	UPROPERTY(EditAnywhere, Category = "Camera|Nose", meta=(ClampMin="0"))
	float NoseRotationLerpSpeed = 10.0f;

	// ============================================================================
	// PUBLIC CAMERA CONTROL FUNCTIONS
	// ============================================================================
	
	/**
	 * ToggleCameraMode - Switch Between Chase and Nose Camera
	 * 
	 * Cycles between the two available camera modes:
	 * Chase -> Nose -> Chase -> ...
	 * 
	 * This is typically bound to a key/button for player control.
	 * The actual camera switching is handled by ApplyCameraMode().
	 */
	void ToggleCameraMode();
	
	/**
	 * ApplyCameraMode - Activate Specified Camera Mode
	 * 
	 * @param bInstant - If true, immediately switch cameras without transition
	 *                   If false, allows for smooth transitions (future feature)
	 * 
	 * This function:
	 * - Deactivates the current camera
	 * - Activates the target camera
	 * - Configures camera-specific settings (position, rotation)
	 */
	void ApplyCameraMode(bool bInstant = true);
	
	/**
	 * ZeroShipRotation - Stop All Ship Rotation
	 * 
	 * Immediately stops all angular velocity of the ship, bringing it to
	 * a stable orientation. This is useful for:
	 * - Emergency stabilization
	 * - Resetting after collisions
	 * - Player-requested orientation reset
	 * 
	 * This delegates to the ShipBasics component which handles the actual
	 * physics manipulation.
	 */
	void ZeroShipRotation();

	/**
	 * TickCameraTrack - Handle Camera Tracking Input
	 * 
	 * @param DeltaSeconds - Time elapsed since last frame
	 * @param bTrackHeld - Whether the camera track input is currently held
	 * 
	 * This function handles the "camera track" feature where holding a button
	 * causes the camera to smoothly rotate to face the ship's forward direction.
	 * This is useful for:
	 * - Reorienting the camera after complex maneuvers
	 * - Getting a better view of where you're going
	 * - Cinematic camera movements
	 * 
	 * The tracking has a maximum duration to prevent infinite rotation.
	 */
	void TickCameraTrack(float DeltaSeconds, bool bTrackHeld);

private:
	// ============================================================================
	// CAMERA TRACKING SYSTEM
	// ============================================================================
	
	/**
	 * CameraTrackMaxSeconds - Maximum Camera Tracking Duration
	 * 
	 * The maximum time (in seconds) that the camera will spend tracking
	 * from the worst-case scenario (180° rotation) to ship forward.
	 * 
	 * This prevents the camera from spinning indefinitely and provides
	 * a predictable maximum tracking time for UI/UX design.
	 * 
	 * Default: 5.0 seconds (reasonable for 180° rotation)
	 */
	UPROPERTY(EditAnywhere, Category = "Camera|Chase", meta=(ClampMin="0.01"))
	float CameraTrackMaxSeconds = 5.0f;

	/**
	 * bCameraTrackActive - Camera Tracking State
	 * 
	 * True when the camera is currently in tracking mode (player holding
	 * the track button). Used to determine whether to apply tracking
	 * rotation or maintain current camera orientation.
	 */
	bool bCameraTrackActive = false;
	
	/**
	 * CameraTrackAccumulated - Time Spent Tracking
	 * 
	 * Accumulates the time spent tracking in the current hold session.
	 * Used to implement the maximum tracking duration and smooth
	 * tracking behavior.
	 */
	float CameraTrackAccumulated = 0.0f;

	// ============================================================================
	// DEBUG AND INTERNAL STATE
	// ============================================================================
	
	/**
	 * CameraDebugAccum - Debug Display Timer
	 * 
	 * Accumulates time for debug display updates. Used to control
	 * the frequency of debug information shown on screen without
	 * overwhelming the display with too many updates.
	 */
	float CameraDebugAccum = 0.0f;

	/**
	 * LastTravelDir - Previous Movement Direction
	 * 
	 * Stores the last valid movement direction when the ship was moving
	 * at sufficient speed. This is used for camera positioning when the
	 * ship is nearly stopped and velocity-based camera positioning
	 * becomes unreliable.
	 * 
	 * Default: Forward vector (assumes ship starts facing forward)
	 */
	FVector LastTravelDir = FVector::ForwardVector;
};


