# UnrealEngineSDK
3Glasses SDK Plugins for Unreal Engine 4.12

1. Installation
  - Download all the UnrealEngineSDK repository files.
  - Create a new folder under your Unreal Project root folder, and name it as 'Plugins'.
  - Copy all the repository files into 'Plugins' folder.
  - Right click your Unreal project *.uproject file, select 'Generate Visual Studio project files'.
  - Open your Unreal Visual Studio solution file *.sln, then build the entire solution.
  - To add the 3Glasses HMD to driver white list, double click the register file 'whilelist.reg' under the 'Source' folder.
  - Connect HMD to PC, excute file in 'Source' folder with parameter 'DirectModeD3DD11.exe -enable' under Command Prompt.
  - Make sure all Oculus Plugins is disable in Unreal Plugin configurate page.
  
2. Updates
  - Support launcher version Unreal Engine so you don't need to build the whole source code of Engine.
  - Support mono style of mirror window.
  - Support Nvidia DirectMode, present with better performance.
  
