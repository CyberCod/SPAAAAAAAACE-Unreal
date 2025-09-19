/**
 * SHIP_BASICS - Ship Gameplay Logic Component
 * 
 * This file defines the core gameplay mechanics for the player ship.
 * It handles input processing, physics force application, and ship behavior.
 * 
 * Key Responsibilities:
 * - Convert player input into physics forces and torques
 * - Apply realistic space physics (thrust, rotation, momentum)
 * - Handle special maneuvers (boost, orient opposite)
 * - Manage speed limits and damping
 * - Provide thruster visualization data
 * 
 * This component is the "brain" of the ship, processing all player
 * input and converting it into the physics forces that move the ship.
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SHIP_BASICS.generated.h"

// Forward declarations to reduce compilation dependencies
class UPrimitiveComponent;  // Physics body component
class USceneComponent;      // Visual root component  
class AAgnosticController;  // Input controller
struct FShipInputState;     // Input state structure

/**
 * FShipForceSettings - Ship Physics Configuration Structure
 * 
 * Contains all the tunable parameters for ship physics and behavior.
 * This structure allows designers to adjust ship feel without code changes.
 * 
 * The settings are organized into logical groups:
 * - Force magnitudes (thrust, boost, torques)
 * - Input handling (deadzones, smoothing, signs)
 * - Physics corrections (axis alignment, speed limits)
 * - Special maneuvers (orient opposite behavior)
 */
USTRUCT()
struct FShipForceSettings
{
    GENERATED_BODY()

    // ============================================================================
    // FORCE MAGNITUDES
    // ============================================================================
    
    /**
     * ThrustForce - Main Engine Thrust Strength
     * 
     * The force applied by the main engines when the player presses thrust.
     * This is the primary way the ship accelerates forward.
     * 
     * Default: 500,000 N (realistic for small spacecraft)
     * Higher values = more powerful engines
     * Lower values = weaker engines
     */
    UPROPERTY(EditAnywhere, Category = "Ship|Forces")
    float ThrustForce = 500000.0f;

    /**
     * BoostForce - Boost Engine Thrust Strength
     * 
     * Additional force applied when boost is activated. This is added
     * to the normal thrust force for maximum acceleration.
     * 
     * Default: 2,000,000 N (4x stronger than normal thrust)
     * This creates a significant speed boost when activated.
     */
    UPROPERTY(EditAnywhere, Category = "Ship|Forces")
    float BoostForce = 2000000.0f;

    /**
     * PitchTorque - Up/Down Rotation Strength
     * 
     * How much torque is applied when the player pitches up or down.
     * This controls how quickly the ship can change its vertical orientation.
     * 
     * Default: 4.0 (moderate pitch authority)
     * Higher values = more responsive pitch control
     * Lower values = slower pitch changes
     */
    UPROPERTY(EditAnywhere, Category = "Ship|Forces")
    float PitchTorque = 4.0f;

    /**
     * YawTorque - Left/Right Rotation Strength
     * 
     * How much torque is applied when the player yaws left or right.
     * This controls how quickly the ship can change its horizontal orientation.
     * 
     * Default: 15.0 (strong yaw authority)
     * Higher values = more responsive yaw control
     * Lower values = slower yaw changes
     */
    UPROPERTY(EditAnywhere, Category = "Ship|Forces")
    float YawTorque = 15.0f;

    /**
     * RollTorque - Barrel Roll Rotation Strength
     * 
     * How much torque is applied when the player rolls left or right.
     * This controls how quickly the ship can barrel roll.
     * 
     * Default: 15.0 (strong roll authority)
     * Higher values = more responsive roll control
     * Lower values = slower roll changes
     */
    UPROPERTY(EditAnywhere, Category = "Ship|Forces")
    float RollTorque = 15.0f;

    // ============================================================================
    // PHYSICS AXIS CORRECTIONS
    // ============================================================================
    
    /**
     * PhysicsAxesCorrection - Model Axis Alignment Fix
     * 
     * Corrective rotation applied to fix models with different forward/up axes.
     * This allows using models that weren't designed for the expected coordinate system
     * without modifying the actual mesh files.
     * 
     * Default: (0, -90, 0) - 90° Y rotation correction
     * Use this when your model's forward axis doesn't match the expected direction.
     */
    UPROPERTY(EditAnywhere, Category = "Ship|Forces")
    FRotator PhysicsAxesCorrection = FRotator(0.f, -90.f, 0.f);

    // ============================================================================
    // INPUT SIGN CORRECTIONS
    // ============================================================================
    
    /**
     * Input Sign Corrections - Fix Inverted Controls
     * 
     * These allow fixing inverted or incorrect control axes without code changes.
     * Use negative values to invert an axis, positive to keep normal.
     * 
     * This is useful when:
     * - Controls feel backwards
     * - Model has different axis conventions
     * - Different control schemes are needed
     */
    
    /**
     * PitchInputSign - Pitch Control Direction
     * 
     * Controls whether pulling back on the stick pitches up or down.
     * 1.0 = normal (pull back = pitch up)
     * -1.0 = inverted (pull back = pitch down)
     */
    UPROPERTY(EditAnywhere, Category = "Ship|Forces")
    float PitchInputSign = 1.0f;

    /**
     * YawInputSign - Yaw Control Direction
     * 
     * Controls whether pushing right on the stick yaws right or left.
     * 1.0 = normal (push right = yaw right)
     * -1.0 = inverted (push right = yaw left)
     */
    UPROPERTY(EditAnywhere, Category = "Ship|Forces")
    float YawInputSign = 1.0f;

    /**
     * RollInputSign - Roll Control Direction
     * 
     * Controls whether pushing right on the stick rolls right or left.
     * 1.0 = normal (push right = roll right)
     * -1.0 = inverted (push right = roll left)
     * 
     * Default: -1.0 (inverted roll for more intuitive flight controls)
     */
    UPROPERTY(EditAnywhere, Category = "Ship|Forces")
    float RollInputSign = -1.0f;

    // ============================================================================
    // INPUT PROCESSING
    // ============================================================================
    
    /**
     * AxisDeadzone - Joystick Deadzone
     * 
     * Minimum input value required before the ship responds to stick input.
     * This prevents unwanted movement from stick drift and small movements.
     * 
     * Range: 0.0 to 0.5 (0% to 50% of stick range)
     * Default: 0.10 (10% deadzone)
     * Higher values = less sensitive controls
     * Lower values = more sensitive controls
     */
    UPROPERTY(EditAnywhere, Category = "Ship|Forces", meta = (ClampMin = "0.0", ClampMax = "0.5"))
    float AxisDeadzone = 0.10f;

    /**
     * TriggerDeadzone - Trigger Deadzone
     * 
     * Minimum input value required before thrust triggers respond.
     * This prevents accidental thrust from trigger sensitivity.
     * 
     * Range: 0.0 to 0.5 (0% to 50% of trigger range)
     * Default: 0.05 (5% deadzone)
     * Higher values = less sensitive triggers
     * Lower values = more sensitive triggers
     */
    UPROPERTY(EditAnywhere, Category = "Ship|Forces", meta = (ClampMin = "0.0", ClampMax = "0.5"))
    float TriggerDeadzone = 0.05f;

    /**
     * InputSmoothing - Input Response Smoothing
     * 
     * How quickly input changes are applied to the ship.
     * Higher values = more responsive (can feel twitchy)
     * Lower values = smoother (can feel sluggish)
     * 
     * Default: 10.0 (good balance of responsiveness and smoothness)
     * This affects the "feel" of the controls.
     */
    UPROPERTY(EditAnywhere, Category = "Ship|Forces", meta = (ClampMin = "0.0"))
    float InputSmoothing = 10.0f;

    // ============================================================================
    // SPEED LIMITS
    // ============================================================================
    
    /**
     * MaxLinearSpeed - Maximum Ship Speed
     * 
     * Maximum linear velocity the ship can achieve (in cm/s).
     * 0 = unlimited speed (default for space physics)
     * 
     * This prevents ships from going too fast and becoming uncontrollable.
     * Useful for gameplay balance and performance.
     */
    UPROPERTY(EditAnywhere, Category = "Ship|Forces", meta = (ClampMin = "0.0"))
    float MaxLinearSpeed = 0.0f; // 0 disables clamping; unlimited by default

    /**
     * MaxAngularSpeed - Maximum Rotation Speed
     * 
     * Maximum angular velocity the ship can achieve (in rad/s).
     * Default: 6.0 rad/s (~343 degrees/second)
     * 
     * This prevents ships from spinning too fast and becoming disorienting.
     * Higher values = faster rotation
     * Lower values = slower rotation
     */
    UPROPERTY(EditAnywhere, Category = "Ship|Forces", meta = (ClampMin = "0.0"))
    float MaxAngularSpeed = 6.0f;   // rad/s (~343 deg/s)

    // ============================================================================
    // SPECIAL MANEUVERS
    // ============================================================================
    
    /**
     * OppositeRotationRateDegPerSec - Orient Opposite Rotation Speed
     * 
     * How fast the ship rotates when using the "Orient Opposite" maneuver.
     * This creates a smooth rotation to face the opposite direction.
     * 
     * Default: 90.0 degrees/second (~2 seconds for 180° rotation)
     * Higher values = faster rotation
     * Lower values = slower rotation
     */
    UPROPERTY(EditAnywhere, Category = "Ship|Forces", meta = (ClampMin = "0.0"))
    float OppositeRotationRateDegPerSec = 90.0f; // ~2s for 180°
};

/**
 * USHIP_BASICS - Ship Gameplay Logic Component
 * 
 * This is the core gameplay component that handles all ship physics and behavior.
 * It processes player input and converts it into physics forces and torques.
 * 
 * Key Responsibilities:
 * - Input processing (joystick, triggers, buttons)
 * - Physics force application (thrust, boost, rotation)
 * - Speed limiting and damping
 * - Special maneuvers (orient opposite)
 * - Thruster visualization data
 * 
 * This component is the "brain" of the ship, processing all player
 * input and converting it into the physics forces that move the ship.
 * 
 * Component References:
 * - ControlledBody: The physics body that receives forces
 * - VisualRoot: The visual component for effects and positioning
 * 
 * The component uses a two-reference system:
 * 1. Blueprint-friendly FComponentReference for editor assignment
 * 2. Runtime-resolved TObjectPtr for actual usage
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SPAAAAAACE_API USHIP_BASICS : public UActorComponent
{
    GENERATED_BODY()

public:
    /**
     * Constructor
     * 
     * Initializes the component with default settings and enables ticking.
     * The component needs to tick every frame to process input and apply forces.
     */
    USHIP_BASICS();

    // ============================================================================
    // COMPONENT REFERENCES (BLUEPRINT-FRIENDLY)
    // ============================================================================
    
    /**
     * ControlledBodyRef - Physics Body Reference (Blueprint)
     * 
     * Blueprint-friendly reference to the physics body component.
     * This allows designers to assign the physics body in the editor
     * without needing to know the exact component name.
     * 
     * The component will automatically resolve this to ControlledBody
     * at runtime, falling back to the actor's root component if not set.
     */
    UPROPERTY(EditAnywhere, Category = "Ship|Refs")
    FComponentReference ControlledBodyRef;

    /**
     * VisualRootRef - Visual Root Reference (Blueprint)
     * 
     * Blueprint-friendly reference to the visual root component.
     * This is used for visual effects and positioning calculations.
     * 
     * The component will automatically resolve this to VisualRoot
     * at runtime, falling back to the actor's root component if not set.
     */
    UPROPERTY(EditAnywhere, Category = "Ship|Refs")
    FComponentReference VisualRootRef;

    // ============================================================================
    // RESOLVED COMPONENT REFERENCES (RUNTIME)
    // ============================================================================
    
    /**
     * ControlledBody - Physics Body Component (Runtime)
     * 
     * The actual physics body component that receives forces and torques.
     * This is resolved from ControlledBodyRef at runtime and is used
     * for all physics operations.
     * 
     * Visible for debugging - shows which component is being controlled.
     */
    UPROPERTY(VisibleAnywhere, Category = "Ship|Refs")
    TObjectPtr<UPrimitiveComponent> ControlledBody;

    /**
     * VisualRoot - Visual Root Component (Runtime)
     * 
     * The visual root component used for effects and positioning.
     * This is resolved from VisualRootRef at runtime and is used
     * for visual calculations and thruster positioning.
     * 
     * Visible for debugging - shows which component is being used for visuals.
     */
    UPROPERTY(VisibleAnywhere, Category = "Ship|Refs")
    TObjectPtr<USceneComponent> VisualRoot;

    // ============================================================================
    // CONFIGURATION
    // ============================================================================
    
    /**
     * Settings - Ship Physics Configuration
     * 
     * Contains all tunable parameters for ship physics and behavior.
     * This allows designers to adjust ship feel without code changes.
     * 
     * See FShipForceSettings documentation for detailed parameter descriptions.
     */
    UPROPERTY(EditAnywhere, Category = "Ship|Config")
    FShipForceSettings Settings;

protected:
    /**
     * BeginPlay - Component Initialization
     * 
     * Called when the component is initialized. Resolves component references
     * and sets up the physics body for gameplay.
     */
    virtual void BeginPlay() override;
    
    /**
     * TickComponent - Main Update Loop
     * 
     * Called every frame to process input and apply physics forces.
     * This is where all the ship's gameplay logic happens.
     * 
     * @param DeltaTime - Time elapsed since last frame
     * @param TickType - Type of tick (not used)
     * @param ThisTickFunction - Tick function info (not used)
     */
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    // ============================================================================
    // INPUT SMOOTHING STATE
    // ============================================================================
    
    /**
     * SmoothedThrust - Smoothed Thrust Input
     * 
     * The current smoothed value of the thrust input (0.0 to 1.0).
     * This is interpolated from raw input to prevent jerky movement.
     */
    float SmoothedThrust = 0.f;
    
    /**
     * SmoothedLeft - Smoothed Left Stick Input
     * 
     * The current smoothed value of the left stick input.
     * X = Roll (left/right), Y = Pitch (up/down)
     * This is interpolated from raw input to prevent jerky movement.
     */
    FVector2D SmoothedLeft = FVector2D::ZeroVector;
    
    /**
     * SmoothedRight - Smoothed Right Stick Input
     * 
     * The current smoothed value of the right stick input.
     * X = Yaw (left/right), Y = unused
     * This is interpolated from raw input to prevent jerky movement.
     */
    FVector2D SmoothedRight = FVector2D::ZeroVector;

    // ============================================================================
    // WARNING STATE (PREVENT SPAM)
    // ============================================================================
    
    /**
     * Warning Flags - Prevent Log Spam
     * 
     * These flags prevent the same warning from being logged repeatedly.
     * Each warning is logged once per component instance to avoid log spam.
     */
    bool bWarnedNoBody = false;        // No physics body found
    bool bWarnedNoPhysics = false;    // Physics not enabled
    bool bWarnedNoController = false; // No input controller found

    // ============================================================================
    // INPUT PROCESSING UTILITIES
    // ============================================================================
    
    /**
     * Deadzone - Single-Axis Deadzone Filter
     * 
     * Applies deadzone filtering to a single input value.
     * Values below the deadzone threshold are set to zero.
     * 
     * @param V - Input value to filter
     * @param DZ - Deadzone threshold
     * @return Filtered input value
     */
    static float Deadzone(float V, float DZ) { return (FMath::Abs(V) < DZ) ? 0.f : V; }
    
    /**
     * Deadzone2D - Two-Axis Deadzone Filter
     * 
     * Applies deadzone filtering to a 2D input vector.
     * If the vector magnitude is below the deadzone threshold, returns zero vector.
     * 
     * @param V - Input vector to filter
     * @param DZ - Deadzone threshold
     * @return Filtered input vector
     */
    static FVector2D Deadzone2D(FVector2D V, float DZ)
    {
        const float M = V.Size();
        return (M < DZ) ? FVector2D::ZeroVector : V;
    }

    // ============================================================================
    // COMPONENT RESOLUTION
    // ============================================================================
    
    /**
     * ResolveBody - Find Physics Body Component
     * 
     * Attempts to find the physics body component for this actor.
     * First tries the ControlledBodyRef, then falls back to the actor's root component.
     * 
     * @return Pointer to physics body component, or nullptr if not found
     */
    UPrimitiveComponent* ResolveBody() const;
    
    /**
     * GetAgnosticController - Find Input Controller
     * 
     * Finds the AgnosticController for the current world.
     * This is used to get player input state.
     * 
     * @return Pointer to input controller, or nullptr if not found
     */
    const class AAgnosticController* GetAgnosticController() const;

    // ============================================================================
    // PHYSICS APPLICATION
    // ============================================================================
    
    /**
     * ApplyForcesAndTorques - Main Physics Application
     * 
     * Processes player input and applies the resulting forces and torques
     * to the physics body. This is the core of the ship's movement system.
     * 
     * @param DeltaTime - Time elapsed since last frame
     * @param Input - Current player input state
     * @param Body - Physics body to apply forces to
     */
    void ApplyForcesAndTorques(float DeltaTime, const struct FShipInputState& Input, UPrimitiveComponent* Body);
    
    /**
     * ClampSpeeds - Apply Speed Limits
     * 
     * Enforces maximum linear and angular speeds to prevent the ship
     * from going too fast and becoming uncontrollable.
     * 
     * @param Body - Physics body to clamp speeds for
     */
    void ClampSpeeds(UPrimitiveComponent* Body) const;

public:
    // ============================================================================
    // PUBLIC CONTROL FUNCTIONS
    // ============================================================================
    
    /**
     * ZeroAngularVelocity - Stop All Rotation
     * 
     * Immediately stops all angular velocity of the ship, bringing it to
     * a stable orientation. This is useful for:
     * - Emergency stabilization
     * - Resetting after collisions
     * - Player-requested orientation reset
     * 
     * This only affects rotation, not linear velocity.
     */
    void ZeroAngularVelocity();

private:
    // ============================================================================
    // ORIENT OPPOSITE STATE
    // ============================================================================
    
    /**
     * bOrientingOpposite - Orient Opposite Active State
     * 
     * True when the ship is currently performing an "orient opposite" maneuver.
     * This prevents manual input from interfering with the automatic rotation.
     */
    bool bOrientingOpposite = false;
    
    /**
     * TargetOppositeYawDeg - Orient Opposite Target
     * 
     * The target yaw angle for the orient opposite maneuver.
     * This is calculated based on the ship's current velocity direction.
     */
    float TargetOppositeYawDeg = 0.0f;

    // ============================================================================
    // THRUST SHAPING UTILITIES
    // ============================================================================
    
    /**
     * CosineSimilarity01 - Calculate Vector Alignment
     * 
     * Calculates how well two vectors are aligned (0.0 = opposite, 1.0 = same direction).
     * This is used for thrust scaling based on movement direction.
     * 
     * @param A - First vector
     * @param B - Second vector
     * @return Alignment value from 0.0 to 1.0
     */
    static float CosineSimilarity01(const FVector& A, const FVector& B)
    {
        const FVector NA = A.GetSafeNormal();
        const FVector NB = B.GetSafeNormal();
        const float Dot = FVector::DotProduct(NA, NB); // [-1,1]
        return FMath::Clamp((Dot * 0.5f) + 0.5f, 0.f, 1.f);
    }

    /**
     * MapAlignmentToThrustScale - Calculate Thrust Scaling
     * 
     * Maps vector alignment to thrust scaling factor.
     * This creates realistic thrust behavior where thrust is more effective
     * when aligned with the ship's movement direction.
     * 
     * @param Cos01 - Alignment value (0.0 to 1.0)
     * @param OppositeScale - Scale when moving opposite to thrust (default 0.25)
     * @param ForwardScale - Scale when moving with thrust (default 1.0)
     * @param BiasExp - Bias exponent for scaling curve (default 1.5)
     * @return Thrust scaling factor
     */
    static float MapAlignmentToThrustScale(float Cos01, float OppositeScale = 0.25f, float ForwardScale = 1.0f, float BiasExp = 1.5f)
    {
        const float T = FMath::Pow(Cos01, BiasExp); // 0..1, biased toward alignment
        return FMath::Lerp(OppositeScale, ForwardScale, T);
    }

    /**
     * ComputeLocalThrusterWeights - Calculate Thruster Visualization
     * 
     * Calculates how much each thruster should be firing based on the desired force.
     * This is used for visual effects and thruster animations.
     * 
     * @param WorldXform - World transform of the ship
     * @param DesiredForce - Desired force in world space
     * @param OutForward - Forward thruster weight (0.0 to 1.0)
     * @param OutBackward - Backward thruster weight (0.0 to 1.0)
     * @param OutRight - Right thruster weight (0.0 to 1.0)
     * @param OutLeft - Left thruster weight (0.0 to 1.0)
     * @param OutUp - Up thruster weight (0.0 to 1.0)
     * @param OutDown - Down thruster weight (0.0 to 1.0)
     */
    static void ComputeLocalThrusterWeights(const FTransform& WorldXform, const FVector& DesiredForce,
        float& OutForward, float& OutBackward, float& OutRight, float& OutLeft, float& OutUp, float& OutDown)
    {
        const FVector LocalDir = WorldXform.InverseTransformVectorNoScale(DesiredForce).GetSafeNormal();
        OutForward  = FMath::Clamp(LocalDir.X, 0.f, 1.f);
        OutBackward = FMath::Clamp(-LocalDir.X, 0.f, 1.f);
        OutRight    = FMath::Clamp(LocalDir.Y, 0.f, 1.f);
        OutLeft     = FMath::Clamp(-LocalDir.Y, 0.f, 1.f);
        OutUp       = FMath::Clamp(LocalDir.Z, 0.f, 1.f);
        OutDown     = FMath::Clamp(-LocalDir.Z, 0.f, 1.f);
    }
};
