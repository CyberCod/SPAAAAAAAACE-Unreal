/**
 * SPAAAAAACE - Main Project Implementation
 * 
 * This is the main implementation file for the SPAAAAAACE project.
 * It defines the primary game module that Unreal Engine uses to
 * load and manage the project.
 * 
 * Key Functions:
 * - Module registration with Unreal Engine
 * - Project initialization
 * - Build system integration
 * 
 * This file is required by Unreal Engine's module system and
 * serves as the entry point for the project's C++ code.
 */

#include "SPAAAAAACE.h"
#include "Modules/ModuleManager.h"

/**
 * IMPLEMENT_PRIMARY_GAME_MODULE - Primary Game Module Definition
 * 
 * This macro tells Unreal Engine that this is the primary game module
 * for the project. It registers the module with the engine's module
 * system and allows the project to be loaded and executed.
 * 
 * Parameters:
 * - FDefaultGameModuleImpl: The default implementation class for game modules
 * - SPAAAAAACE: The name of the module (must match project name)
 * - "SPAAAAAACE": The display name of the module
 */
IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, SPAAAAAACE, "SPAAAAAACE");
