// Fill out your copyright notice in the Description page of Project Settings.

#include "ThreeGlassesHMD.h"
#include "ThreeGlassBPFunctionLibrary.h"

UThreeGlassBPFunctionLibrary::UThreeGlassBPFunctionLibrary(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

#if THREE_GLASSES_SUPPORTED_PLATFORMS
FThreeGlassesHMD* GetThreeGlassesHMD()
{
    if (GEngine->HMDDevice.IsValid() && (GEngine->HMDDevice->GetHMDDeviceType() == EHMDDeviceType::DT_OculusRift))
    {
        return static_cast<FThreeGlassesHMD*>(GEngine->HMDDevice.Get());
    }

    return nullptr;
}
#endif // THREE_GLASSES_SUPPORTED_PLATFORMS

void UThreeGlassBPFunctionLibrary::SetHMDInterpupillaryDistance(float NewIPD)
{
#if THREE_GLASSES_SUPPORTED_PLATFORMS
    FThreeGlassesHMD* HMD = GetThreeGlassesHMD();
    if (HMD != nullptr)
    {
        HMD->SetInterpupillaryDistance(NewIPD);
        //UE_LOG(LogClass, Log, TEXT("Set New IPD for HMD!! Value : %f"), NewIPD);
    }
#endif // THREE_GLASSES_SUPPORTED_PLATFORMS
}

float UThreeGlassBPFunctionLibrary::GetHMDInterpupillaryDistance()
{
    float IPD = 0.064;
#if THREE_GLASSES_SUPPORTED_PLATFORMS
    FThreeGlassesHMD* HMD = GetThreeGlassesHMD();
    if (HMD != nullptr)
    {
        IPD = HMD->GetInterpupillaryDistance();
    }
#endif // THREE_GLASSES_SUPPORTED_PLATFORMS
    return IPD;
}

//For D2, Motion Prediction
void UThreeGlassBPFunctionLibrary::SetHMDMotionPredictionFactor(bool bMotionPredictionOn, bool bVsyncOn, float PredictionFactor, int MaxFPS)
{
#if THREE_GLASSES_SUPPORTED_PLATFORMS
    FThreeGlassesHMD* HMD = GetThreeGlassesHMD();
    if (HMD != nullptr)
    {
        HMD->SetMotionPredictionFactor(bMotionPredictionOn, bVsyncOn, PredictionFactor, MaxFPS);
    }
#endif // THREE_GLASSES_SUPPORTED_PLATFORMS
}

void UThreeGlassBPFunctionLibrary::SetStereoEffectParam(float HFOV, float GazePlane)
{
#if THREE_GLASSES_SUPPORTED_PLATFORMS
    FThreeGlassesHMD* HMD = GetThreeGlassesHMD();
    if (HMD != nullptr)
    {
        HMD->SetStereoEffectParam(HFOV, GazePlane);
    }
#endif // THREE_GLASSES_SUPPORTED_PLATFORMS
}