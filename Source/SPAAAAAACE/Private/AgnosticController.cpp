/**
 * AgnosticController Implementation
 * 
 * This file contains the implementation of the input controller that processes
 * player input and converts it into ship control commands.
 * 
 * Key Systems:
 * - Enhanced Input system integration
 * - Input asset loading (Blueprint and config-based)
 * - Input action callback handling
 * - Input state management
 * - Camera control integration
 * 
 * The controller uses Unreal's Enhanced Input system to provide flexible,
 * device-agnostic input handling for the space ship.
 */

#include "AgnosticController.h"

// Enhanced Input system includes
#include "EnhancedInputComponent.h"    // Enhanced input component
#include "EnhancedInputSubsystems.h"   // Input subsystem management
#include "InputAction.h"               // Input action definitions
#include "InputActionValue.h"          // Input value handling
#include "InputMappingContext.h"       // Input mapping context

// Core engine includes
#include "Camera/CameraComponent.h"    // Camera component access
#include "Engine/AssetManager.h"       // Asset loading
#include "Engine/StreamableManager.h"  // Asset streaming
#include "Engine/Engine.h"             // Engine utilities

// Game-specific includes
#include "ShipPawn.h"                  // Ship pawn access

/**
 * Log Category Definition
 * 
 * Creates a dedicated log category for AgnosticController messages.
 * This allows filtering log output to only show input controller messages
 * when debugging. Use UE_LOG(LogAgnosticController, Warning, TEXT("Message")) to log.
 */
DEFINE_LOG_CATEGORY_STATIC(LogAgnosticController, Log, All);

/**
 * BeginPlay - Controller Initialization
 * 
 * Called when the controller is initialized. This function:
 * - Attempts to load input assets from configuration
 * - Sets up the Enhanced Input system
 * - Logs controller status for debugging
 * 
 * The controller supports both Blueprint-assigned input assets and
 * configuration-based loading for flexibility.
 */
void AAgnosticController::BeginPlay()
{
    Super::BeginPlay();

    // ============================================================================
    // INPUT ASSET LOADING
    // ============================================================================
    
    /**
     * Load Input Assets from Configuration
     * 
     * Attempts to load input assets from configuration paths if Blueprint
     * assignments are not available. This provides fallback loading capability
     * for scenarios where Blueprint setup is not desired or possible.
     */
    TryLoadInputAssetsFromConfig();

    // ============================================================================
    // DEBUG LOGGING
    // ============================================================================
    
    /**
     * Log Controller Status
     * 
     * Logs the current state of the input mapping context for debugging.
     * This helps identify setup issues during development.
     */
    UE_LOG(LogAgnosticController, Log, TEXT("BeginPlay: Controller active. MappingContext %s"), 
        MappingContext ? TEXT("SET") : TEXT("NOT SET"));

    // ============================================================================
    // ENHANCED INPUT SETUP
    // ============================================================================
    
    /**
     * Add Input Mapping Context
     * 
     * Attempts to add the input mapping context to the Enhanced Input system.
     * This is required for input actions to be processed.
     */
    TryAddMappingContext();
}

void AAgnosticController::TryLoadInputAssetsFromConfig()
{
    FStreamableManager& Streamable = UAssetManager::GetStreamableManager();

    if (!MappingContext && MappingContextPath.IsValid())
    {
        UObject* Obj = Streamable.LoadSynchronous(MappingContextPath);
        MappingContext = Cast<UInputMappingContext>(Obj);
        if (MappingContext)
        {
            UE_LOG(LogAgnosticController, Log, TEXT("Loaded MappingContext from config: %s"), *MappingContext->GetName());
        }
        else if (Obj)
        {
            UE_LOG(LogAgnosticController, Warning, TEXT("Config MappingContextPath resolved to %s but cast failed."), *Obj->GetName());
        }
    }

    auto LoadAction = [&](TObjectPtr<UInputAction>& Target, const FSoftObjectPath& Path, const TCHAR* Label)
    {
        if (!Target && Path.IsValid())
        {
            UObject* Obj = Streamable.LoadSynchronous(Path);
            Target = Cast<UInputAction>(Obj);
            if (Target)
            {
                UE_LOG(LogAgnosticController, Log, TEXT("Loaded %s from config: %s"), Label, *Target->GetName());
            }
            else if (Obj)
            {
                UE_LOG(LogAgnosticController, Warning, TEXT("Config %s path resolved to %s but cast failed."), Label, *Obj->GetName());
            }
        }
    };

    LoadAction(IA_LeftStick, IA_LeftStickPath, TEXT("IA_LeftStick"));
    LoadAction(IA_RightStick, IA_RightStickPath, TEXT("IA_RightStick"));
    LoadAction(IA_Thrust, IA_ThrustPath, TEXT("IA_Thrust"));
    LoadAction(IA_Boost, IA_BoostPath, TEXT("IA_Boost"));
    LoadAction(IA_OrientOpposite, IA_OrientOppositePath, TEXT("IA_OrientOpposite"));
    LoadAction(IA_CameraToggle, IA_CameraTogglePath, TEXT("IA_CameraToggle"));
    LoadAction(IA_ZeroRotation, IA_ZeroRotationPath, TEXT("IA_ZeroRotation"));
    LoadAction(IA_CameraTrack, IA_CameraTrackPath, TEXT("IA_CameraTrack"));
}

void AAgnosticController::TryAddMappingContext()
{
    if (ULocalPlayer* LP = GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            if (MappingContext)
            {
                Subsys->AddMappingContext(MappingContext, /*Priority=*/1);
                UE_LOG(LogAgnosticController, Log, TEXT("Added Input Mapping Context: %s"), *MappingContext->GetName());
            }
            else
            {
                UE_LOG(LogAgnosticController, Warning, TEXT("No MappingContext assigned. Inputs will not be bound to IMC."));
            }
        }
    }
}

void AAgnosticController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    if (InPawn)
    {
        SetViewTargetWithBlend(InPawn, 0.0f);
        UE_LOG(LogAgnosticController, Log, TEXT("OnPossess: View target set to pawn %s"), *InPawn->GetName());
        
        // Debug: Log the pawn's camera component
        if (UCameraComponent* FoundCamera = InPawn->FindComponentByClass<UCameraComponent>())
        {
            UE_LOG(LogAgnosticController, Log, TEXT("Found camera component: %s, Active: %s"), 
                *FoundCamera->GetName(), FoundCamera->IsActive() ? TEXT("YES") : TEXT("NO"));
        }
    }
}

void AAgnosticController::SetupInputComponent()
{
    Super::SetupInputComponent();

    // Ensure assets are loaded from config before binding, and mapping context is added
    TryLoadInputAssetsFromConfig();
    TryAddMappingContext();

    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
    {
        UE_LOG(LogAgnosticController, Log, TEXT("SetupInputComponent: EnhancedInputComponent ready."));
        if (IA_LeftStick)
        {
            EIC->BindAction(IA_LeftStick, ETriggerEvent::Triggered, this, &AAgnosticController::OnLeftStick);
            EIC->BindAction(IA_LeftStick, ETriggerEvent::Completed, this, &AAgnosticController::OnLeftStickComplete);
            UE_LOG(LogAgnosticController, Log, TEXT("Bound IA_LeftStick: %s"), *IA_LeftStick->GetName());
        }
        else { UE_LOG(LogAgnosticController, Warning, TEXT("IA_LeftStick is NOT set.")); }
        if (IA_RightStick)
        {
            EIC->BindAction(IA_RightStick, ETriggerEvent::Triggered, this, &AAgnosticController::OnRightStick);
            EIC->BindAction(IA_RightStick, ETriggerEvent::Completed, this, &AAgnosticController::OnRightStickComplete);
            UE_LOG(LogAgnosticController, Log, TEXT("Bound IA_RightStick: %s"), *IA_RightStick->GetName());
        }
        else { UE_LOG(LogAgnosticController, Warning, TEXT("IA_RightStick is NOT set.")); }
        if (IA_Thrust)
        {
            EIC->BindAction(IA_Thrust, ETriggerEvent::Triggered, this, &AAgnosticController::OnThrust);
            EIC->BindAction(IA_Thrust, ETriggerEvent::Completed, this, &AAgnosticController::OnThrustComplete);
            UE_LOG(LogAgnosticController, Log, TEXT("Bound IA_Thrust: %s"), *IA_Thrust->GetName());
        }
        else { UE_LOG(LogAgnosticController, Warning, TEXT("IA_Thrust is NOT set.")); }
        if (IA_Boost)
        {
            // Axis1D percentage, continuous while pressed; we log on threshold crossings only
            EIC->BindAction(IA_Boost, ETriggerEvent::Triggered, this, &AAgnosticController::OnBoost);
            EIC->BindAction(IA_Boost, ETriggerEvent::Completed, this, &AAgnosticController::OnBoostComplete);
            UE_LOG(LogAgnosticController, Log, TEXT("Bound IA_Boost: %s"), *IA_Boost->GetName());
        }
        else { UE_LOG(LogAgnosticController, Warning, TEXT("IA_Boost is NOT set.")); }
        if (IA_OrientOpposite)
        {
            // Use Started so it fires once on press (no repeat while held)
            EIC->BindAction(IA_OrientOpposite, ETriggerEvent::Started, this, &AAgnosticController::OnOrientOppositeStarted);
            EIC->BindAction(IA_OrientOpposite, ETriggerEvent::Completed, this, &AAgnosticController::OnOrientOppositeCompleted);
            UE_LOG(LogAgnosticController, Log, TEXT("Bound IA_OrientOpposite: %s"), *IA_OrientOpposite->GetName());
        }
        else { UE_LOG(LogAgnosticController, Warning, TEXT("IA_OrientOpposite is NOT set.")); }

        if (IA_CameraToggle)
        {
            EIC->BindAction(IA_CameraToggle, ETriggerEvent::Started, this, &AAgnosticController::OnCameraToggle);
            UE_LOG(LogAgnosticController, Log, TEXT("Bound IA_CameraToggle: %s"), *IA_CameraToggle->GetName());
        }
        else { UE_LOG(LogAgnosticController, Warning, TEXT("IA_CameraToggle is NOT set.")); }

        if (IA_ZeroRotation)
        {
            EIC->BindAction(IA_ZeroRotation, ETriggerEvent::Started, this, &AAgnosticController::OnZeroRotation);
            UE_LOG(LogAgnosticController, Log, TEXT("Bound IA_ZeroRotation: %s"), *IA_ZeroRotation->GetName());
        }
        else { UE_LOG(LogAgnosticController, Warning, TEXT("IA_ZeroRotation is NOT set.")); }

        if (IA_CameraTrack)
        {
            EIC->BindAction(IA_CameraTrack, ETriggerEvent::Started, this, &AAgnosticController::OnCameraTrackStarted);
            EIC->BindAction(IA_CameraTrack, ETriggerEvent::Completed, this, &AAgnosticController::OnCameraTrackCompleted);
            UE_LOG(LogAgnosticController, Log, TEXT("Bound IA_CameraTrack: %s"), *IA_CameraTrack->GetName());
        }
        else { UE_LOG(LogAgnosticController, Warning, TEXT("IA_CameraTrack is NOT set.")); }
    }
    else
    {
        UE_LOG(LogAgnosticController, Warning, TEXT("SetupInputComponent: InputComponent is not EnhancedInputComponent."));
    }
}

// === Callbacks ===
void AAgnosticController::OnLeftStick(const FInputActionValue& Value)
{
    FVector2D V = FVector2D::ZeroVector;
    const EInputActionValueType VT = Value.GetValueType();
    if (VT == EInputActionValueType::Axis2D)
    {
        V = Value.Get<FVector2D>();
    }
    else if (VT == EInputActionValueType::Axis1D)
    {
        V.X = Value.Get<float>();
    }
    else if (VT == EInputActionValueType::Boolean)
    {
        V.X = Value.Get<bool>() ? 1.f : 0.f;
    }
    InputState.LeftStick = V;
}
void AAgnosticController::OnLeftStickComplete(const FInputActionValue&)
{
    InputState.LeftStick = FVector2D::ZeroVector;
}

void AAgnosticController::OnRightStick(const FInputActionValue& Value)
{
    // Simplified to avoid crash - just set the value without logging for now
    FVector2D V = FVector2D::ZeroVector;
    const EInputActionValueType VT = Value.GetValueType();
    if (VT == EInputActionValueType::Axis2D)
    {
        V = Value.Get<FVector2D>();
    }
    else if (VT == EInputActionValueType::Axis1D)
    {
        V.X = Value.Get<float>();
    }
    else if (VT == EInputActionValueType::Boolean)
    {
        V.X = Value.Get<bool>() ? 1.f : 0.f;
    }
    InputState.RightStick = V;
}
void AAgnosticController::OnRightStickComplete(const FInputActionValue&)
{
    InputState.RightStick = FVector2D::ZeroVector;
}

void AAgnosticController::OnThrust(const FInputActionValue& Value)
{
    InputState.Thrust = FMath::Clamp(Value.Get<float>(), 0.f, 1.f);
    UE_LOG(LogAgnosticController, Log, TEXT("Thrust Triggered: %.2f"), InputState.Thrust);
}
void AAgnosticController::OnThrustComplete(const FInputActionValue&)
{
    InputState.Thrust = 0.f;
    UE_LOG(LogAgnosticController, Log, TEXT("Thrust Completed: 0.00"));
}

void AAgnosticController::OnBoost(const FInputActionValue& Value)
{
    const float Prev = InputState.Boost;
    
    if (Value.GetValueType() == EInputActionValueType::Axis1D)
    {
        InputState.Boost = FMath::Clamp(Value.Get<float>(), 0.f, 1.f);
    }
    else if (Value.GetValueType() == EInputActionValueType::Boolean)
    {
        InputState.Boost = Value.Get<bool>() ? 1.f : 0.f;
    }
    else
    {
        InputState.Boost = 0.f;
    }
    
    // Simple threshold logging without dangerous string formatting
    constexpr float OnThreshold = 0.05f;
    constexpr float OffThreshold = 0.03f;
    if (Prev < OnThreshold && InputState.Boost >= OnThreshold)
    {
        UE_LOG(LogAgnosticController, Log, TEXT("Boost: ON"));
    }
    else if (Prev >= OffThreshold && InputState.Boost < OffThreshold)
    {
        UE_LOG(LogAgnosticController, Log, TEXT("Boost: OFF"));
    }
}
void AAgnosticController::OnBoostComplete(const FInputActionValue&)
{
    const float Prev = InputState.Boost;
    InputState.Boost = 0.f;
    if (Prev > 0.f)
    {
        UE_LOG(LogAgnosticController, Log, TEXT("Boost: OFF"));
    }
}

void AAgnosticController::OnOrientOppositeStarted(const FInputActionValue&)
{
    InputState.bOrientOpposite = true;
    UE_LOG(LogAgnosticController, Log, TEXT("OrientOpposite Started"));
}

void AAgnosticController::OnOrientOppositeCompleted(const FInputActionValue&)
{
    InputState.bOrientOpposite = false;
    UE_LOG(LogAgnosticController, Log, TEXT("OrientOpposite Completed"));
}

void AAgnosticController::OnCameraToggle(const FInputActionValue&)
{
    APawn* ControlledPawn = GetPawn();
    if (!IsValid(ControlledPawn)) { return; }

    AShipPawn* Ship = Cast<AShipPawn>(ControlledPawn);
    if (!IsValid(Ship)) { return; }

    Ship->ToggleCameraMode();
    if (GEngine)
    {
        const TCHAR* ModeName = (Ship->CameraMode == ECameraMode::Chase) ? TEXT("Chase") : TEXT("Nose");
        GEngine->AddOnScreenDebugMessage(-1, 1.2f, FColor::White, FString::Printf(TEXT("CAM: %s"), ModeName));
    }
}

void AAgnosticController::OnZeroRotation(const FInputActionValue&)
{
    APawn* ControlledPawn = GetPawn();
    if (!IsValid(ControlledPawn)) { return; }

    AShipPawn* Ship = Cast<AShipPawn>(ControlledPawn);
    if (!IsValid(Ship)) { return; }

    Ship->ZeroShipRotation();
    UE_LOG(LogAgnosticController, Log, TEXT("Zero rotation requested"));
    if (GEngine) { GEngine->AddOnScreenDebugMessage(-1, 1.2f, FColor::Yellow, TEXT("ZERO ROTATION")); }
}

void AAgnosticController::OnCameraTrackStarted(const FInputActionValue&)
{
    bCameraTrackHeld = true;
    UE_LOG(LogAgnosticController, Log, TEXT("CameraTrack: Started"));
}

void AAgnosticController::OnCameraTrackCompleted(const FInputActionValue&)
{
    bCameraTrackHeld = false;
    UE_LOG(LogAgnosticController, Log, TEXT("CameraTrack: Completed"));
}

void AAgnosticController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    // Always keep the view target on the possessed pawn
    if (APawn* P = GetPawn())
    {
        if (GetViewTarget() != P)
        {
            SetViewTargetWithBlend(P, 0.0f);
        }
    }
    
    // Removed debug view target change logging
}
