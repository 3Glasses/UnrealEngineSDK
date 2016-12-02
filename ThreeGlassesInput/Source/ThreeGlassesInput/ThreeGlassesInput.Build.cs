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

      //  PrivateIncludePathModuleNames.AddRange(
      //      new string[]
      //      {
      //          "InputDevice",			// For IInputDevice.h
		    //}
      //      );


        PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "CoreUObject",
                "Engine",
    //            "InputCore",        // Provides LOCTEXT and other Input features
				//"InputDevice",      // Provides IInputInterface
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
    //            "InputCore",        // Provides LOCTEXT and other Input features
				//"InputDevice",      // Provides IInputInterface
                "HeadMountedDisplay",
                "Slate",
				"SlateCore",
       
				// ... add private dependencies that you statically link with here ...	
			}
			);

        PublicLibraryPaths.Add(ModuleDirectory + "/lib/");
        PublicAdditionalLibraries.Add("SZVR_MEMAPI.lib");

        DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
