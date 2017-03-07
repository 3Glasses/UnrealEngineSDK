// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "Engine.h"
#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#include <d3d11.h>
#include "ThirdParty/Windows/DirectX/Include/D3DX11tex.h"
#include "SZVRSharedMemoryAPI.h"
#include "HideWindowsPlatformTypes.h"
#endif

#include "IThreeGlassesPlugin.h"
#include "HeadMountedDisplay.h"

#include "SceneViewExtension.h"

#include "ShowFlags.h"
#include "SceneView.h"

#include "Shader.h"

#define THREE_GLASSES_SUPPORTED_PLATFORMS 1//(PLATFORM_WINDOWS && WINVER > 0x0502)
#if THREE_GLASSES_SUPPORTED_PLATFORMS

class FThreeGlassesHMD : public IHeadMountedDisplay, public ISceneViewExtension, public TSharedFromThis < FThreeGlassesHMD, ESPMode::ThreadSafe >
{
public:
	/** IHeadMountedDisplay interface */
	virtual FName GetDeviceName() const
	{
		static FName DefaultName(TEXT("ThreeGlassesHMD"));
		return DefaultName;
	}

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
	
	virtual float GetScreenPercentage() const { return 2.0f; }
	/** IStereoRendering interface */
	virtual bool IsStereoEnabled() const override;
	virtual bool EnableStereo(bool stereo = true) override;
	virtual FVector2D GetTextSafeRegionBounds() const { return FVector2D(1.f, 1.f); }
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
	virtual void RenderTexture_RenderThread(class FRHICommandListImmediate& RHICmdList, class FRHITexture2D* BackBuffer, class FRHITexture2D* SrcTexture) const override;

	/**
	* This method is called when playing begins. Useful to reset all runtime values stored in the plugin.
	*/
	virtual void OnBeginPlay(FWorldContext& InWorldContext) override;
	virtual void OnEndPlay(FWorldContext& InWorldContext) override;
public:
	/** Constructor */
	FThreeGlassesHMD();

	/** Destructor */
	virtual ~FThreeGlassesHMD();

	/** @return	True if the HMD was initialized OK */
	bool IsInitialized() const;
	void UpdateStereoRenderingParams();
	void ApplySystemOverridesOnStereo(bool force = false);
	FString GetVersionString() const;
	virtual void UpdateViewport(bool bUseSeparateRenderTarget, const FViewport& Viewport, class SViewport*) override;
	bool NeedReAllocateViewportRenderTarget(const FViewport &viewport);
	virtual bool ShouldUseSeparateRenderTarget() const override
	{
		check(IsInGameThread());
		return IsStereoEnabled();
	}

public:
	int32									HMDDesktopX = 0;
	int32									HMDDesktopY = 0;
	int32									HMDResX = 2880;
	int32									HMDResY = 1440;

	void RenderMirrorToBackBuffer(class FRHICommandListImmediate& rhiCmdList, class FRHITexture2D* srcTexture, class FRHITexture2D* backBuffer) const;

	bool AllocateRenderTargetTexture(
		uint32 index,
		uint32 sizeX, uint32 sizeY,
		uint8 format, uint32 numMips, uint32 flags,
		uint32 targetableTextureFlags,
		FTexture2DRHIRef& outTargetableTexture,
		FTexture2DRHIRef& outShaderResourceTexture,
		uint32 numSamples);

private:
	class D3D11Present : public FRHICustomPresent
	{
	public:
		D3D11Present() :
			FRHICustomPresent(nullptr)
		{}

		virtual ~D3D11Present();// should release any references to D3D resources.
		virtual void OnBackBufferResize() {};
		virtual bool Present(int32& InOutSyncInterval);
		virtual void PostPresent();
		virtual void Reset() {};
		virtual void Shutdown() {};
	};

	D3D11Present* mCurrentPresent = NULL;
private:
	bool  bHMDEnabled;
	bool  bStereoEnabled;

	float InterpupillaryDistance;

	FQuat DeltaControlOrientation; // same as DeltaControlRotation but as quat
	FRotator DeltaControlRotation;

	void GetCurrentPose(FQuat& CurrentHmdOrientation, FVector& CurrentHmdPosition);
};
#endif //THREE_GLASSES_SUPPORTED_PLATFORMS