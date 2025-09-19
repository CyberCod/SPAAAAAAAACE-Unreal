/**
 * SpaceGameMode - Game Mode for Space Racing
 * 
 * This file defines the game mode for the space racing game.
 * The game mode handles level initialization, player spawning,
 * and game state management.
 * 
 * Key Responsibilities:
 * - Level initialization and setup
 * - Player spawning and controller assignment
 * - Game state management
 * - Integration with other game systems
 * 
 * This is the main game mode that orchestrates the overall
 * game experience and manages the game's lifecycle.
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SpaceGameMode.generated.h"

/**
 * ASpaceGameMode - Space Racing Game Mode
 * 
 * This is the main game mode class for the space racing game.
 * It handles the overall game state, player management, and
 * level initialization.
 * 
 * The game mode is responsible for:
 * - Setting up the game world
 * - Spawning players and assigning controllers
 * - Managing game rules and win conditions
 * - Coordinating between different game systems
 */
UCLASS()
class SPAAAAAACE_API ASpaceGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	/**
	 * Constructor
	 * 
	 * Initializes the game mode with default settings.
	 * This is where the game mode's configuration is set up.
	 */
	ASpaceGameMode();

protected:
	/**
	 * BeginPlay - Game Mode Initialization
	 * 
	 * Called when the game mode starts. This is where the game
	 * world is set up and players are spawned.
	 * 
	 * This function handles:
	 * - Game world initialization
	 * - Player spawning
	 * - Controller assignment
	 * - Game state setup
	 */
	virtual void BeginPlay() override;
};


