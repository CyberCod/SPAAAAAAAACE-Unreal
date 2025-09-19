/**
 * GameManagerSubsystem - Game State Management System
 * 
 * This file defines a world subsystem for managing game state and
 * coordinating between different game systems.
 * 
 * Key Features:
 * - World-level game state management
 * - System coordination and communication
 * - Blueprint integration for game logic
 * - Extensible architecture for future features
 * 
 * The subsystem provides a centralized location for game state
 * management and can be extended with additional functionality
 * as the game grows in complexity.
 */

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameManagerSubsystem.generated.h"

/**
 * UGameManagerSubsystem - Game State Management Subsystem
 * 
 * A world subsystem that manages game state and coordinates between
 * different game systems. This provides a centralized location for
 * game logic that needs to persist across the world.
 * 
 * Key Responsibilities:
 * - Game state management
 * - System coordination
 * - Event handling
 * - Data persistence
 * 
 * The subsystem is designed to be lightweight and extensible,
 * with no ticking by default to minimize performance impact.
 */
UCLASS()
class SPAAAAAACE_API UGameManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ============================================================================
	// LIFECYCLE MANAGEMENT
	// ============================================================================
	
	/**
	 * Initialize - Subsystem Initialization
	 * 
	 * Called when the subsystem is first created. This is where
	 * initial game state is set up and systems are initialized.
	 * 
	 * @param Collection - Collection of subsystems being initialized
	 */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	/**
	 * Deinitialize - Subsystem Cleanup
	 * 
	 * Called when the subsystem is being destroyed. This is where
	 * cleanup and finalization should occur.
	 */
	virtual void Deinitialize() override;

	// ============================================================================
	// ACCESS AND UTILITIES
	// ============================================================================
	
	/**
	 * Get - Get Game Manager Subsystem Instance
	 * 
	 * Static convenience function to get the game manager subsystem
	 * from any world context. This makes it easy to access the
	 * subsystem from anywhere in the codebase.
	 * 
	 * @param WorldContext - Any object with world context
	 * @return Pointer to the game manager subsystem, or nullptr if not found
	 */
	UFUNCTION(BlueprintPure, Category = "GM")
	static UGameManagerSubsystem* Get(const UObject* WorldContext);
};
