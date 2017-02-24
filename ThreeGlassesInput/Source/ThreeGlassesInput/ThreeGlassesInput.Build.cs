// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ThreeGlassesInput : ModuleRules
{
	public ThreeGlassesInput(TargetInfo Target)
	{
		
		PublicIncludePaths.AddRange(
			new string[] {
				"ThreeGlassesInput/Public",
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				"ThreeGlassesInput/Private",
             
				// ... add other private include paths required here ...
			}
			);

        PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "CoreUObject",
                "Engine",
                "HeadMountedDisplay",
				// ... add other public dependencies that you statically link with here ...
			}
			);

        PrivateDependencyModuleNames.AddRange(
			new string[]
			{
                "Core",
				"CoreUObject",
				"Engine",
                "HeadMountedDisplay",
                "Slate",
				"SlateCore",
       
				// ... add private dependencies that you statically link with here ...	
			}
			);

        PublicLibraryPaths.Add(ModuleDirectory + "/lib/");
        PublicAdditionalLibraries.Add("SZVRSharedMemoryAPI.lib");

        DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
