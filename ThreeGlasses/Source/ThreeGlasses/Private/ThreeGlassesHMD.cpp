#include "ThreeGlassesHMD.h"
#include "Runtime/Renderer/Private/RendererPrivate.h"
#include "Runtime/Renderer/Private/ScenePrivate.h"
#include "Runtime/Engine/Public/ScreenRendering.h"
#include "Runtime/Renderer/Private/PostProcess/RenderingCompositionGraph.h"
#include "DrawDebugHelpers.h"

#if WITH_EDITOR
#include "Editor/UnrealEd/Classes/Editor/EditorEngine.h"
#endif


#if THREE_GLASSES_SUPPORTED_PLATFORMS

class FThreeGlassesPlugin : public IThreeGlassesPlugin
{
	/** IHeadMountedDisplayModule implementation */
	virtual TSharedPtr< class IHeadMountedDisplay, ESPMode::ThreadSafe > CreateHeadMountedDisplay() override;

	FString GetModulePriorityKeyName() const
	{
		return FString(TEXT("ThreeGlasses"));
	}

public:

};

IMPLEMENT_MODULE(FThreeGlassesPlugin, ThreeGlasses)

TSharedPtr< class IHeadMountedDisplay, ESPMode::ThreadSafe > FThreeGlassesPlugin::CreateHeadMountedDisplay()
{
	TSharedPtr< FThreeGlassesHMD, ESPMode::ThreadSafe > ThreeGlassesHMD(new FThreeGlassesHMD());

	if (ThreeGlassesHMD->IsInitialized())
	{
		return ThreeGlassesHMD;
	}
	return NULL;
}

bool FThreeGlassesHMD::IsHMDEnabled() const
{
	return Flags.bHMDEnabled;
}

void FThreeGlassesHMD::RebaseObjectOrientationAndPosition(FVector& Position, FQuat& Orientation) const
{
}

void FThreeGlassesHMD::EnableHMD(bool enable)
{
	Flags.bHMDEnabled = enable;

	if (!Flags.bHMDEnabled)
	{
		EnableStereo(false);
	}
}

EHMDDeviceType::Type FThreeGlassesHMD::GetHMDDeviceType() const
{
	return EHMDDeviceType::DT_OculusRift;
}

bool FThreeGlassesHMD::GetHMDMonitorInfo(MonitorInfo& MonitorDesc)
{
	if (IsInitialized())
	{
		InitDevice();
	}
	if (Hmd)
	{
		MonitorDesc.MonitorName = Hmd->szDevice;
		MonitorDesc.MonitorId = Hmd->DisplayId;
		MonitorDesc.DesktopX = Hmd->WindowsPos.x;
		MonitorDesc.DesktopY = Hmd->WindowsPos.y;
		MonitorDesc.ResolutionX = Hmd->Resolution.w;
		MonitorDesc.ResolutionY = Hmd->Resolution.h;
		return true;
	}
	else
	{
		MonitorDesc.MonitorName = "";
		MonitorDesc.MonitorId = 0;
		MonitorDesc.DesktopX = MonitorDesc.DesktopY = MonitorDesc.ResolutionX = MonitorDesc.ResolutionY = 0;
		MonitorDesc.WindowSizeX = MonitorDesc.WindowSizeY = 0;
	}
	return false;
}

void FThreeGlassesHMD::GetFieldOfView(float& OutHFOVInDegrees, float& OutVFOVInDegrees) const
{
	OutHFOVInDegrees = FMath::RadiansToDegrees(HFOVInRadians);
	OutVFOVInDegrees = FMath::RadiansToDegrees(VFOVInRadians);
}

bool FThreeGlassesHMD::DoesSupportPositionalTracking() const
{
	return false;
}

bool FThreeGlassesHMD::HasValidTrackingPosition()
{
	return false;
}

void FThreeGlassesHMD::GetPositionalTrackingCameraProperties(FVector& OutOrigin, FQuat& OutOrientation, float& OutHFOV, float& OutVFOV, float& OutCameraDistance, float& OutNearPlane, float& OutFarPlane) const
{
}

void FThreeGlassesHMD::SetInterpupillaryDistance(float NewInterpupillaryDistance)
{
	InterpupillaryDistance = NewInterpupillaryDistance;
	UpdateStereoRenderingParams();
}

float FThreeGlassesHMD::GetInterpupillaryDistance() const
{
	return InterpupillaryDistance;
}

void FThreeGlassesHMD::GetCurrentPose(FQuat& CurrentHmdOrientation, FVector& CurrentHmdPosition)
{
	if (IsHMDConnected())
	{
		SZVR_GetHmdOrientationWithQuaternion(
			&CurrentHmdOrientation.Y, 
			&CurrentHmdOrientation.Z,
			&CurrentHmdOrientation.X,
			&CurrentHmdOrientation.W);
		CurrentHmdOrientation.Y *= -1;
		CurrentHmdOrientation.Z *= -1;
	}
}

void FThreeGlassesHMD::GetCurrentOrientationAndPosition(FQuat& CurrentOrientation, FVector& CurrentPosition)
{
	check(IsInGameThread());
	GetCurrentPose(CurHmdOrientation, CurHmdPosition);
	CurrentOrientation = LastHmdOrientation = CurHmdOrientation;

	CurrentPosition = CurHmdPosition;
}

TSharedPtr<ISceneViewExtension, ESPMode::ThreadSafe> FThreeGlassesHMD::GetViewExtension()
{
	TSharedPtr<FThreeGlassesHMD, ESPMode::ThreadSafe> ptr(AsShared());
	return StaticCastSharedPtr<ISceneViewExtension>(ptr);
}

void FThreeGlassesHMD::ApplyHmdRotation(APlayerController* PC, FRotator& ViewRotation)
{
	ViewRotation.Normalize();

	GetCurrentPose(CurHmdOrientation, CurHmdPosition);
	LastHmdOrientation = CurHmdOrientation;

	const FRotator DeltaRot = ViewRotation - PC->GetControlRotation();
	DeltaControlRotation = (DeltaControlRotation + DeltaRot).GetNormalized();

	// Pitch from other sources is never good, because there is an absolute up and down that must be respected to avoid motion sickness.
	// Same with roll.
	DeltaControlRotation.Pitch = 0;
	DeltaControlRotation.Roll = 0;
	DeltaControlOrientation = DeltaControlRotation.Quaternion();

	ViewRotation = FRotator(DeltaControlOrientation * CurHmdOrientation);
}

#if __UESDK_4_10__
void FThreeGlassesHMD::UpdatePlayerCameraRotation(APlayerCameraManager* Camera, struct FMinimalViewInfo& POV)
{
	GetCurrentPose(CurHmdOrientation, CurHmdPosition);
	LastHmdOrientation = CurHmdOrientation;

	DeltaControlRotation = POV.Rotation;
	DeltaControlOrientation = DeltaControlRotation.Quaternion();

	// Apply HMD orientation to camera rotation.
	POV.Rotation = FRotator(POV.Rotation.Quaternion() * CurHmdOrientation);
}
#endif

bool FThreeGlassesHMD::UpdatePlayerCamera(FQuat& CurrentOrientation, FVector& CurrentPosition)
{
	GetCurrentPose(CurHmdOrientation, CurHmdPosition);
	LastHmdOrientation = CurHmdOrientation;
	LastHmdPosition = CurHmdPosition;

	if (!bImplicitHmdPosition && GEnableVREditorHacks)
	{
		DeltaControlOrientation = CurrentOrientation;
		DeltaControlRotation = DeltaControlOrientation.Rotator();
	}

	CurrentOrientation = CurHmdOrientation;
	CurrentPosition = CurHmdPosition;

	return true;
}

bool FThreeGlassesHMD::IsChromaAbCorrectionEnabled() const
{
	return false;
}

//void FThreeGlassesHMD::OnScreenModeChange(EWindowMode::Type WindowMode)
//{
//	EnableStereo(WindowMode != EWindowMode::Windowed);
//	UpdateStereoRenderingParams();
//}

bool FThreeGlassesHMD::IsPositionalTrackingEnabled() const
{
	return false;
}

bool FThreeGlassesHMD::EnablePositionalTracking(bool enable)
{
	return false;
}

bool FThreeGlassesHMD::IsHeadTrackingAllowed() const
{
#if WITH_EDITOR
	UEditorEngine* EdEngine = Cast<UEditorEngine>(GEngine);
#endif//WITH_EDITOR
	return Hmd &&
#if WITH_EDITOR
		(!EdEngine || EdEngine->bUseVRPreviewForPlayWorld || GetDefault<ULevelEditorPlaySettings>()->ViewportGetsHMDControl) &&
#endif//WITH_EDITOR
		(GEngine->IsStereoscopic3D());
}


bool FThreeGlassesHMD::IsInLowPersistenceMode() const
{
	return false;
}

void FThreeGlassesHMD::EnableLowPersistenceMode(bool Enable)
{
}

void FThreeGlassesHMD::ResetOrientationAndPosition(float yaw)
{
	ResetOrientation(yaw);
	ResetPosition();
}

void FThreeGlassesHMD::ResetOrientation(float Yaw)
{
	FRotator ViewRotation;
	ViewRotation.Pitch = 0;
	ViewRotation.Roll = 0;

	if (Yaw != 0.f)
	{
		// apply optional yaw offset
		ViewRotation.Yaw -= Yaw;
		ViewRotation.Normalize();
	}

	BaseOrientation = ViewRotation.Quaternion();

	SZVR_ResetSensorOrientation();
}

void FThreeGlassesHMD::ResetPosition()
{
}

void FThreeGlassesHMD::SetClippingPlanes(float NCP, float FCP)
{
}

void FThreeGlassesHMD::SetBaseRotation(const FRotator& BaseRot)
{
}

FRotator FThreeGlassesHMD::GetBaseRotation() const
{
	return FRotator::ZeroRotator;
}

void FThreeGlassesHMD::SetBaseOrientation(const FQuat& BaseOrient)
{
}

FQuat FThreeGlassesHMD::GetBaseOrientation() const
{
	return FQuat::Identity;
}

void FThreeGlassesHMD::ApplySystemOverridesOnStereo(bool force)
{
	// TODO: update flags/settings here triggered by exec
}

void FThreeGlassesHMD::UpdateStereoRenderingParams()
{
	// If we've manually overridden stereo rendering params for debugging, don't mess with them
	if (Flags.bOverrideStereo || !IsStereoEnabled() )
	{
		return;
	}
	if (IsInitialized() && Hmd)
	{
		DistScaleOffsetUV DummyDistScale;
		for (int eyeIndex = szvrEye_Left; eyeIndex < szvrEye_Count; ++eyeIndex)
		{
			SZVR_CopyDistortionMesh(DistorMesh[eyeIndex].pVertices, DistorMesh[eyeIndex].pIndices, &DummyDistScale, eyeIndex == szvrEye_Right);

			for (int i = 0; i < DistorMesh[eyeIndex].NumVertices; ++i)
			{
				DistorMesh[eyeIndex].pVerticesCached[i].Position.X = DistorMesh[eyeIndex].pVertices[i].ScreenPosNDC_x;
				DistorMesh[eyeIndex].pVerticesCached[i].Position.Y = DistorMesh[eyeIndex].pVertices[i].ScreenPosNDC_y;

				DistorMesh[eyeIndex].pVerticesCached[i].VignetteFactor = 1.0f;
				DistorMesh[eyeIndex].pVerticesCached[i].TimewarpFactor = DistorMesh[eyeIndex].pVertices[i].TimewarpLerp;

				DistorMesh[eyeIndex].pVerticesCached[i].TexR = FVector2D(DistorMesh[eyeIndex].pVertices[i].UVIR_u, DistorMesh[eyeIndex].pVertices[i].UVIR_v);
				DistorMesh[eyeIndex].pVerticesCached[i].TexG = FVector2D(DistorMesh[eyeIndex].pVertices[i].UVIG_u, DistorMesh[eyeIndex].pVertices[i].UVIG_v);
				DistorMesh[eyeIndex].pVerticesCached[i].TexB = FVector2D(DistorMesh[eyeIndex].pVertices[i].UVIB_u, DistorMesh[eyeIndex].pVertices[i].UVIB_v);
			}
		}

		Flags.bNeedUpdateStereoRenderingParams = false;
	}
}

void FThreeGlassesHMD::DrawDistortionMesh_RenderThread(struct FRenderingCompositePassContext& Context, const FIntPoint& TextureSize)
{
	check(IsInRenderingThread());

	const FSceneView& View = Context.View;

	FRHICommandListImmediate& RHICmdList = Context.RHICmdList;
	const FSceneViewFamily& ViewFamily = *(View.Family);
	int ViewportSizeX = ViewFamily.RenderTarget->GetRenderTargetTexture()->GetSizeX();
	int ViewportSizeY = ViewFamily.RenderTarget->GetRenderTargetTexture()->GetSizeY();
	RHICmdList.SetViewport(0, 0, 0.0f, ViewportSizeX, ViewportSizeY, 1.0f);

	const FDistortionMesh& distMesh = DistorMesh[(View.StereoPass == eSSP_LEFT_EYE) ? szvrEye_Left : szvrEye_Right];

	if ((!Flags.bNotRenderLeft && View.StereoPass == eSSP_LEFT_EYE) || (!Flags.bNotRenderRight && View.StereoPass == eSSP_RIGHT_EYE) )
	{
		DrawIndexedPrimitiveUP(Context.RHICmdList, PT_TriangleList, 0, distMesh.NumVertices, distMesh.NumTriangles, distMesh.pIndices,
			sizeof(int), distMesh.pVerticesCached, sizeof(FDistortionVertex));
	}
}

#if !UE_BUILD_SHIPPING
static void RenderLines(FCanvas* Canvas, int numLines, const FColor& c, float* x, float* y)
{
	for (int i = 0; i < numLines; ++i)
	{
		FCanvasLineItem line(FVector2D((x[i * 2]/2+0.5)*1280, (-y[i * 2]/2+0.5)*720), FVector2D((x[i * 2 + 1]/2+0.5)*1280, (-y[i * 2 + 1]/2+0.5)*720));
		line.SetColor(FLinearColor(c));
		Canvas->DrawItem(line);
	}
}
#endif // #if !UE_BUILD_SHIPPING

void FThreeGlassesHMD::DrawDebug(UCanvas* Canvas)
{
#if !UE_BUILD_SHIPPING
	check(IsInGameThread());
	if (!Flags.bDrawGrid) return;
	const FColor cLeft(0, 128, 255);
	const FColor cRight(0, 255, 0);
	for (int eyeIndex = szvrEye_Left; eyeIndex < szvrEye_Count; ++eyeIndex)
	{
		if (Flags.bNotRenderLeft && eyeIndex == szvrEye_Left || Flags.bNotRenderRight && eyeIndex == szvrEye_Right)
			continue;

		float x[2], y[2];
		for (int i = 0; i < DistorMesh[eyeIndex].NumTriangles; i++)
		{
			x[0] = DistorMesh[eyeIndex].pVerticesCached[DistorMesh[eyeIndex].pIndices[i * 3]].Position.X;
			x[1] = DistorMesh[eyeIndex].pVerticesCached[DistorMesh[eyeIndex].pIndices[i * 3 + 1]].Position.X;
			y[0] = DistorMesh[eyeIndex].pVerticesCached[DistorMesh[eyeIndex].pIndices[i * 3]].Position.Y;
			y[1] = DistorMesh[eyeIndex].pVerticesCached[DistorMesh[eyeIndex].pIndices[i * 3 + 1]].Position.Y;
			RenderLines(Canvas->Canvas, 1, eyeIndex == szvrEye_Left ? cLeft : cRight, x, y);
			x[0] = DistorMesh[eyeIndex].pVerticesCached[DistorMesh[eyeIndex].pIndices[i * 3 + 1]].Position.X;
			x[1] = DistorMesh[eyeIndex].pVerticesCached[DistorMesh[eyeIndex].pIndices[i * 3 + 2]].Position.X;
			y[0] = DistorMesh[eyeIndex].pVerticesCached[DistorMesh[eyeIndex].pIndices[i * 3 + 1]].Position.Y;
			y[1] = DistorMesh[eyeIndex].pVerticesCached[DistorMesh[eyeIndex].pIndices[i * 3 + 2]].Position.Y;
			RenderLines(Canvas->Canvas, 1, eyeIndex == szvrEye_Left ? cLeft : cRight, x, y);
			x[0] = DistorMesh[eyeIndex].pVerticesCached[DistorMesh[eyeIndex].pIndices[i * 3 + 2]].Position.X;
			x[1] = DistorMesh[eyeIndex].pVerticesCached[DistorMesh[eyeIndex].pIndices[i * 3]].Position.X;
			y[0] = DistorMesh[eyeIndex].pVerticesCached[DistorMesh[eyeIndex].pIndices[i * 3 + 2]].Position.Y;
			y[1] = DistorMesh[eyeIndex].pVerticesCached[DistorMesh[eyeIndex].pIndices[i * 3]].Position.Y;
			RenderLines(Canvas->Canvas, 1, eyeIndex == szvrEye_Left ? cLeft : cRight, x, y);
		}
	}

#endif //!UE_BUILD_SHIPPING
}

bool FThreeGlassesHMD::IsStereoEnabled() const
{
	return true;
}

bool FThreeGlassesHMD::EnableStereo(bool stereo)
{
	Flags.bStereoEnabled = (IsHMDEnabled()) ? stereo : false;
	int x, y, w, h;
	if (SZVR_GetHmdMonitor(&x, &y, &w, &h))
	{
		FSystemResolution::RequestResolutionChange(w, h,
			EWindowMode::WindowedFullscreen);
	}

	return Flags.bStereoEnabled;
}

void FThreeGlassesHMD::AdjustViewRect(EStereoscopicPass StereoPass, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const
{
	SizeX = SizeX / 2;
	if (StereoPass == eSSP_RIGHT_EYE)
	{
		X += SizeX;
	}
}

void FThreeGlassesHMD::CalculateStereoViewOffset(const enum EStereoscopicPass StereoPassType, const FRotator& ViewRotation, const float WorldToMeters, FVector& ViewLocation)
{
}

FMatrix FThreeGlassesHMD::GetStereoProjectionMatrix(const enum EStereoscopicPass StereoPassType, const float FOV) const
{
	const float ProjectionCenterOffset = 0.15f;
	const float PassProjectionOffset = (StereoPassType == eSSP_LEFT_EYE) ? ProjectionCenterOffset : -ProjectionCenterOffset;

	return FMatrix(
		FPlane(0.63f, 0.0f, 0.0f, 0.0f),
		FPlane(0.0f, 0.63f, 0.0f, 0.0f),
		FPlane(0.0f, 0.0f, 0.0f, 1.0f),
		FPlane(0.0f, 0.0f, GNearClippingPlane, 0.0f))
		* FTranslationMatrix(FVector(PassProjectionOffset, 0, 0));
}

void FThreeGlassesHMD::InitCanvasFromView(FSceneView* InView, UCanvas* Canvas)
{
}

void FThreeGlassesHMD::GetEyeRenderParams_RenderThread(const FRenderingCompositePassContext& Context, FVector2D& EyeToSrcUVScaleValue, FVector2D& EyeToSrcUVOffsetValue) const
{
	EyeToSrcUVOffsetValue.X = 0.0f;
	EyeToSrcUVOffsetValue.Y = 0.0f;

	EyeToSrcUVScaleValue.X = 1.0f;
	EyeToSrcUVScaleValue.Y = 1.0f;
}


void FThreeGlassesHMD::SetupViewFamily(FSceneViewFamily& InViewFamily)
{
	InViewFamily.EngineShowFlags.MotionBlur = 0;
	InViewFamily.EngineShowFlags.HMDDistortion = Flags.bHmdDistortion;
	InViewFamily.EngineShowFlags.ScreenPercentage = 1.0f;
	InViewFamily.EngineShowFlags.StereoRendering = IsStereoEnabled();
}

void FThreeGlassesHMD::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
	InView.BaseHmdOrientation = LastHmdOrientation;
	InView.BaseHmdLocation = LastHmdPosition;
	WorldToMetersScale = InView.WorldToMetersScale;
	NearClippingPlane = Hmd->CameraFrustumNearZInMeters * WorldToMetersScale;
	FarClippingPlane = Hmd->CameraFrustumFarZInMeters * WorldToMetersScale;
	InViewFamily.bUseSeparateRenderTarget = ShouldUseSeparateRenderTarget();

	int x, y, w, h;
	if (SZVR_GetHmdMonitor(&x, &y, &w, &h) && m_bWndSet == false)
	{
		FVector2D WindowPosition = FVector2D((float)x, (float)y);
		FVector2D WindowSize = FVector2D((float)w, (float)h);
		GEngine->GameViewport->GetWindow()->MoveWindowTo(WindowPosition);
		GEngine->GameViewport->GetWindow()->Resize(WindowSize);
		m_bWndSet = true;
	}
}

void FThreeGlassesHMD::OnBeginPlay(FWorldContext& InWorldContext)
{
}

void FThreeGlassesHMD::PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView)
{
	check(IsInRenderingThread());
}

void FThreeGlassesHMD::PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& ViewFamily)
{
	check(IsInRenderingThread());
}

void FThreeGlassesHMD::Startup()
{
	// grab a pointer to the renderer module for displaying our mirror window
	static const FName RendererModuleName("Renderer");
	IRendererModule *RendererModule = FModuleManager::GetModulePtr<IRendererModule>(RendererModuleName);

	for (int eyeIndex = szvrEye_Left; eyeIndex < szvrEye_Count; ++eyeIndex)
	{
		SZVR_GenerateDistortionMesh(&(DistorMesh[eyeIndex].NumVertices), &(DistorMesh[eyeIndex].NumIndices), eyeIndex == szvrEye_Right);
		DistorMesh[eyeIndex].NumTriangles = DistorMesh[eyeIndex].NumIndices / 3;
		DistorMesh[eyeIndex].pIndices = new int[DistorMesh[eyeIndex].NumIndices];

		FMemory::Memzero(DistorMesh[eyeIndex].pIndices, DistorMesh[eyeIndex].NumIndices * sizeof(int));
		DistorMesh[eyeIndex].pVertices = new DistMeshVert[DistorMesh[eyeIndex].NumVertices];
		DistorMesh[eyeIndex].pVerticesCached = new FDistortionVertex[DistorMesh[eyeIndex].NumVertices];

		FMemory::Memzero(DistorMesh[eyeIndex].pVertices, DistorMesh[eyeIndex].NumVertices * sizeof(DistMeshVert));
		FMemory::Memzero(DistorMesh[eyeIndex].pVerticesCached, DistorMesh[eyeIndex].NumVertices * sizeof(FDistortionVertex));
	}
}

void FThreeGlassesHMD::Shutdown()
{
	SZVR_Destroy();
	Hmd = nullptr;
}

FThreeGlassesHMD::FThreeGlassesHMD() :
	CurHmdOrientation(FQuat::Identity),
	LastHmdOrientation(FQuat::Identity),
	BaseOrientation(FQuat::Identity),
	DeltaControlRotation(FRotator::ZeroRotator),
	DeltaControlOrientation(FQuat::Identity),
	CurHmdPosition(FVector::ZeroVector),
	ScreenPercentage(1.0f),
	Hmd(nullptr)
{
	Flags.Raw = 0;
	Flags.bHmdDistortion = true;
	Flags.bHMDEnabled = true;
	m_bWndSet = false;
	SZVR_Initialize();

	szvrHeadDisplayDevice * pHmd = new szvrHeadDisplayDevice;
	FMemory::Memzero(pHmd, sizeof(szvrHeadDisplayDevice));
	SZVR_GetHeadDisplayDevice(pHmd);
	Hmd = pHmd;

	SZVR_GetInterpupillaryDistance(&InterpupillaryDistance);
	HFOVInRadians = Hmd->CameraFrustumHFovInRadians;
	VFOVInRadians = Hmd->CameraFrustumVFovInRadians;
	Startup();
	InitDevice();
}

FThreeGlassesHMD::~FThreeGlassesHMD()
{
	Shutdown();
}

bool FThreeGlassesHMD::IsInitialized() const
{
	return Hmd != nullptr;
}

bool FThreeGlassesHMD::InitDevice()
{
	UpdateStereoRenderingParams();
	return true;
}

bool FThreeGlassesHMD::IsHMDConnected()
{
	if (IsHMDEnabled())
	{
		return Hmd != nullptr;
	}
	return false;
}

void FThreeGlassesHMD::CalculateRenderTargetSize(const class FViewport& Viewport, uint32& InOutSizeX, uint32& InOutSizeY)
{
	check(IsInGameThread());

	static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("r.ScreenPercentage"));
	float value = CVar->GetValueOnGameThread();
	if (value > 0.0f)
	{
		InOutSizeX = FMath::CeilToInt(InOutSizeX * value / 100.f);
		InOutSizeY = FMath::CeilToInt(InOutSizeY * value / 100.f);
	}
}

bool FThreeGlassesHMD::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	return false;
}

FString FThreeGlassesHMD::GetVersionString() const
{
	return FString(TEXT(""));
}


#endif //THREE_GLASSES_SUPPORTED_PLATFORMS