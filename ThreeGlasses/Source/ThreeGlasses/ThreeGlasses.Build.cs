// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
using System;
using System.IO;

namespace UnrealBuildTool.Rules
{
	public class ThreeGlasses : ModuleRules
	{
        public ThreeGlasses(TargetInfo Target)
		{
			PrivateIncludePaths.AddRange(
				new string[] {
					"ThreeGlasses/Private",
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"InputCore",
					"RHI",
					"RenderCore",
					"Renderer",
					"ShaderCore",
					"HeadMountedDisplay",
					"Slate",
					"SlateCore",
				}
				);

			if (UEBuildConfiguration.bBuildEditor == true)
			{
				PrivateDependencyModuleNames.Add("UnrealEd");
			}

            string ThreeGlassesLibPath = ModuleDirectory + "/lib/";
            // Currently, the ThreeGlasses is only supported on windows 64bits platforms
            if (Target.Platform == UnrealTargetPlatform.Win32 || Target.Platform == UnrealTargetPlatform.Win64)
            {
                PrivateDependencyModuleNames.AddRange(new string[] { "OpenGLDrv", "D3D11RHI" });
                if (Target.Platform == UnrealTargetPlatform.Win32)
                {
                    PublicAdditionalLibraries.Add(ThreeGlassesLibPath + "win32/ThreeGlasses.lib");
                }
                else
                {
                    PublicAdditionalLibraries.Add(ThreeGlassesLibPath + "x64/ThreeGlasses64.lib");
                }
            }
		}
	}
}