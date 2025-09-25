#include "ExhaustBellController.h"

#include "AgnosticController.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"

UExhaustBellController::UExhaustBellController()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UExhaustBellController::BeginPlay()
{
	Super::BeginPlay();

	if (UActorComponent* Comp = ExhaustBellRef.GetComponent(GetOwner()))
	{
		ExhaustBell = Cast<UStaticMeshComponent>(Comp);
	}

	if (ExhaustBell)
	{
		InitialRelRotation = ExhaustBell->GetRelativeRotation();
		InitialRelScale = ExhaustBell->GetRelativeScale3D();
		InitialRelLocation = ExhaustBell->GetRelativeLocation();

		// Apply designer offset once; afterwards location follows parent naturally
		ExhaustBell->SetRelativeLocation(InitialRelLocation + RelativeLocationOffset);
	}
}

void UExhaustBellController::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!ExhaustBell) { return; }

	const float LT = FMath::Clamp(GetLT(), 0.f, 1.f);

	// Uniform scale from baseline
	const float S = FMath::Lerp(ScaleMin, ScaleMax, LT);
	ExhaustBell->SetRelativeScale3D(InitialRelScale * S);

	// Accumulate roll (local Z) scaled by LT
	const float DeltaDeg = LT * RotationSpeed * DeltaTime;
	AccumRollDeg = FMath::Fmod(AccumRollDeg + DeltaDeg, 360.0f);
	if (AccumRollDeg < 0.f) { AccumRollDeg += 360.0f; }

    // Build new relative rotation around the selected LOCAL axis
    // EXHAUST BELL ROTATION AXIS CHOSEN HERE
    if (bUseCustomAxis)
    {
        FVector Axis = RotationAxisLocal;
        if (!Axis.Normalize())
        {
            Axis = FVector::UpVector; // Fallback if zero-length axis provided
        }
        const FQuat Q = FQuat(Axis, FMath::DegreesToRadians(AccumRollDeg));
        const FQuat Base = InitialRelRotation.Quaternion();
        // LOCAL rotation composition: Base then Q (Base * Q)
        ExhaustBell->SetRelativeRotation((Base * Q).Rotator());
    }
    else
    {
        FRotator NewRelRot = InitialRelRotation;
        switch (RotationAxis)
        {
        case EExhaustRotationAxis::X: NewRelRot.Roll += AccumRollDeg;  break;
        case EExhaustRotationAxis::Y: NewRelRot.Pitch += AccumRollDeg; break;
        default:                     NewRelRot.Yaw += AccumRollDeg;   break;
        }
        ExhaustBell->SetRelativeRotation(NewRelRot);
    }

    // Location not modified here; follows parent transform naturally
}

float UExhaustBellController::GetLT() const
{
	if (!GetWorld()) return 0.f;
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (const AAgnosticController* AC = Cast<AAgnosticController>(PC))
		{
			// Access input state via public getter
			const FShipInputState& State = AC->GetShipInputState();
			return FMath::Clamp(State.Thrust, 0.f, 1.f);
		}
	}
	return 0.f;
}


