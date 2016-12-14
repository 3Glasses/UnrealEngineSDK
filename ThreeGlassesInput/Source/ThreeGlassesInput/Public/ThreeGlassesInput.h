// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"
#include "Runtime/InputDevice/Public/IInputDeviceModule.h"

class FThreeGlassesInputModule : public IInputDeviceModule
{
public:
	static inline FThreeGlassesInputModule& Get()
	{
		return FModuleManager::LoadModuleChecked< FThreeGlassesInputModule >("ThreeGlassesInput");
	}

	
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("ThreeGlassesInput");
	}

	virtual TSharedPtr< class IInputDevice > CreateInputDevice(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler) override;
};