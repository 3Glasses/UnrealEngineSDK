// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Engine.h"
#include "IThreeGlassesPlugin.h"
#include "HeadMountedDisplay.h"

#include "SceneViewExtension.h"

#include "ShowFlags.h"
#include "SceneView.h"

#include "Shader.h"

#include "SZVR_CAPI.h"

#define THREE_GLASSES_SUPPORTED_PLATFORMS (PLATFORM_WINDOWS && WINVER > 0x0502)
#if THREE_GLASSES_SUPPORTED_PLATFORMS

class FThreeGlassesHMD : public IHeadMountedDisplay, public ISceneViewExtension, public TSharedFromThis < FThreeGlassesHMD, ESPMode::ThreadSafe >
{
public:
	/** IHeadMountedDisplay interface */
	virtual bool IsHMDConnected() override;
	virtual bool IsHMDEnabled() const override;
	virtual void EnableHMD(bool allow = true) override;
	virtual EHMDDeviceType::Type GetHMDDeviceType() const override;
	virtual bool GetHMDMonitorInfo(MonitorInfo&) override;

	virtual void GetFieldOfView(float& OutHFOVInDegrees, float& OutVFOVInDegrees) const override;

	virtual bool DoesSupportPositionalTracking() const override;
	virtual bool HasValidTrackingPosition() override;
	virtual void GetPositionalTrackingCameraProperties(FVector& OutOrigin, FQuat& OutOrientation, float& OutHFOV, float& OutVFOV, float& OutCameraDistance, float& OutNearPlane, float& OutFarPlane) const override;

	virtual void SetInterpupillaryDistance(float NewInterpupillaryDistance) override;
	virtual float GetInterpupillaryDistance() const override;

	virtual void GetCurrentOrientationAndPosition(FQuat& CurrentOrientation, FVector& CurrentPosition) override;
	virtual TSharedPtr<class ISceneViewExtension, ESPMode::ThreadSafe> GetViewExtension() override;
	virtual void ApplyHmdRotation(APlayerController* PC, FRotator& ViewRotation) override;
#if __UESDK_4_10__
	virtual void UpdatePlayerCameraRotation(class APlayerCameraManager* Camera, struct FMinimalViewInfo& POV) override;
#endif
	virtual bool UpdatePlayerCamera(FQuat& CurrentOrientation, FVector& CurrentPosition) override;

	virtual bool IsChromaAbCorrectionEnabled() const override;

	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;
	//virtual void OnScreenModeChange(EWindowMode::Type WindowMode) override;

	virtual bool IsPositionalTrackingEnabled() const override;
	virtual bool EnablePositionalTracking(bool enable) override;
	virtual void RebaseObjectOrientationAndPosition(FVector& Position, FQuat& Orientation) const override;

	virtual bool IsHeadTrackingAllowed() const override;

	virtual bool IsInLowPersistenceMode() const override;
	virtual void EnableLowPersistenceMode(bool Enable = true) override;

	virtual void ResetOrientationAndPosition(float yaw = 0.f) override;
	virtual void ResetOrientation(float Yaw = 0.f) override;
	virtual void ResetPosition() override;

	virtual void SetClippingPlanes(float NCP, float FCP) override;

	virtual void SetBaseRotation(const FRotator& BaseRot) override;
	virtual FRotator GetBaseRotation() const override;

	virtual void SetBaseOrientation(const FQuat& BaseOrient) override;
	virtual FQuat GetBaseOrientation() const override;

	virtual void DrawDistortionMesh_RenderThread(struct FRenderingCompositePassContext& Context, const FIntPoint& TextureSize) override;

	/** IStereoRendering interface */
	virtual bool IsStereoEnabled() const override;
	virtual bool EnableStereo(bool stereo = true) override;
	virtual void AdjustViewRect(EStereoscopicPass StereoPass, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const override;
	virtual void CalculateStereoViewOffset(const EStereoscopicPass StereoPassType, const FRotator& ViewRotation,
		const float MetersToWorld, FVector& ViewLocation) override;
	virtual FMatrix GetStereoProjectionMatrix(const EStereoscopicPass StereoPassType, const float FOV) const override;
	virtual void InitCanvasFromView(FSceneView* InView, UCanvas* Canvas) override;
	virtual void GetEyeRenderParams_RenderThread(const struct FRenderingCompositePassContext& Context, FVector2D& EyeToSrcUVScaleValue, FVector2D& EyeToSrcUVOffsetValue) const override;
	virtual void CalculateRenderTargetSize(const class FViewport& Viewport, uint32& InOutSizeX, uint32& InOutSizeY) override;
	virtual void DrawDebug(UCanvas* Canvas) override;

	/** ISceneViewExtension interface */
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override;
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) {}
	virtual void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override;
	virtual void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override;

	/**
	* This method is called when playing begins. Useful to reset all runtime values stored in the plugin.
	*/
	virtual void OnBeginPlay(FWorldContext& InWorldContext) override;

public:
	/** Constructor */
	FThreeGlassesHMD();

	/** Destructor */
	virtual ~FThreeGlassesHMD();

	/** @return	True if the HMD was initialized OK */
	bool IsInitialized() const;
	bool InitDevice();
	void UpdateStereoRenderingParams();
	void ApplySystemOverridesOnStereo(bool force = false);
	FString GetVersionString() const;


private:
	void Startup();
	void Shutdown();

private:

	typedef enum
	{
		szvrEye_Left = 0,
		szvrEye_Right = 1,
		szvrEye_Count = 2
	} szvrEyeType;

	struct FDistortionVertex
	{
		FVector2D	Position;
		FVector2D	TexR;
		FVector2D	TexG;
		FVector2D	TexB;
		float		VignetteFactor;
		float		TimewarpFactor;
	};

	struct FDistortionMesh
	{
		DistMeshVert*				pVertices;
		FDistortionVertex*			pVerticesCached;
		int*						pIndices;
		int							NumVertices;
		int							NumIndices;
		int							NumTriangles;

		FDistortionMesh() :pVertices(nullptr), pIndices(nullptr), NumVertices(0), NumIndices(0), NumTriangles(0) {}
		~FDistortionMesh() { Clear(); }
		void Clear() { delete pVertices; delete pIndices; NumVertices = NumIndices = NumTriangles = 0; }
	};

	typedef uint64 bool64;
	union
	{
		struct
		{
			uint64 InitStatus : 2; // see bitmask EInitStatus

			/** Whether stereo is currently on or off. */
			bool64 bStereoEnabled : 1;

			/** Whether or not switching to stereo is allowed */
			bool64 bHMDEnabled : 1;

			/** Indicates if it is necessary to update stereo rendering params */
			bool64 bNeedUpdateStereoRenderingParams : 1;

			/** Debugging:  Whether or not the stereo rendering settings have been manually overridden by an exec command.  They will no longer be auto-calculated */
			bool64 bOverrideStereo : 1;

			/** Debugging:  Whether or not the IPD setting have been manually overridden by an exec command. */
			bool64 bOverrideIPD : 1;

			/** Debugging:  Whether or not the distortion settings have been manually overridden by an exec command.  They will no longer be auto-calculated */
			bool64 bOverrideDistortion : 1;

			/** Debugging: Allows changing internal params, such as screen size, eye-to-screen distance, etc */
			bool64 bDevSettingsEnabled : 1;

			bool64 bOverrideFOV : 1;

			/** Whether or not to override game VSync setting when switching to stereo */
			bool64 bOverrideVSync : 1;

			/** Overridden VSync value */
			bool64 bVSync : 1;

			/** Saved original values for VSync and ScreenPercentage. */
			bool64 bSavedVSync : 1;

			/** Whether or not to override game ScreenPercentage setting when switching to stereo */
			bool64 bOverrideScreenPercentage : 1;

			/** Allows renderer to finish current frame. Setting this to 'true' may reduce the total
			*  framerate (if it was above vsync) but will reduce latency. */
			bool64 bAllowFinishCurrentFrame : 1;

			/** Whether world-to-meters scale is overriden or not. */
			bool64 bWorldToMetersOverride : 1;

			/** Distortion on/off */
			bool64 bHmdDistortion : 1;

			/** Debug left eye */
			bool64 bNotRenderLeft : 1;

			/** Debug right eye */
			bool64 bNotRenderRight : 1;

			bool64 bDrawGrid : 1;
		};
		uint64 Raw;
	} Flags;

	const szvrHeadDisplayDevice * Hmd;

	float NearClippingPlane;
	float FarClippingPlane;

	float WorldToMetersScale;
	float ScreenPercentage;

	float HFOVInRadians; // horizontal
	float VFOVInRadians; // vertical

	float InterpupillaryDistance;

	bool m_bWndSet;

	mutable FQuat			CurHmdOrientation;
	FQuat DeltaControlOrientation; // same as DeltaControlRotation but as quat
	FRotator DeltaControlRotation;

	mutable FVector			CurHmdPosition;
	FQuat LastHmdOrientation; // contains last APPLIED ON GT HMD orientation
	FVector LastHmdPosition;

	// HMD base values, specify forward orientation and zero pos offset
	FQuat BaseOrientation;	// base orientation

	double					LastSensorTime;

	FDistortionMesh DistorMesh[2];

	void GetCurrentPose(FQuat& CurrentHmdOrientation, FVector& CurrentHmdPosition);
};

FORCEINLINE FMatrix ToFMatrix(const szvrMatrix4f& vtm)
{
	// Rows and columns are swapped between OVR::Matrix4f and FMatrix
	return FMatrix(
		FPlane(vtm.M[0][0], vtm.M[1][0], vtm.M[2][0], vtm.M[3][0]),
		FPlane(vtm.M[0][1], vtm.M[1][1], vtm.M[2][1], vtm.M[3][1]),
		FPlane(vtm.M[0][2], vtm.M[1][2], vtm.M[2][2], vtm.M[3][2]),
		FPlane(vtm.M[0][3], vtm.M[1][3], vtm.M[2][3], vtm.M[3][3]));
}

#endif //THREE_GLASSES_SUPPORTED_PLATFORMS