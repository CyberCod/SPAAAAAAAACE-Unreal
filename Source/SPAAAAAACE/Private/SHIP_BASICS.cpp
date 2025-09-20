/**
 * SHIP_BASICS Implementation
 * 
 * This file contains the implementation of the ship's core gameplay logic.
 * It handles input processing, physics force application, and ship behavior.
 * 
 * Key Systems:
 * - Input processing and smoothing
 * - Physics force and torque application
 * - Speed limiting and damping
 * - Special maneuvers (orient opposite)
 * - Thruster visualization calculations
 * 
 * The component processes player input every frame and converts it into
 * physics forces that move the ship through space.
 */

#include "SHIP_BASICS.h"

// Core Unreal Engine includes
#include "GameFramework/Actor.h"           // For actor ownership
#include "GameFramework/PlayerController.h" // For input controller access
#include "Components/PrimitiveComponent.h"  // For physics body operations
#include "Engine/World.h"                  // For world access

// Game-specific includes
#include "AgnosticController.h"             // For input state access

/**
 * Log Category Definition
 * 
 * Creates a dedicated log category for SHIP_BASICS messages.
 * This allows filtering log output to only show ship physics messages
 * when debugging. Use UE_LOG(LogShipBasics, Warning, TEXT("Message")) to log.
 */
DEFINE_LOG_CATEGORY_STATIC(LogShipBasics, Log, All);

/**
 * USHIP_BASICS Constructor
 * 
 * Initializes the component with default settings and enables ticking.
 * The component needs to tick every frame to process input and apply forces.
 */
USHIP_BASICS::USHIP_BASICS()
{
    // Enable component ticking - this component needs to update every frame
    // to process input and apply physics forces
    PrimaryComponentTick.bCanEverTick = true;
}

/**
 * BeginPlay - Component Initialization
 * 
 * Called when the component is initialized. This function:
 * - Resolves component references from Blueprint assignments
 * - Falls back to automatic component detection if references aren't set
 * - Logs component status for debugging
 * - Validates physics setup
 * 
 * The component uses a two-tier reference system:
 * 1. Blueprint-friendly FComponentReference for editor assignment
 * 2. Runtime-resolved TObjectPtr for actual usage
 */
void USHIP_BASICS::BeginPlay()
{
    Super::BeginPlay();

    // ============================================================================
    // COMPONENT REFERENCE RESOLUTION
    // ============================================================================
    
    /**
     * Resolve Physics Body Reference
     * 
     * First tries to resolve the ControlledBodyRef (Blueprint assignment),
     * then falls back to automatic detection if not set.
     */
    if (!ControlledBody)
    {
        if (UActorComponent* Comp = ControlledBodyRef.GetComponent(GetOwner()))
        {
            ControlledBody = Cast<UPrimitiveComponent>(Comp);
        }
    }
    
    /**
     * Resolve Visual Root Reference
     * 
     * First tries to resolve the VisualRootRef (Blueprint assignment),
     * then falls back to automatic detection if not set.
     */
    if (!VisualRoot)
    {
        if (UActorComponent* Comp = VisualRootRef.GetComponent(GetOwner()))
        {
            VisualRoot = Cast<USceneComponent>(Comp);
        }
    }

    /**
     * Fallback Physics Body Detection
     * 
     * If no physics body was found through Blueprint references,
     * attempt to find one automatically by searching the actor's components.
     */
    if (!ControlledBody)
    {
        ControlledBody = ResolveBody();
    }

    // ============================================================================
    // DEBUG LOGGING
    // ============================================================================
    
    /**
     * Log Component Status
     * 
     * Logs the current state of component references for debugging.
     * This helps identify setup issues during development.
     */
    AActor* Owner = GetOwner();
    UPrimitiveComponent* Body = ControlledBody ? ControlledBody.Get() : nullptr;
    UE_LOG(LogShipBasics, Log, TEXT("BeginPlay: Owner=%s, Body=%s"),
        Owner ? *Owner->GetName() : TEXT("<null>"),
        Body ? *Body->GetName() : TEXT("<null>"));

    /**
     * Log Physics Configuration
     * 
     * Logs the physics setup of the controlled body for debugging.
     * This helps verify that the physics body is properly configured.
     */
    if (Body)
    {
        const bool bSim = Body->IsSimulatingPhysics();
        const bool bGrav = Body->IsGravityEnabled();
        const float MassKg = Body->GetMass();
        UE_LOG(LogShipBasics, Log, TEXT("Body Physics: Simulating=%s, Gravity=%s, Mass=%.2f kg"),
            bSim ? TEXT("true") : TEXT("false"),
            bGrav ? TEXT("true") : TEXT("false"),
            MassKg);
    }
}

/**
 * ResolveBody - Find Physics Body Component
 * 
 * Attempts to find the physics body component for this actor.
 * This is used as a fallback when no Blueprint reference is set.
 * 
 * Search Order:
 * 1. Actor's root component (if it's a primitive component)
 * 2. First primitive component found in the actor
 * 
 * @return Pointer to physics body component, or nullptr if not found
 */
UPrimitiveComponent* USHIP_BASICS::ResolveBody() const
{
    if (AActor* Owner = GetOwner())
    {
        // First try the root component (most common case)
        if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Owner->GetRootComponent()))
        {
            return Prim;
        }
        
        // Fallback: search for any primitive component
        if (UPrimitiveComponent* FirstPrim = Owner->FindComponentByClass<UPrimitiveComponent>())
        {
            return FirstPrim;
        }
    }
    return nullptr;
}

/**
 * GetAgnosticController - Find Input Controller
 * 
 * Finds the AgnosticController for the current world.
 * This is used to get player input state for ship control.
 * 
 * @return Pointer to input controller, or nullptr if not found
 */
const AAgnosticController* USHIP_BASICS::GetAgnosticController() const
{
    if (!GetWorld()) return nullptr;
    
    // Get the first player controller and cast it to our custom controller
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        return Cast<AAgnosticController>(PC);
    }
    return nullptr;
}

/**
 * TickComponent - Main Update Loop
 * 
 * Called every frame to process input and apply physics forces.
 * This is the core of the ship's gameplay logic.
 * 
 * The function performs these steps:
 * 1. Validates that all required components are available
 * 2. Gets the current input state from the controller
 * 3. Applies forces and torques based on input
 * 4. Enforces speed limits
 * 
 * @param DeltaTime - Time elapsed since last frame
 * @param TickType - Type of tick (not used)
 * @param ThisTickFunction - Tick function info (not used)
 */
void USHIP_BASICS::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // ============================================================================
    // COMPONENT VALIDATION
    // ============================================================================
    
    /**
     * Resolve Physics Body
     * 
     * First tries the cached ControlledBody, then falls back to ResolveBody()
     * if no cached reference is available.
     */
    UPrimitiveComponent* Body = ControlledBody ? ControlledBody.Get() : ResolveBody();
    if (!Body)
    {
        if (!bWarnedNoBody)
        {
            UE_LOG(LogShipBasics, Warning, TEXT("Tick: No ControlledBody resolved for owner %s. Set it or ensure root is a physics-simulating primitive."),
                GetOwner() ? *GetOwner()->GetName() : TEXT("<null>"));
            bWarnedNoBody = true;
        }
        return;
    }

    /**
     * Validate Physics Simulation
     * 
     * Ensures the physics body is actually simulating physics.
     * Without this, forces won't have any effect.
     */
    if (!Body->IsSimulatingPhysics())
    {
        if (!bWarnedNoPhysics)
        {
            UE_LOG(LogShipBasics, Warning, TEXT("Tick: Body '%s' is not simulating physics. Enable 'Simulate Physics' on this component."), *Body->GetName());
            bWarnedNoPhysics = true;
        }
        return;
    }

    /**
     * Get Input Controller
     * 
     * Finds the AgnosticController to get player input state.
     * Without this, the ship can't respond to player input.
     */
    const AAgnosticController* AC = GetAgnosticController();
    if (!AC)
    {
        if (!bWarnedNoController)
        {
            UE_LOG(LogShipBasics, Warning, TEXT("Tick: No AAgnosticController found for world."));
            bWarnedNoController = true;
        }
        return;
    }

    // ============================================================================
    // INPUT PROCESSING AND PHYSICS APPLICATION
    // ============================================================================
    
    /**
     * Get Current Input State
     * 
     * Retrieves the current player input state from the controller.
     * This includes joystick positions, trigger values, and button states.
     */
    const FShipInputState& Input = AC->GetShipInputState();

    /**
     * Apply Physics Forces
     * 
     * Processes the input and applies the resulting forces and torques
     * to the physics body. This is where all the ship's movement logic happens.
     */
    ApplyForcesAndTorques(DeltaTime, Input, Body);
    
    /**
     * Enforce Speed Limits
     * 
     * Applies speed limits to prevent the ship from going too fast
     * and becoming uncontrollable.
     */
    ClampSpeeds(Body);
}

void USHIP_BASICS::ApplyForcesAndTorques(float DeltaTime, const FShipInputState& Input, UPrimitiveComponent* Body)
{
    AActor* Owner = GetOwner();
    if (!Owner) return;

    // Raw inputs
    FVector2D L = Input.LeftStick;   // X=Roll, Y=Pitch
    FVector2D R = Input.RightStick;  // X=Yaw,  Y=unused
    float ThrustRaw = Input.Thrust;  // 0..1
    const float BoostPct = FMath::Clamp(Input.Boost, 0.f, 1.f);

    UE_LOG(LogShipBasics, Verbose, TEXT("Input: L(%.2f,%.2f) R(%.2f,%.2f) Thrust=%.2f Boost=%.2f"),
        L.X, L.Y, R.X, R.Y, ThrustRaw, BoostPct);

    // Deadzone + smooth
    L = Deadzone2D(L, Settings.AxisDeadzone);
    R = Deadzone2D(R, Settings.AxisDeadzone);
    ThrustRaw = Deadzone(ThrustRaw, Settings.TriggerDeadzone);
    // Boost is axis percentage now; no deadzone applied here

    SmoothedLeft = FMath::Vector2DInterpTo(SmoothedLeft, L, DeltaTime, Settings.InputSmoothing);
    SmoothedRight = FMath::Vector2DInterpTo(SmoothedRight, R, DeltaTime, Settings.InputSmoothing);
    SmoothedThrust = FMath::FInterpTo(SmoothedThrust, ThrustRaw, DeltaTime, Settings.InputSmoothing);

    UE_LOG(LogShipBasics, Verbose, TEXT("Smoothed: L(%.2f,%.2f) R(%.2f,%.2f) T=%.2f"),
        SmoothedLeft.X, SmoothedLeft.Y, SmoothedRight.X, SmoothedRight.Y, SmoothedThrust);

    // Ship-local axes derived strictly from the physics body so X+=forward, Y+=right, Z+=up
    const FTransform BodyXform = Body->GetComponentTransform();
    const FVector Forward = BodyXform.GetUnitAxis(EAxis::X); // roll axis (local X)
    const FVector Right   = BodyXform.GetUnitAxis(EAxis::Y); // pitch axis (local Y)
    const FVector Up      = BodyXform.GetUnitAxis(EAxis::Z); // yaw axis (local Z)

    // Forces
    const float   ForwardForce = (SmoothedThrust * Settings.ThrustForce);
    const FVector ForceVec = Forward * ForwardForce;

    // Alignment-based scaling (normal thrust only). Boost is applied separately unscaled.
    const FVector CurrentVel = Body->GetPhysicsLinearVelocity();
    const float   Cos01      = CosineSimilarity01(ForceVec, CurrentVel);
    const float   ThrustScale = MapAlignmentToThrustScale(Cos01);

    FVector FinalForce = ForceVec * ThrustScale;
    if (BoostPct > 0.f)
    {
        FinalForce += Forward * (Settings.BoostForce * BoostPct);
    }

    // Mass-independent acceleration
    Body->AddForce(FinalForce, NAME_None, /*bAccelChange=*/true);

    // Torques (pitch about Right, yaw about Up, roll about Forward), with input sign overrides
    FVector Torque =
        (Up * (SmoothedRight.X * Settings.YawTorque * Settings.YawInputSign)) +
        (Right * (SmoothedLeft.Y * Settings.PitchTorque * Settings.PitchInputSign)) +
        (Forward * (SmoothedLeft.X * Settings.RollTorque * Settings.RollInputSign));

    // Always apply manual torque - player can override Orient Opposite
    Body->AddTorqueInRadians(Torque, NAME_None, /*bAccelChange=*/true);
    
    // If player is giving manual input while Orient Opposite is active, cancel it
    const bool bManualInput = (FMath::Abs(SmoothedLeft.X) > 0.1f || FMath::Abs(SmoothedLeft.Y) > 0.1f || FMath::Abs(SmoothedRight.X) > 0.1f);
    if (bOrientingOpposite && bManualInput)
    {
        bOrientingOpposite = false;
    }

    // Thruster weights for VFX/anim and logging
    float WFwd, WBack, WRight, WLeft, WUp, WDown;
    ComputeLocalThrusterWeights(Owner->GetActorTransform(), FinalForce, WFwd, WBack, WRight, WLeft, WUp, WDown);

    UE_LOG(LogShipBasics, Verbose, TEXT("Applied: Force=%s (|F|=%.1f, Cos01=%.2f, Scale=%.2f, Boost=%.2f) Torque=%s (|T|=%.2f) Weights F=%.2f B=%.2f R=%.2f L=%.2f U=%.2f D=%.2f"),
        *FinalForce.ToString(), FinalForce.Size(), Cos01, ThrustScale, BoostPct,
        *Torque.ToString(), Torque.Size(), WFwd, WBack, WRight, WLeft, WUp, WDown);

    // Full 3D retrograde align while held
    if (Input.bOrientOpposite)
    {
        const FVector ShipVelocity = Body->GetPhysicsLinearVelocity();
        if (ShipVelocity.SizeSquared() > 100.0f) // Only if moving (>10 cm/s)
        {
            // Desired forward is opposite of velocity
            const FVector TargetFwd = -ShipVelocity.GetSafeNormal();

            // Current forward in corrected physics frame
            const FRotator AxesRotForRot = Settings.PhysicsAxesCorrection;
            const FRotationMatrix AxesMatForRot(AxesRotForRot);
            const FVector CurFwd = AxesMatForRot.TransformVector(Owner->GetActorForwardVector()).GetSafeNormal();
            const FVector CurUp  = AxesMatForRot.TransformVector(Owner->GetActorUpVector()).GetSafeNormal();

            // Angle and axis between current forward and target forward
            const float Dot = FMath::Clamp(FVector::DotProduct(CurFwd, TargetFwd), -1.0f, 1.0f);
            const float Angle = FMath::Acos(Dot); // radians

            if (Angle < FMath::DegreesToRadians(1.0f))
            {
                // Close enough: stop spinning
                Body->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector, false);
            }
            else
            {
                FVector Axis = FVector::CrossProduct(CurFwd, TargetFwd);
                if (Axis.SizeSquared() < KINDA_SMALL_NUMBER)
                {
                    // 180° case: pick Up as a stable axis
                    Axis = CurUp;
                }
                Axis = Axis.GetSafeNormal();

                // Spin at fixed rate about the needed axis
                const float RateRad = FMath::DegreesToRadians(Settings.OppositeRotationRateDegPerSec);
                Body->SetPhysicsAngularVelocityInRadians(Axis * RateRad, false);
            }

            bOrientingOpposite = true;
        }
    }
    else if (bOrientingOpposite)
    {
        // Button released - stop orienting and clear angular velocity
        Body->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector, false);
        bOrientingOpposite = false;
    }
}

void USHIP_BASICS::ClampSpeeds(UPrimitiveComponent* Body) const
{
    if (!Body) return;

    if (Settings.MaxLinearSpeed > 0.f)
    {
        const FVector V = Body->GetPhysicsLinearVelocity();
        const float   S = V.Size();
        if (S > Settings.MaxLinearSpeed)
        {
            Body->SetPhysicsLinearVelocity(V.GetSafeNormal() * Settings.MaxLinearSpeed, false);
        }
    }

    if (Settings.MaxAngularSpeed > 0.f)
    {
        const FVector W = Body->GetPhysicsAngularVelocityInRadians();
        const float   M = W.Size();
        if (M > Settings.MaxAngularSpeed)
        {
            Body->SetPhysicsAngularVelocityInRadians(W.GetSafeNormal() * Settings.MaxAngularSpeed, false);
        }
    }
}

void USHIP_BASICS::ZeroAngularVelocity()
{
    UPrimitiveComponent* Body = ControlledBody ? ControlledBody.Get() : ResolveBody();
    if (!Body) return;
    Body->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector, false);
    bOrientingOpposite = false;
}
