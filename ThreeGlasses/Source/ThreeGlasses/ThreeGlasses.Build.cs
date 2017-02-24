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
                    "Runtime/Renderer/Private",
                    "ThreeGlasses/Private"
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
                    "D3D11RHI"
                }
				);

			if (UEBuildConfiguration.bBuildEditor == true)
			{
				PrivateDependencyModuleNames.Add("UnrealEd");
			}

            string ThreeGlassesLibPath = ModuleDirectory + "/lib/";
            // Currently, the ThreeGlasses is only supported on windows platforms
            if (Target.Platform == UnrealTargetPlatform.Win32 || Target.Platform == UnrealTargetPlatform.Win64)
            {
                PublicIncludePaths.AddRange(
                   new string[]
                   {
                       "Runtime/Renderer/Private",
                       "Runtime/Windows/D3D11RHI/Private",
                       "Runtime/Windows/D3D11RHI/Private/Windows"
                       // ... add other private include paths required here ...
                   });
                PrivateDependencyModuleNames.AddRange(new string[] { "OpenGLDrv", "D3D11RHI" });
            
                if (Target.Platform == UnrealTargetPlatform.Win32)
                {
                    PublicLibraryPaths.Add(ThreeGlassesLibPath + "win32");
                    PublicAdditionalLibraries.Add("ThirdParty/Windows/DirectX/Lib/x86/d3dx11.lib");
                    PublicAdditionalLibraries.Add("ThirdParty/Windows/DirectX/Lib/x86/d3d11.lib");
                }
                else
                {
                    PublicLibraryPaths.Add(ThreeGlassesLibPath + "x64");
                    RuntimeDependencies.Add(new RuntimeDependency(ModuleDirectory + "/3GlassesTracker.dll"));
                    PublicAdditionalLibraries.Add(ThreeGlassesLibPath + "x64/SDKCompositor.lib");
                    PublicAdditionalLibraries.Add(ThreeGlassesLibPath + "x64/SZVRSharedMemoryAPI.lib");
                    PublicAdditionalLibraries.Add("ThirdParty/Windows/DirectX/Lib/x64/d3dx11.lib");
                    PublicAdditionalLibraries.Add("ThirdParty/Windows/DirectX/Lib/x64/d3d11.lib");
                }
            }
		}
	}
}