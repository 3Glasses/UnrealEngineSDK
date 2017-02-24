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
    static void SetStereoEffectParam(float HFOV = 65.0f, float GazePlane = 340.f);
	
	UFUNCTION(BlueprintCallable, Category = "ThreeGlassesVR")
	static void GetHMDOrientationAndPosition(FQuat& CurrentOrientation, FVector& CurrentPosition);

	UFUNCTION(BlueprintCallable, Category = "ThreeGlassesVR")
	static bool GetControllerOrientationAndPosition(const int32 ControllerIndex, const EControllerHand DeviceHand, FRotator& OutOrientation, FVector& OutPosition);

	UFUNCTION(BlueprintCallable, Category = "ThreeGlassesVR")
	static bool IsHMDExitButtonDown();

	UFUNCTION(BlueprintCallable, Category = "ThreeGlassesVR")
	static bool IsHMDMenuButtonDown();

	/**
	*	Get ThreeGlasses Wand Button status
	*  @param  ButtonStatus the array has 12 element,true is Pressed.
	*	first wand
	*	    0 Menu button;
	*	    1 Back button;
	*	    2 Left handle button;
	*	    3 Right handle button;
	*	    4 Trigger press down;
	*	    5 Trigger press all the way down;
	*
	*	second wand
	*	    6 Menu button;
	*	    7 Back button;
	*	    8 Left handle button;
	*	    9 Right handle button;
	*	    10 Trigger press down;
	*	    11 Trigger press all the way down;
	* @return  true is success, other value is false
	*/
	UFUNCTION(BlueprintCallable, Category = "ThreeGlassesVR")
	static bool GetWandButton(TArray<bool>& ButtonStatus);

	/**
	* Get Wand stick value. 0 is minimum,255 is Maximum
	* @return 	It returns true it success, else value is false*/
	UFUNCTION(BlueprintCallable, Category = "ThreeGlassesVR")
	static bool GetWandStick(int32& LeftX, int32& LeftY, int32& RightX, int32& RightY);
};
