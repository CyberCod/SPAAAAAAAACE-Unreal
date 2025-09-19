/**
 * SpaceGameMode Implementation
 * 
 * This file contains the implementation of the space racing game mode.
 * It handles game initialization, player spawning, and controller assignment.
 * 
 * Key Systems:
 * - Blueprint class loading and assignment
 * - Player controller and pawn setup
 * - Game mode initialization
 * - Error handling and logging
 */

#include "SpaceGameMode.h"

// Game-specific includes
#include "ShipPawn.h"                  // Ship pawn class
#include "AgnosticController.h"        // Input controller class

// Core engine includes
#include "UObject/ConstructorHelpers.h" // Blueprint class loading

/**
 * ASpaceGameMode Constructor
 * 
 * Initializes the game mode with Blueprint classes for the player controller
 * and default pawn. This allows for Blueprint customization while maintaining
 * C++ functionality.
 */
ASpaceGameMode::ASpaceGameMode()
{
	// ============================================================================
	// BLUEPRINT CLASS LOADING
	// ============================================================================
	
	/**
	 * Load Player Controller Blueprint
	 * 
	 * Attempts to load the Blueprint version of the AgnosticController.
	 * This allows for Blueprint customization of input handling while
	 * maintaining the C++ functionality.
	 */
	static ConstructorHelpers::FClassFinder<APlayerController> ControllerBPClass(TEXT("/Game/BP_AgnosticController"));
	if (ControllerBPClass.Succeeded())
	{
		PlayerControllerClass = ControllerBPClass.Class;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("BP_AgnosticController not found at /Game/BP_AgnosticController. Set PlayerController in project/level settings."));
	}

	/**
	 * Load Default Pawn Blueprint
	 * 
	 * Attempts to load the Blueprint version of the ShipPawn.
	 * This allows for Blueprint customization of ship behavior while
	 * maintaining the C++ physics and gameplay systems.
	 */
	static ConstructorHelpers::FClassFinder<APawn> ShipPawnBPClass(TEXT("/Game/BP_ShipPawn"));
	if (ShipPawnBPClass.Succeeded())
	{
		DefaultPawnClass = ShipPawnBPClass.Class;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("BP_ShipPawn not found at /Game/BP_ShipPawn. Set DefaultPawn in project/level settings."));
	}
}

/**
 * BeginPlay - Game Mode Initialization
 * 
 * Called when the game mode starts. This is where the game world is set up
 * and players are spawned. The function logs the current configuration for
 * debugging purposes.
 */
void ASpaceGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	/**
	 * Log Game Mode Status
	 * 
	 * Logs the current configuration of the game mode for debugging.
	 * This helps identify which classes are being used for the player
	 * controller and default pawn.
	 */
	UE_LOG(LogTemp, Log, TEXT("ASpaceGameMode active. DefaultPawn=%s Controller=%s"),
		*GetDefaultPawnClassForController(nullptr)->GetName(),
		*PlayerControllerClass->GetName());
}

