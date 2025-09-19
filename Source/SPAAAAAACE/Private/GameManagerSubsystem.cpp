/**
 * GameManagerSubsystem Implementation
 * 
 * This file contains the implementation of the game state management subsystem.
 * It provides a centralized location for game state management and system
 * coordination.
 * 
 * Key Systems:
 * - Subsystem lifecycle management
 * - Game state initialization and cleanup
 * - Convenient access patterns for other systems
 * - Extensible architecture for future features
 */

#include "GameManagerSubsystem.h"
#include "Engine/World.h"

/**
 * Initialize - Subsystem Initialization
 * 
 * Called when the subsystem is first created. This is where
 * initial game state is set up and systems are initialized.
 * 
 * Currently minimal implementation that can be extended with
 * additional game state management as needed.
 * 
 * @param Collection - Collection of subsystems being initialized
 */
void UGameManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	/**
	 * Initialize Global Game State
	 * 
	 * This is where global game state would be initialized.
	 * Currently minimal, but can be extended with:
	 * - Game session management
	 * - Player state tracking
	 * - Event system setup
	 * - Data persistence initialization
	 */
	
	// init global state here later if needed
}

/**
 * Deinitialize - Subsystem Cleanup
 * 
 * Called when the subsystem is being destroyed. This is where
 * cleanup and finalization should occur.
 * 
 * Currently minimal implementation that can be extended with
 * additional cleanup logic as needed.
 */
void UGameManagerSubsystem::Deinitialize()
{
	/**
	 * Cleanup Global Game State
	 * 
	 * This is where global game state would be cleaned up.
	 * Currently minimal, but can be extended with:
	 * - Save game data
	 * - Cleanup event listeners
	 * - Release resources
	 * - Finalize statistics
	 */
	// tear down state here if needed
	
	Super::Deinitialize();
}

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
UGameManagerSubsystem* UGameManagerSubsystem::Get(const UObject* WorldContext)
{
	if (!WorldContext) return nullptr;
	if (UWorld* World = WorldContext->GetWorld())
	{
		return World->GetSubsystem<UGameManagerSubsystem>();
	}
	return nullptr;
}
