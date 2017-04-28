// Fill out your copyright notice in the Description page of Project Settings.

#include "ThreeGlassesHMD.h"
#include "ThreeGlassBPFunctionLibrary.h"

static SZVR::MemoryManager ShareMemory;

UThreeGlassBPFunctionLibrary::UThreeGlassBPFunctionLibrary(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

FThreeGlassesHMD* GetThreeGlassesHMD()
{
    if (GEngine->HMDDevice.IsValid() && (GEngine->HMDDevice->GetHMDDeviceType() == EHMDDeviceType::DT_ES2GenericStereoMesh))
    {
        return static_cast<FThreeGlassesHMD*>(GEngine->HMDDevice.Get());
    }

    return nullptr;
}

void UThreeGlassBPFunctionLibrary::SetHMDInterpupillaryDistance(float NewIPD)
{
    FThreeGlassesHMD* HMD = GetThreeGlassesHMD();
    if (HMD != nullptr)
    {
        HMD->SetInterpupillaryDistance(NewIPD);
        //UE_LOG(LogClass, Log, TEXT("Set New IPD for HMD!! Value : %f"), NewIPD);
    }
}

float UThreeGlassBPFunctionLibrary::GetHMDInterpupillaryDistance()
{
    float IPD = 0.064;
    FThreeGlassesHMD* HMD = GetThreeGlassesHMD();
    if (HMD != nullptr)
    {
        IPD = HMD->GetInterpupillaryDistance();
    }
    return IPD;
}

void UThreeGlassBPFunctionLibrary::GetHMDOrientationAndPosition(FQuat& CurrentOrientation, FVector& CurrentPosition)
{
	FThreeGlassesHMD* HMD = GetThreeGlassesHMD();
	if (HMD != nullptr)
	{
		HMD->GetCurrentOrientationAndPosition(CurrentOrientation, CurrentPosition);
	}
}

bool UThreeGlassBPFunctionLibrary::GetControllerOrientationAndPosition(const int32 ControllerIndex,  FRotator& OutOrientation, FVector& OutPosition) 
{
	FQuat DeviceOrientation = FQuat::Identity;
	float PosData[6] = { 0 };
	SZVR_GetWandPos(PosData);
	float RotData[8] = { 0 };
	SZVR_GetWandRotate(RotData);

	switch (ControllerIndex)
	{
	case 0:
		DeviceOrientation = FQuat(RotData[0], RotData[1], RotData[2], RotData[3]);
		OutPosition = FVector(-PosData[2], -PosData[0], -PosData[1]) *0.1f;
		break;
	case 1:
		DeviceOrientation = FQuat(RotData[4], RotData[5], RotData[6], RotData[7]);
		OutPosition = FVector(-PosData[5], -PosData[3], -PosData[4]) *0.1f;
		break;
	default:
		return false;
	}

	if (DeviceOrientation.W == 0)
	{
		DeviceOrientation = FQuat::Identity;
	}

	DeviceOrientation = FQuat(-DeviceOrientation.Z, -DeviceOrientation.X, DeviceOrientation.Y, DeviceOrientation.W);
	OutOrientation = DeviceOrientation.Rotator();
	return true;
}

bool UThreeGlassBPFunctionLibrary::IsHMDExitButtonDown()
{
	bool result = false;
	if (SZVR_GetHMDPresent(&result) == 0)
	{
		return result;
	}

	return result;
}

bool UThreeGlassBPFunctionLibrary::IsHMDMenuButtonDown()
{
	bool result = false;
	if (SZVR_GetHMDMenuButton(&result) == 0)
	{
		return result;
	}

	return result;
}

bool UThreeGlassBPFunctionLibrary::IsControllerButtonPressed(int ControllerIndex, EControllerButton eButton)
{
	bool ButtonStatus[12] = { 0 };
	if (SZVR_GetWandButton(ButtonStatus) == 0)
	{
		return ButtonStatus[ControllerIndex * 6 + eButton];
	}

	return false;
}

void UThreeGlassBPFunctionLibrary::GetControllerStick(int32 ControllerIndex, int32& X, int32& Y)
{
	uint8_t ret[4] = { 0 };
	if (SZVR_GetWandStick(ret) == 0)
	{
		if (ControllerIndex==0)
		{
			X = ret[0];
			Y = ret[1];
		}
		else if (ControllerIndex == 1)
		{
			X = ret[2];
			Y = ret[3];
		}
	}
}

void UThreeGlassBPFunctionLibrary::SetWandHapic(int32 Hand, int32 Frequency, int32 Amplitude)
{
	if (Hand >= 0 && Hand < 2)
	{
		uint16 data[2] = { 0 };
		ShareMemory.LoadDataMemory(SZVR::eWAND_VIBRATOR, data);
		data[Hand] = Amplitude;
		ShareMemory.SaveDataMemory(SZVR::eWAND_VIBRATOR, data);
	}
}