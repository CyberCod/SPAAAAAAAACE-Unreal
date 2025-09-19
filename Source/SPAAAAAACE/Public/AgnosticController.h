/**
 * AgnosticController - Input Processing System
 * 
 * This file defines the input controller that processes player input
 * and converts it into ship control commands.
 * 
 * Key Responsibilities:
 * - Enhanced Input system integration
 * - Input state management and smoothing
 * - Input action callbacks and processing
 * - Configuration-based input asset loading
 * - Camera control integration
 * 
 * The controller uses Unreal's Enhanced Input system to provide
 * flexible, device-agnostic input handling for the space ship.
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AgnosticController.generated.h"

// Forward declarations for Enhanced Input system
class UInputMappingContext;  // Input mapping configuration
class UInputAction;          // Individual input actions
struct FInputActionValue;    // Input value data

/**
 * FShipInputState - Ship Input State Structure
 * 
 * Contains the current state of all ship input controls.
 * This structure is updated every frame with the latest input values
 * and is used by the ship's physics system to determine movement.
 * 
 * The structure uses normalized values (0.0 to 1.0) for consistency
 * across different input devices and provides a clean interface
 * between input processing and ship physics.
 */
USTRUCT()
struct FShipInputState
{
    GENERATED_BODY()

    /**
     * LeftStick - Left Analog Stick Input
     * 
     * X = Roll (left/right rotation)
     * Y = Pitch (up/down rotation)
     * 
     * Values range from -1.0 to 1.0 for each axis.
     * This controls the ship's rotational movement.
     */
    UPROPERTY() FVector2D LeftStick = FVector2D::ZeroVector;
    
    /**
     * RightStick - Right Analog Stick Input
     * 
     * X = Yaw (left/right rotation)
     * Y = Unused (reserved for future features)
     * 
     * Values range from -1.0 to 1.0 for each axis.
     * This controls the ship's horizontal rotation.
     */
    UPROPERTY() FVector2D RightStick = FVector2D::ZeroVector;
    
    /**
     * Thrust - Main Engine Thrust Input
     * 
     * Controls the main engine thrust power.
     * Values range from 0.0 (no thrust) to 1.0 (full thrust).
     * This is typically controlled by triggers or throttle.
     */
    UPROPERTY() float Thrust = 0.f;
    
    /**
     * Boost - Boost Engine Input
     * 
     * Controls the boost engine power for maximum acceleration.
     * Values range from 0.0 (no boost) to 1.0 (full boost).
     * This is typically controlled by a separate trigger or button.
     */
    UPROPERTY() float Boost = 0.f;
    
    /**
     * bOrientOpposite - Orient Opposite Maneuver
     * 
     * Edge-triggered input for the "orient opposite" maneuver.
     * When true, the ship will rotate to face the opposite direction
     * of its current velocity. This is useful for quick direction changes.
     * 
     * This is a boolean flag that gets consumed after being read.
     */
    UPROPERTY() bool bOrientOpposite = false;
};

/**
 * AAgnosticController - Enhanced Input Controller
 * 
 * This is the main input controller that processes player input and converts
 * it into ship control commands. It uses Unreal's Enhanced Input system
 * to provide flexible, device-agnostic input handling.
 * 
 * Key Features:
 * - Enhanced Input system integration
 * - Input state management and smoothing
 * - Configuration-based input asset loading
 * - Camera control integration
 * - Device-agnostic input handling
 * 
 * The controller supports both Blueprint assignment and configuration-based
 * loading of input assets, making it flexible for different setups.
 */
UCLASS(config=Game)
class SPAAAAAACE_API AAgnosticController : public APlayerController
{
    GENERATED_BODY()

public:
    // ============================================================================
    // INPUT ACTION ASSIGNMENTS (BLUEPRINT-FRIENDLY)
    // ============================================================================
    
    /**
     * MappingContext - Input Mapping Context
     * 
     * The main input mapping context that defines how input devices
     * map to input actions. This is the core of the Enhanced Input system.
     * 
     * Assign this in the Blueprint defaults to set up input mappings.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Ship|Input")
    TObjectPtr<UInputMappingContext> MappingContext;

    /**
     * IA_LeftStick - Left Stick Input Action
     * 
     * Handles left analog stick input for ship rotation.
     * X = Roll (left/right), Y = Pitch (up/down)
     */
    UPROPERTY(EditDefaultsOnly, Category = "Ship|Input")
    TObjectPtr<UInputAction> IA_LeftStick;

    /**
     * IA_RightStick - Right Stick Input Action
     * 
     * Handles right analog stick input for ship rotation.
     * X = Yaw (left/right), Y = Unused
     */
    UPROPERTY(EditDefaultsOnly, Category = "Ship|Input")
    TObjectPtr<UInputAction> IA_RightStick;

    /**
     * IA_Thrust - Thrust Input Action
     * 
     * Handles main engine thrust input.
     * Typically controlled by triggers or throttle controls.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Ship|Input")
    TObjectPtr<UInputAction> IA_Thrust;

    /**
     * IA_Boost - Boost Input Action
     * 
     * Handles boost engine input for maximum acceleration.
     * Typically controlled by a separate trigger or button.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Ship|Input")
    TObjectPtr<UInputAction> IA_Boost;

    /**
     * IA_OrientOpposite - Orient Opposite Input Action
     * 
     * Handles the "orient opposite" maneuver input.
     * This is an edge-triggered action for quick direction changes.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Ship|Input")
    TObjectPtr<UInputAction> IA_OrientOpposite;

    // ============================================================================
    // CAMERA CONTROL INPUT ACTIONS
    // ============================================================================
    
    /**
     * IA_CameraToggle - Camera Mode Toggle
     * 
     * Toggles between chase and nose camera modes.
     * This allows players to switch between third-person and first-person views.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Ship|Input")
    TObjectPtr<UInputAction> IA_CameraToggle;

    /**
     * IA_ZeroRotation - Zero Rotation Input
     * 
     * Immediately stops all ship rotation.
     * This is useful for emergency stabilization or orientation reset.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Ship|Input")
    TObjectPtr<UInputAction> IA_ZeroRotation;

    /**
     * IA_CameraTrack - Camera Tracking Input
     * 
     * Hold-to-track camera input that makes the camera smoothly rotate
     * to face the ship's forward direction. This is only active in chase mode.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Ship|Input")
    TObjectPtr<UInputAction> IA_CameraTrack;

    // ============================================================================
    // CONFIGURATION-BASED INPUT ASSET LOADING
    // ============================================================================
    
    /**
     * Configuration-Based Input Asset Paths
     * 
     * These properties allow setting input assets via configuration files
     * instead of Blueprint assignment. This is useful for:
     * - Version control (no Blueprint dependencies)
     * - Automated testing
     * - Different builds with different input schemes
     * - Runtime input asset swapping
     * 
     * The controller will attempt to load these assets at runtime if
     * the Blueprint-assigned assets are not available.
     */
    
    /**
     * MappingContextPath - Input Mapping Context Asset Path
     * 
     * Path to the input mapping context asset that defines how input devices
     * map to input actions. This is the core of the Enhanced Input system.
     */
    UPROPERTY(Config, EditAnywhere, Category = "Ship|Input|Config", meta=(AllowedClasses="/Script/EnhancedInput.InputMappingContext"))
    FSoftObjectPath MappingContextPath;

    /**
     * Input Action Asset Paths
     * 
     * Paths to individual input action assets. These define the specific
     * input behaviors for each control.
     */
    UPROPERTY(Config, EditAnywhere, Category = "Ship|Input|Config", meta=(AllowedClasses="/Script/EnhancedInput.InputAction"))
    FSoftObjectPath IA_LeftStickPath;

    UPROPERTY(Config, EditAnywhere, Category = "Ship|Input|Config", meta=(AllowedClasses="/Script/EnhancedInput.InputAction"))
    FSoftObjectPath IA_RightStickPath;

    UPROPERTY(Config, EditAnywhere, Category = "Ship|Input|Config", meta=(AllowedClasses="/Script/EnhancedInput.InputAction"))
    FSoftObjectPath IA_ThrustPath;

    UPROPERTY(Config, EditAnywhere, Category = "Ship|Input|Config", meta=(AllowedClasses="/Script/EnhancedInput.InputAction"))
    FSoftObjectPath IA_BoostPath;

    UPROPERTY(Config, EditAnywhere, Category = "Ship|Input|Config", meta=(AllowedClasses="/Script/EnhancedInput.InputAction"))
    FSoftObjectPath IA_OrientOppositePath;

    UPROPERTY(Config, EditAnywhere, Category = "Ship|Input|Config", meta=(AllowedClasses="/Script/EnhancedInput.InputAction"))
    FSoftObjectPath IA_CameraTogglePath;

    UPROPERTY(Config, EditAnywhere, Category = "Ship|Input|Config", meta=(AllowedClasses="/Script/EnhancedInput.InputAction"))
    FSoftObjectPath IA_ZeroRotationPath;

    UPROPERTY(Config, EditAnywhere, Category = "Ship|Input|Config", meta=(AllowedClasses="/Script/EnhancedInput.InputAction"))
    FSoftObjectPath IA_CameraTrackPath;

    // ============================================================================
    // PUBLIC INTERFACE
    // ============================================================================
    
    /**
     * GetShipInputState - Get Current Input State
     * 
     * Returns the current state of all ship input controls.
     * This is used by the ship's physics system to determine movement.
     * 
     * @return Reference to the current input state
     */
    const FShipInputState& GetShipInputState() const { return InputState; }
    
    /**
     * IsCameraTrackHeld - Check Camera Tracking State
     * 
     * Returns whether the camera tracking input is currently being held.
     * This is used by the ship's camera system to determine if tracking
     * should be active.
     * 
     * @return True if camera tracking input is held, false otherwise
     */
    bool IsCameraTrackHeld() const { return bCameraTrackHeld; }
    
    /**
     * ConsumeOrientOpposite - Consume Orient Opposite Input
     * 
     * Returns the current state of the orient opposite input and clears it.
     * This is an edge-triggered input that gets consumed after being read.
     * 
     * @return True if orient opposite was triggered, false otherwise
     */
    bool ConsumeOrientOpposite()
    {
        const bool Was = InputState.bOrientOpposite;
        InputState.bOrientOpposite = false;
        return Was;
    }

protected:
    /**
     * BeginPlay - Controller Initialization
     * 
     * Called when the controller is initialized. Attempts to load input assets
     * from configuration if Blueprint assignments are not available.
     */
    virtual void BeginPlay() override;
    
    /**
     * OnPossess - Pawn Possession Handler
     * 
     * Called when this controller possesses a pawn. Sets up input mappings
     * and ensures the input system is properly configured.
     * 
     * @param InPawn - The pawn being possessed
     */
    virtual void OnPossess(APawn* InPawn) override;
    
    /**
     * SetupInputComponent - Input System Setup
     * 
     * Sets up the Enhanced Input system and binds input actions to callbacks.
     * This is where all input handling is configured.
     */
    virtual void SetupInputComponent() override;
    
    /**
     * Tick - Controller Update Loop
     * 
     * Called every frame to update controller state and handle input processing.
     * 
     * @param DeltaTime - Time elapsed since last frame
     */
    virtual void Tick(float DeltaTime) override;

private:
    // ============================================================================
    // INPUT STATE MANAGEMENT
    // ============================================================================
    
    /**
     * InputState - Current Input State
     * 
     * Contains the current state of all ship input controls.
     * This is updated every frame with the latest input values.
     */
    FShipInputState InputState;
    
    /**
     * bCameraTrackHeld - Camera Tracking State
     * 
     * Tracks whether the camera tracking input is currently being held.
     * This is used to determine if camera tracking should be active.
     */
    bool bCameraTrackHeld = false;
    
    /**
     * LastBoostLogged - Boost Logging Timer
     * 
     * Used to prevent spam logging of boost input values.
     * This helps keep the log output clean during debugging.
     */
    float LastBoostLogged = 0.0f;

    // ============================================================================
    // INPUT ASSET LOADING
    // ============================================================================
    
    /**
     * TryLoadInputAssetsFromConfig - Load Input Assets from Configuration
     * 
     * Attempts to load input assets from configuration paths if Blueprint
     * assignments are not available. This provides fallback loading capability.
     */
    void TryLoadInputAssetsFromConfig();
    
    /**
     * TryAddMappingContext - Add Input Mapping Context
     * 
     * Attempts to add the input mapping context to the Enhanced Input system.
     * This is required for input actions to be processed.
     */
    void TryAddMappingContext();

    // ============================================================================
    // INPUT ACTION CALLBACKS
    // ============================================================================
    
    /**
     * Input Action Callbacks
     * 
     * These functions are called by the Enhanced Input system when
     * input actions are triggered. They update the input state and
     * handle special input behaviors.
     * 
     * Each input action has multiple callback types:
     * - Started: When input begins
     * - Ongoing: While input is active (for continuous inputs)
     * - Completed: When input ends
     * 
     * This allows for different behaviors based on input timing.
     */
    
    // Left stick callbacks (Roll/Pitch)
    void OnLeftStick(const FInputActionValue& Value);
    void OnLeftStickComplete(const FInputActionValue& Value);

    // Right stick callbacks (Yaw)
    void OnRightStick(const FInputActionValue& Value);
    void OnRightStickComplete(const FInputActionValue& Value);

    // Thrust callbacks (Main engine)
    void OnThrust(const FInputActionValue& Value);
    void OnThrustComplete(const FInputActionValue& Value);

    // Boost callbacks (Boost engine)
    void OnBoost(const FInputActionValue& Value);
    void OnBoostComplete(const FInputActionValue& Value);

    // Orient opposite callbacks (Special maneuver)
    void OnOrientOppositeStarted(const FInputActionValue& Value);
    void OnOrientOppositeCompleted(const FInputActionValue& Value);

    // Camera control callbacks
    void OnCameraToggle(const FInputActionValue& Value);
    void OnZeroRotation(const FInputActionValue& Value);
    void OnCameraTrackStarted(const FInputActionValue& Value);
    void OnCameraTrackCompleted(const FInputActionValue& Value);
};
