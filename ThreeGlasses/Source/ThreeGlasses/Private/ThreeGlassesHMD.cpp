#include "ThreeGlassesHMD.h"

#include "Runtime/Renderer/Private/RendererPrivate.h"
#include "Runtime/Renderer/Private/ScenePrivate.h"
#include "Runtime/RenderCore/Public/RenderUtils.h"
#include "Runtime/Engine/Public/ScreenRendering.h"
#include "Runtime/Renderer/Private/PostProcess/PostProcessing.h"
#include "Runtime/Renderer/Private/PostProcess/RenderingCompositionGraph.h"
#include "SceneViewport.h"
#include "DrawDebugHelpers.h"
#include "Runtime/Windows/D3D11RHI/Private/D3D11RHIPrivate.h"
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

typedef bool(*InitDirectModeFunc)(char* ErrorMsg, int msgLength, int &SurfaceFormat);
typedef void(*DirectModePresentFunc)(struct ID3D11Texture2D* srcTexture);
typedef void(*ClearDirectModeFunc)();
InitDirectModeFunc InitDirectMode = NULL;
DirectModePresentFunc DirectModePresent = NULL;
ClearDirectModeFunc ClearDirectMode = NULL;
int DxgiFormat;

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

	MonitorDesc.ResolutionX = 2880;
	MonitorDesc.ResolutionY = 1440;
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

void FThreeGlassesHMD::CopyToMirrorTexture(FRHICommandListImmediate &RHICmdList, const FTextureRHIRef& SrcTexture)
{
	const auto featureLevel = GMaxRHIFeatureLevel;
	auto shaderMap = GetGlobalShaderMap(featureLevel);

	TShaderMapRef<FScreenVS> vertexShader(shaderMap);
	TShaderMapRef<FScreenPS> pixelShader(shaderMap);

	static FGlobalBoundShaderState boundShaderState;
	SetGlobalBoundShaderState(RHICmdList, featureLevel, boundShaderState, RendererModule->GetFilterVertexDeclaration().VertexDeclarationRHI, *vertexShader, *pixelShader);

	pixelShader->SetParameters(RHICmdList, TStaticSamplerState<SF_Bilinear>::GetRHI(), SrcTexture);
	int32 srcSizeX = SrcTexture->GetTexture2D()->GetSizeX();
	int32 srcSizeY = SrcTexture->GetTexture2D()->GetSizeY();
	SetRenderTarget(RHICmdList, MirrorTexture, FTextureRHIRef());

	int32 TargetSizeX = MirrorTexture->GetSizeX();
	int32 TargetSizeY = MirrorTexture->GetSizeY();
	RHICmdList.SetViewport(0, 0, 0, TargetSizeX, TargetSizeY, 1.0f);
	RendererModule->DrawRectangle(
		RHICmdList,
		0, 0, // X, Y
		TargetSizeX, TargetSizeY, // SizeX, SizeY
		0.0f, 0.0f, // U, V
		srcSizeX*0.5, srcSizeY, // SizeU, SizeV
		FIntPoint(TargetSizeX, TargetSizeY), // TargetSize
		FIntPoint(srcSizeX, srcSizeY), // TextureSize
		*vertexShader,
		EDRF_Default);
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

	if ((!Flags.bNotRenderLeft && View.StereoPass == eSSP_LEFT_EYE) || (!Flags.bNotRenderRight && View.StereoPass == eSSP_RIGHT_EYE))
	{
		DrawIndexedPrimitiveUP(Context.RHICmdList, PT_TriangleList, 0, distMesh.NumVertices, distMesh.NumTriangles, distMesh.pIndices,
			sizeof(int), distMesh.pVerticesCached, sizeof(FDistortionVertex));
	}

	if (View.StereoPass == eSSP_LEFT_EYE)
	{
		FRenderingCompositeOutputRef*  OutputRef = Context.Pass->GetInput(ePId_Input0);
		FRenderingCompositePass* Source = *(FRenderingCompositePass**)(OutputRef);
		FRenderingCompositeOutput* Input = Source->GetOutput(OutputRef->GetOutputId());

		TRefCountPtr<IPooledRenderTarget> InputPooledElement = Input->RequestInput();

		check(!InputPooledElement->IsFree());
		const FTextureRHIRef& SrcTexture = InputPooledElement->GetRenderTargetItem().ShaderResourceTexture;

		CopyToMirrorTexture(RHICmdList, SrcTexture);
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
	FSystemResolution::RequestResolutionChange(2880, 1440, EWindowMode::Windowed);

	int32 width = 2880;
	int32 height = 1440;

	FSceneViewport* sceneViewport;
	if (!GIsEditor)
	{
		//UE_LOG(OSVRHMDLog, Warning, TEXT("OSVR getting UGameEngine::SceneViewport viewport"));
		UGameEngine* gameEngine = Cast<UGameEngine>(GEngine);
		sceneViewport = gameEngine->SceneViewport.Get();
	}
#if WITH_EDITOR
	else
	{
		//UE_LOG(OSVRHMDLog, Warning, TEXT("OSVR getting editor viewport"));
		UEditorEngine* editorEngine = CastChecked<UEditorEngine>(GEngine);
		sceneViewport = (FSceneViewport*)editorEngine->GetPIEViewport();
		if (sceneViewport == nullptr || !sceneViewport->IsStereoRenderingAllowed())
		{
			sceneViewport = (FSceneViewport*)editorEngine->GetActiveViewport();
			if (sceneViewport != nullptr && !sceneViewport->IsStereoRenderingAllowed())
			{
				sceneViewport = nullptr;
			}
		}
	}
#endif

	if (!sceneViewport)
	{
		return false;
	}
	else
	{
		//UE_LOG(OSVRHMDLog, Warning, TEXT("OSVR scene viewport exists"));
#if !WITH_EDITOR
		auto window = sceneViewport->FindWindow();
#endif
		if (stereo)
		{
			sceneViewport->SetViewportSize(width, height);
#if !WITH_EDITOR
			if (window.IsValid())
			{
				window->SetViewportSizeDrivenByWindow(false);
			}
#endif
		}
	}

	return Flags.bStereoEnabled;
}

void FThreeGlassesHMD::AdjustViewRect(EStereoscopicPass StereoPass, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const
{
	int PosX = 0, PosY =0, Width=2880, Height=1440;
	/*if (SZVR_GetHmdMonitor(&PosX, &PosY, &Width, &Height))
	{
		SizeX = Width; SizeY = Height;
	}*/
	SizeX = 2880;
	SizeY = 1440;
	SizeX = SizeX / 2;
	if (StereoPass == eSSP_RIGHT_EYE)
	{
		X += SizeX;
	}
}

void FThreeGlassesHMD::CalculateStereoViewOffset(const enum EStereoscopicPass StereoPassType, const FRotator& ViewRotation, const float WorldToMeters, FVector& ViewLocation)
{
	if (StereoPassType != eSSP_FULL)
	{
		float EyeOffset = (GetInterpupillaryDistance() * WorldToMeters) / 2.0f;
		const float PassOffset = (StereoPassType == eSSP_LEFT_EYE) ? -EyeOffset : EyeOffset;
		ViewLocation += ViewRotation.Quaternion().RotateVector(FVector(0, PassOffset, 0));

		const FVector vHMDPosition = DeltaControlOrientation.RotateVector(CurHmdPosition);
		ViewLocation += vHMDPosition;
	}
}

FMatrix FThreeGlassesHMD::GetStereoProjectionMatrix(const enum EStereoscopicPass StereoPassType, const float FOV) const
{
	const float ProjectionCenterOffset = 0.1f;
	const float PassProjectionOffset = (StereoPassType == eSSP_LEFT_EYE) ? ProjectionCenterOffset : -ProjectionCenterOffset;

	return FMatrix(
		FPlane(FMath::Tan(0.5*PI-HFOVInRadians), 0.0f, 0.0f, 0.0f),
		FPlane(0.0f, FMath::Tan(0.5*PI - HFOVInRadians), 0.0f, 0.0f),
		FPlane(0.0f, 0.0f, 0.0f, 1.0f),
		FPlane(0.0f, 0.0f, GNearClippingPlane, 0.0f))
		* FTranslationMatrix(FVector(PassProjectionOffset, 0, 0));
}

void FThreeGlassesHMD::InitCanvasFromView(FSceneView* InView, UCanvas* Canvas)
{
}

void FThreeGlassesHMD::RenderMirrorToBackBuffer(class FRHICommandListImmediate& rhiCmdList, class FRHITexture2D* backBuffer) const
{
	const uint32 viewportWidth = MirrorTexture->GetSizeX();
	const uint32 viewportHeight = MirrorTexture->GetSizeY();

	SetRenderTarget(rhiCmdList, backBuffer, FTextureRHIRef());
	rhiCmdList.SetViewport(0, 0, 0, viewportWidth, viewportHeight, 1.0f);

	rhiCmdList.SetBlendState(TStaticBlendState<>::GetRHI());
	rhiCmdList.SetRasterizerState(TStaticRasterizerState<>::GetRHI());
	rhiCmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());

	const auto featureLevel = GMaxRHIFeatureLevel;
	auto shaderMap = GetGlobalShaderMap(featureLevel);

	TShaderMapRef<FScreenVS> vertexShader(shaderMap);
	TShaderMapRef<FScreenPS> pixelShader(shaderMap);

	static FGlobalBoundShaderState boundShaderState;
	SetGlobalBoundShaderState(rhiCmdList, featureLevel, boundShaderState, RendererModule->GetFilterVertexDeclaration().VertexDeclarationRHI, *vertexShader, *pixelShader);

	pixelShader->SetParameters(rhiCmdList, TStaticSamplerState<SF_Bilinear>::GetRHI(), MirrorTexture);
	RendererModule->DrawRectangle(
		rhiCmdList,
		0, 0, // X, Y
		viewportWidth, viewportHeight, // SizeX, SizeY
		0.0f, 0.0f, // U, V
		1, 1, // SizeU, SizeV
		FIntPoint(viewportWidth, viewportHeight), // TargetSize
		FIntPoint(1, 1), // TextureSize
		*vertexShader,
		EDRF_Default);
}

// Based off of the SteamVR Unreal Plugin implementation.
void FThreeGlassesHMD::RenderTexture_RenderThread(class FRHICommandListImmediate& rhiCmdList, class FRHITexture2D* backBuffer, class FRHITexture2D* srcTexture) const
{
	check(IsInRenderingThread());

	RenderMirrorToBackBuffer(rhiCmdList,backBuffer);

	if (bDirectMode)
	{
		DirectModePresent((ID3D11Texture2D*)srcTexture->GetNativeResource());
	}
	else
		MonitorPresent((ID3D11Texture2D*)srcTexture->GetNativeResource());
	
}

#if PLATFORM_WINDOWS

bool FThreeGlassesHMD::NeedReAllocateViewportRenderTarget(const FViewport &viewport)
{
	check(IsInGameThread());
	
	return false;
}

#endif // PLATFORM_WINDOWS

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

	/*int x=0, y=0, w=2560, h=1440;
	SZVR_GetHmdMonitor(&x, &y, &w, &h);
	FVector2D WindowPosition = FVector2D((float)x, (float)y);
	FVector2D WindowSize = FVector2D((float)w, (float)h);*/

	RECT WindowRect;
	ZeroMemory(&WindowRect, sizeof(WindowRect));
	WindowRect.left = (GetSystemMetrics(SM_CXSCREEN) - MirrorWidth) / 2;
	WindowRect.top = (GetSystemMetrics(SM_CYSCREEN) - MirrorHeight) / 2;
	WindowRect.right = WindowRect.left + MirrorWidth;
	WindowRect.bottom = WindowRect.top + MirrorHeight;
	FVector2D WindowPosition = FVector2D((float)WindowRect.left, (float)WindowRect.top);
	FVector2D WindowSize = FVector2D((float)MirrorWidth, (float)MirrorHeight);
	GEngine->GameViewport->GetWindow()->MoveWindowTo(WindowPosition);
	GEngine->GameViewport->GetWindow()->Resize(WindowSize);
	GEngine->GameViewport->GetWindow()->SetWindowMode(EWindowMode::Windowed);
	GEngine->GameViewport->GetWindow()->SetViewportSizeDrivenByWindow(true);
}

void FThreeGlassesHMD::OnBeginPlay(FWorldContext& InWorldContext)
{
	FApp::SetHasVRFocus(true);
}

void FThreeGlassesHMD::OnEndPlay(FWorldContext& InWorldContext)
{
	FApp::SetHasVRFocus(false);
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
	GEngine->bSmoothFrameRate = false;
	static IConsoleVariable* CVSyncVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync"));
	CVSyncVar->Set(true);

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

#if PLATFORM_WINDOWS
	if (IsPCPlatform(GMaxRHIShaderPlatform) && !IsOpenGLPlatform(GMaxRHIShaderPlatform))
	{
		if (bDirectMode)
		{
			FString DllPath;
			FString PluginPath = FPaths::GamePluginsDir();
			TArray<FString> FileNames;
			IFileManager::Get().FindFilesRecursive(FileNames, *PluginPath, TEXT("ThreeGlasses"), false, true);
			if (FileNames.Num() == 0)
			{
				PluginPath = FPaths::EnginePluginsDir();
				IFileManager::Get().FindFilesRecursive(FileNames, *PluginPath, TEXT("ThreeGlasses"), false, true);
			}
		
			check(FileNames.Num() > 0);
			DllPath = FileNames[0];
		
#if _WIN64
			DllPath.Append(TEXT("\\Source\\ThreeGlasses\\lib\\x64\\D3D11Present.dll"));
#elif _WIN32
			DllPath.Append(TEXT("\\Source\\ThreeGlasses\\lib\\win32\\D3D11Present.dll"));
#endif
			HMODULE hModule = LoadLibrary(*DllPath);
			if (!hModule)
			{
				FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("DirectMode dll load failed!!")));
			}

			InitDirectMode = (InitDirectModeFunc)GetProcAddress(hModule, "InitDirectMode");
			DirectModePresent = (DirectModePresentFunc)GetProcAddress(hModule, "DirectModePresent");
			ClearDirectMode = (ClearDirectModeFunc)GetProcAddress(hModule, "ClearDirectMode");
		
			char Msg[256] = {0};
	
			if (!InitDirectMode(Msg,256,DxgiFormat))
				bDirectMode = false;
		}
		
		if(!bDirectMode)
		{
			InitWindow(GetModuleHandle(NULL));
			DxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		}
	}
#endif
}

void FThreeGlassesHMD::Shutdown()
{
	if (SwapChain) SwapChain->Release();
	if (D3DContext) D3DContext->Release();
	if (Device) Device->Release();

	ClearDirectMode();
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
	SZVR_Initialize();

	szvrHeadDisplayDevice * pHmd = new szvrHeadDisplayDevice;
	FMemory::Memzero(pHmd, sizeof(szvrHeadDisplayDevice));
	SZVR_GetHeadDisplayDevice(pHmd);
	Hmd = pHmd;

	SZVR_GetInterpupillaryDistance(&InterpupillaryDistance);
	HFOVInRadians = PI*110.f/360.f;// Hmd->CameraFrustumHFovInRadians;
	VFOVInRadians = PI*110.f /360.f;// Hmd->CameraFrustumVFovInRadians;
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
	static const FName RendererModuleName("Renderer");
	RendererModule = FModuleManager::GetModulePtr<IRendererModule>(RendererModuleName);

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
	InOutSizeX = 2880;
	InOutSizeY = 1440;
	//SZVR_Get3GlassesResolution((char*)&InOutSizeX, (char*)&InOutSizeY);
}

bool FThreeGlassesHMD::AllocateMirrorTexture() 
{
	auto d3d11RHI = static_cast<FD3D11DynamicRHI*>(GDynamicRHI);
	auto graphicsDevice = reinterpret_cast<ID3D11Device*>(RHIGetNativeDevice());
	ID3D11DeviceContext* context = NULL;
	graphicsDevice->GetImmediateContext(&context);

	HRESULT hr;

	D3D11_TEXTURE2D_DESC textureDesc;
	memset(&textureDesc, 0, sizeof(textureDesc));
	textureDesc.Width = MirrorWidth;
	textureDesc.Height = MirrorHeight;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	// We need it to be both a render target and a shader resource
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

	ID3D11Texture2D *D3DTexture = nullptr;
	hr = graphicsDevice->CreateTexture2D(
		&textureDesc, NULL, &D3DTexture);
	check(!FAILED(hr));

	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	memset(&renderTargetViewDesc, 0, sizeof(renderTargetViewDesc));
	// This must match what was created in the texture to be rendered
	//renderTargetViewDesc.Format = renderTextureDesc.Format;
	renderTargetViewDesc.Format = textureDesc.Format;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;

	// Create the render target view.
	ID3D11RenderTargetView *renderTargetView; //< Pointer to our render target view
	hr = graphicsDevice->CreateRenderTargetView(
		D3DTexture, &renderTargetViewDesc, &renderTargetView);
	check(!FAILED(hr));

	ID3D11ShaderResourceView* shaderResourceView = nullptr;
	bool bCreatedRTVsPerSlice = false;
	int32 rtvArraySize = 1;
	TArray<TRefCountPtr<ID3D11RenderTargetView>> renderTargetViews;
	TRefCountPtr<ID3D11DepthStencilView>* depthStencilViews = nullptr;
	uint32 sizeZ = 0;
	EPixelFormat epFormat = PF_R8G8B8A8;
	bool bCubemap = false;
	bool bPooled = false;
	// override flags
	uint32 flags = TexCreate_RenderTargetable | TexCreate_ShaderResource;

	renderTargetViews.Add(renderTargetView);
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	memset(&shaderResourceViewDesc, 0, sizeof(shaderResourceViewDesc));
	shaderResourceViewDesc.Format = textureDesc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MipLevels = textureDesc.MipLevels;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = textureDesc.MipLevels - 1;

	hr = graphicsDevice->CreateShaderResourceView(
		D3DTexture, &shaderResourceViewDesc, &shaderResourceView);
	check(!FAILED(hr));

	auto targetableTexture = new FD3D11Texture2D(
		d3d11RHI, D3DTexture, shaderResourceView, bCreatedRTVsPerSlice,
		rtvArraySize, renderTargetViews, depthStencilViews,
		textureDesc.Width, textureDesc.Height, sizeZ, 1, 1, epFormat,
		bCubemap, flags, bPooled, FClearValueBinding::Black);

	MirrorTexture = targetableTexture->GetTexture2D();
	
	float color[4] = { 1,1,1,1 };
	context->ClearRenderTargetView(renderTargetView, color);
	context->Flush();
	return true;
}

bool FThreeGlassesHMD::AllocateRenderTargetTexture(
	uint32 index,
	uint32 sizeX, uint32 sizeY,
	uint8 format, uint32 numMips, uint32 flags,
	uint32 targetableTextureFlags,
	FTexture2DRHIRef& outTargetableTexture,
	FTexture2DRHIRef& outShaderResourceTexture,
	uint32 numSamples)
{
	const DXGI_FORMAT platformResourceFormat = (DXGI_FORMAT)DxgiFormat;
	const DXGI_FORMAT platformShaderResourceFormat = FindShaderResourceDXGIFormat(platformResourceFormat, false);
	auto d3d11RHI = static_cast<FD3D11DynamicRHI*>(GDynamicRHI);
	auto graphicsDevice = reinterpret_cast<ID3D11Device*>(RHIGetNativeDevice());
	HRESULT hr;
	D3D11_TEXTURE2D_DESC textureDesc;
	memset(&textureDesc, 0, sizeof(textureDesc));
	textureDesc.Width = sizeX;
	textureDesc.Height = sizeY;
	textureDesc.MipLevels = numMips;
	textureDesc.ArraySize = 1;
	//textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	textureDesc.Format = platformResourceFormat;
	//textureDesc.Format = platformResourceFormat;
	textureDesc.SampleDesc.Count = numSamples;
	textureDesc.SampleDesc.Quality = numSamples > 0 ? D3D11_STANDARD_MULTISAMPLE_PATTERN : 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	// We need it to be both a render target and a shader resource
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

	ID3D11Texture2D *D3DTexture = nullptr;
	hr = graphicsDevice->CreateTexture2D(
		&textureDesc, NULL, &D3DTexture);
	check(!FAILED(hr));

	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	memset(&renderTargetViewDesc, 0, sizeof(renderTargetViewDesc));
	// This must match what was created in the texture to be rendered
	//renderTargetViewDesc.Format = renderTextureDesc.Format;
	//renderTargetViewDesc.Format = textureDesc.Format;
	renderTargetViewDesc.Format = platformShaderResourceFormat;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;

	// Create the render target view.
	ID3D11RenderTargetView *RenderTargetView; //< Pointer to our render target view
	hr = graphicsDevice->CreateRenderTargetView(
		D3DTexture, &renderTargetViewDesc, &RenderTargetView);
	check(!FAILED(hr));


	ID3D11ShaderResourceView* shaderResourceView = nullptr;
	bool bCreatedRTVsPerSlice = false;
	int32 rtvArraySize = 1;
	TArray<TRefCountPtr<ID3D11RenderTargetView>> renderTargetViews;
	TRefCountPtr<ID3D11DepthStencilView>* depthStencilViews = nullptr;
	uint32 sizeZ = 0;
	EPixelFormat epFormat = EPixelFormat(format);
	bool bCubemap = false;
	bool bPooled = false;
	// override flags
	flags = TexCreate_RenderTargetable | TexCreate_ShaderResource;

	renderTargetViews.Add(RenderTargetView);
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	memset(&shaderResourceViewDesc, 0, sizeof(shaderResourceViewDesc));
	//shaderResourceViewDesc.Format = textureDesc.Format;
	shaderResourceViewDesc.Format = platformShaderResourceFormat;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MipLevels = textureDesc.MipLevels;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = textureDesc.MipLevels - 1;

	hr = graphicsDevice->CreateShaderResourceView(
		D3DTexture, &shaderResourceViewDesc, &shaderResourceView);
	check(!FAILED(hr));

	auto targetableTexture = new FD3D11Texture2D(
		d3d11RHI, D3DTexture, shaderResourceView, bCreatedRTVsPerSlice,
		rtvArraySize, renderTargetViews, depthStencilViews,
		textureDesc.Width, textureDesc.Height, sizeZ, numMips, numSamples, epFormat,
		bCubemap, flags, bPooled, FClearValueBinding::Black);

	outTargetableTexture = targetableTexture->GetTexture2D();
	outShaderResourceTexture = targetableTexture->GetTexture2D();

	AllocateMirrorTexture();
	return true;
}

bool FThreeGlassesHMD::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	return false;
}

FString FThreeGlassesHMD::GetVersionString() const
{
	return FString(TEXT(""));
}

void FThreeGlassesHMD::UpdateViewport(bool bUseSeparateRenderTarget, const FViewport& InViewport, class SViewport*)
{
	check(IsInGameThread());
}

void FThreeGlassesHMD::MonitorPresent(ID3D11Texture2D* tex2d) const
{
	HANDLE texHandle = 0;

	IDXGIResource* pResource;
	tex2d->QueryInterface(__uuidof(IDXGIResource), reinterpret_cast<void**>(&pResource));
	pResource->GetSharedHandle(&texHandle);
	pResource->Release();
	ID3D11Texture2D* SrcTexure = NULL;
	HRESULT hr = Device->OpenSharedResource(texHandle, __uuidof(ID3D11Texture2D),
		(LPVOID*)&SrcTexure);

	ID3D11Texture2D* pBackBuffer = NULL;
	hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

	D3DContext->CopyResource(pBackBuffer, SrcTexure);
	D3DContext->Flush();
	SwapChain->Present(1, 0);
	SrcTexure->Release();
	pBackBuffer->Release();
}

LRESULT CALLBACK WndProc(HWND hWnd, uint32 message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

void FThreeGlassesHMD::InitWindow(HINSTANCE hInst)
{
	int Width = 2880;
	int Height = 1440;
	HINSTANCE HInstance = hInst;

	WNDCLASSEX WndClassEx;
	ZeroMemory(&WndClassEx, sizeof(WndClassEx));
	WndClassEx.cbSize = sizeof(WndClassEx);
	WndClassEx.style = CS_HREDRAW | CS_VREDRAW | CS_NOCLOSE;
	WndClassEx.lpfnWndProc = &WndProc;
	WndClassEx.hIcon = 0;
	WndClassEx.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClassEx.hInstance = HInstance;
	WndClassEx.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	WndClassEx.lpszClassName = TEXT("HMD");
	ATOM WndClassAtom = RegisterClassEx(&WndClassEx);

	unsigned long WindowStyle = WS_VISIBLE;

	int winW = Width;
	int winH = Height;
	RECT WindowRect;
	ZeroMemory(&WindowRect, sizeof(WindowRect));
	WindowRect.left = (GetSystemMetrics(SM_CXSCREEN) - winW) / 2;
	WindowRect.top = (GetSystemMetrics(SM_CYSCREEN) - winH) / 2;
	WindowRect.right = WindowRect.left + winW;
	WindowRect.bottom = WindowRect.top + winH;
	AdjustWindowRectEx(&WindowRect, WindowStyle, 0, 0);

	MonitorWindow = CreateWindow(TEXT("HMD"), TEXT("Unreal Engine"),
		WindowStyle, WindowRect.left, WindowRect.top,
		WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top,
		NULL, NULL, HInstance, NULL);

	uint32 createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
	};
	uint32 numFeatureLevels = ARRAYSIZE(featureLevels);

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = Width;
	sd.BufferDesc.Height = Height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 120;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = MonitorWindow;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = 0;
	sd.Windowed = true;

	D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_11_0;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
		D3D11_SDK_VERSION, &sd, &SwapChain, &Device, &g_featureLevel, &D3DContext);
}

#endif //THREE_GLASSES_SUPPORTED_PLATFORMS