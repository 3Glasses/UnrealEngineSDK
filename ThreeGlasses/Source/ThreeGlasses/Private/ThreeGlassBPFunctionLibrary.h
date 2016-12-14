// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "ThreeGlassBPFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class UThreeGlassBPFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_UCLASS_BODY()

public:

    /**
    * Set ThreeGlasses HMD InterpupillaryDistance
    *
    * @param    IPD Set Interpupillary Distance Value in meters.
    */
    UFUNCTION(BlueprintCallable, Category = "ThreeGlassesVR")
    static void SetHMDInterpupillaryDistance(float IPD = 0.064);

    /**
    * Get ThreeGlasses HMD InterpupillaryDistance
    *
    * @return    ThreeGlasses Get InterpupillaryDistance in meters.
    */
    UFUNCTION(BlueprintCallable, Category = "ThreeGlassesVR")
    static float GetHMDInterpupillaryDistance();

    /**
    * Set ThreeGlasses HMD Motion Prediction Factor, For D2
    *
    * @param    bMotionPredictionOn Enable/Disable MotionPrediction.
    * @param    bVsyncOn	        Enable/Disable Vsync. If you enable MotionPerdiction, please enable Vsync too.
    * @param    PredictionFactor	Motion Prediction Factor.
    * @param    MaxFPS              Max FPS.
    */
	UFUNCTION(BlueprintCallable, Category = "ThreeGlassesVR")
	static void SetHMDMotionPredictionFactor(bool bMotionPredictionOn = true, bool bVsyncOn = true, float PredictionFactor = 1.367f, int MaxFPS = 60);

    /**
    * Set ThreeGlasses HMD Stereo Effect Parameters
    * 
    * @param    HFOV        Field of View.
    * @param    GazePlane	It will affect the Stereo Projection Matrix.
    */
    UFUNCTION(BlueprintCallable, Category = "ThreeGlassesVR")
    static void SetStereoEffectParam(float HFOV = 65.0f, float GazePlane = 33.95749f);
	
};
