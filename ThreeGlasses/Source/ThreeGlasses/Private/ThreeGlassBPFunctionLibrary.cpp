// Fill out your copyright notice in the Description page of Project Settings.

#include "ThreeGlassesHMD.h"
#include "ThreeGlassBPFunctionLibrary.h"

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

//For D2, Motion Prediction
void UThreeGlassBPFunctionLibrary::SetHMDMotionPredictionFactor(bool bMotionPredictionOn, bool bVsyncOn, float PredictionFactor, int MaxFPS)
{
    FThreeGlassesHMD* HMD = GetThreeGlassesHMD();
    if (HMD != nullptr)
    {
        HMD->SetMotionPredictionFactor(bMotionPredictionOn, bVsyncOn, PredictionFactor, MaxFPS);
    }
}

void UThreeGlassBPFunctionLibrary::SetStereoEffectParam(float HFOV, float GazePlane)
{
    FThreeGlassesHMD* HMD = GetThreeGlassesHMD();
    if (HMD != nullptr)
    {
        HMD->SetStereoEffectParam(HFOV, GazePlane);
    }
}

void UThreeGlassBPFunctionLibrary::GetHMDOrientationAndPosition(FQuat& CurrentOrientation, FVector& CurrentPosition)
{
	FThreeGlassesHMD* HMD = GetThreeGlassesHMD();
	if (HMD != nullptr)
	{
		HMD->GetCurrentOrientationAndPosition(CurrentOrientation, CurrentPosition);
	}
}

bool UThreeGlassBPFunctionLibrary::GetControllerOrientationAndPosition(const int32 ControllerIndex, const EControllerHand DeviceHand, FRotator& OutOrientation, FVector& OutPosition) 
{
	FQuat DeviceOrientation = FQuat::Identity;
	float PosData[6] = { 0 };
	SZVR_GetWandPos(PosData);
	float RotData[8] = { 0 };
	SZVR_GetWandRotate(RotData);

	switch (DeviceHand)
	{
	case EControllerHand::Left:
		DeviceOrientation = FQuat(RotData[0], RotData[1], RotData[2], RotData[3]);
		OutPosition = FVector(-PosData[2], -PosData[0], -PosData[1]) *0.1f;
		break;
	case EControllerHand::Right:
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

bool UThreeGlassBPFunctionLibrary::GetWandButton(TArray<bool>& ButtonStatus)
{
	if (ButtonStatus.Num() != 12)
	{
		return false;
	}

	return SZVR_GetWandButton(ButtonStatus.GetData()) == 0;
}

bool UThreeGlassBPFunctionLibrary::GetWandStick(int32& LeftX, int32& LeftY, int32& RightX, int32& RightY)
{
	uint8_t ret[4] = {0};
	bool result = SZVR_GetWandStick(ret) == 0;
	LeftX = ret[0];
	LeftY = ret[1];
	RightX = ret[2];
	RightY = ret[3];
	return result;
}