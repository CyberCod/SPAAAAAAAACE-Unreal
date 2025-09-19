// Copyright Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;

public class SPAAAAAACE : ModuleRules
{
    public SPAAAAAACE(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "ProceduralMeshComponent"
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "EnhancedInput"
        });

        // Slate / OnlineSubsystem lines left commented intentionally.
    }
}
