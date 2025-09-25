#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ExhaustBellController.generated.h"

class UStaticMeshComponent;
class AAgnosticController;

/** Axis to rotate the exhaust bell around (local space) */
UENUM(BlueprintType)
enum class EExhaustRotationAxis : uint8
{
    X UMETA(DisplayName = "Local X (Roll)"),
    Y UMETA(DisplayName = "Local Y (Pitch)"),
    Z UMETA(DisplayName = "Local Z (Yaw)")
};

/**
 * UExhaustBellController
 * 
 * Component that controls a ship's exhaust bell visual:
 * - Uniform scale between ScaleMin..ScaleMax based on LT (thrust) axis
 * - Continuous rotation about local Z at LT * RotationSpeed deg/s
 * 
 * The target mesh is assigned via FComponentReference and resolved at runtime.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SPAAAAAACE_API UExhaustBellController : public UActorComponent
{
	GENERATED_BODY()

public:
	UExhaustBellController();

	// === Blueprint Tuning ===
	/** Minimum uniform scale factor applied at LT=0 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExhaustBell")
	float ScaleMin = 0.9f;

	/** Maximum uniform scale factor applied at LT=1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExhaustBell")
	float ScaleMax = 1.3f;

	/** Rotation speed in degrees per second at LT=1 (scaled by LT) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExhaustBell", meta=(ClampMin="0"))
	float RotationSpeed = 360.0f;

    /** If true, use RotationAxisLocal (vector) instead of the enum */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExhaustBell")
    bool bUseCustomAxis = true;

    /**
     * RotationAxisLocal - LOCAL axis (mesh space) to rotate around.
     * Example: (1,0,0)=local X, (0,1,0)=local Y, (0,0,1)=local Z.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExhaustBell")
    FVector RotationAxisLocal = FVector(0.f, 0.f, 1.f);

    /** Enum remains available; used only if bUseCustomAxis=false */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExhaustBell")
    EExhaustRotationAxis RotationAxis = EExhaustRotationAxis::Z;

    /**
     * Relative location offset to apply to the exhaust mesh (in parent/ship local space)
     * Applied on top of the mesh's initial relative location captured at BeginPlay.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExhaustBell")
    FVector RelativeLocationOffset = FVector::ZeroVector;

	/** Exhaust bell mesh reference (editor-assignable) */
	UPROPERTY(EditAnywhere, Category = "ExhaustBell|Refs")
	FComponentReference ExhaustBellRef;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Resolved mesh pointer at runtime */
	UPROPERTY(VisibleAnywhere, Category = "ExhaustBell|Refs")
	TObjectPtr<UStaticMeshComponent> ExhaustBell = nullptr;

	/** Cached initial relative rotation to preserve baseline orientation */
	FRotator InitialRelRotation = FRotator::ZeroRotator;

	/** Cached initial relative scale to scale uniformly from baseline */
	FVector InitialRelScale = FVector(1.0f);

    /** Cached initial relative location to offset from baseline */
    FVector InitialRelLocation = FVector::ZeroVector;

	/** Accumulated roll (about local Z) in degrees, wrapped 0..360 */
	float AccumRollDeg = 0.0f;

	/** Get current Left Trigger (thrust) value from the input controller (0..1) */
	float GetLT() const;
};


