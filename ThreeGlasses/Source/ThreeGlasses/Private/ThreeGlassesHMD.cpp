#include "ThreeGlassesHMD.h"

#include "Runtime/Launch/Resources/Version.h"
#include "Runtime/Renderer/Private/RendererPrivate.h"
#include "Runtime/Renderer/Private/ScenePrivate.h"
#include "Runtime/RenderCore/Public/RenderUtils.h"
#include "Runtime/Engine/Public/ScreenRendering.h"
#include "Runtime/Renderer/Private/PostProcess/PostProcessing.h"
#include "Runtime/Renderer/Private/PostProcess/RenderingCompositionGraph.h"
#include "SceneViewport.h"
#include "DrawDebugHelpers.h"
#include "Runtime/Windows/D3D11RHI/Private/D3D11RHIPrivate.h"
#include "Runtime/Core/Public/Windows/WindowsWindow.h"
#include "SDKCompositor.h"
#if WITH_EDITOR
#include "Editor/UnrealEd/Classes/Editor/EditorEngine.h"
#endif

#pragma warning(disable : 4191)

#if THREE_GLASSES_SUPPORTED_PLATFORMS

class FThreeGlassesPlugin : public IThreeGlassesPlugin
{
	/** IHeadMountedDisplayModule implementation */
	virtual TSharedPtr< class IHeadMountedDisplay, ESPMode::ThreadSafe > CreateHeadMountedDisplay() override;

#if ENGINE_MINOR_VERSION >= 14
	virtual FString GetModuleKeyName() const
#else
	virtual FString GetModulePriorityKeyName() const
#endif
	{
		return FString(TEXT("ThreeGlasses"));
	}

public:

};

IMPLEMENT_MODULE(FThreeGlassesPlugin, ThreeGlasses)

static HMODULE hModule = nullptr;
static bool LoadTrackDll()
{
	FString TrackerParh;
	FString PluginPath = FPaths::GamePluginsDir();
	TArray<FString> FileNames;
	IFileManager::Get().FindFilesRecursive(FileNames, *PluginPath, TEXT("ThreeGlasses"), false, true);

	if (FileNames.Num()==0)
	{
		PluginPath = FPaths::EnginePluginsDir();
		IFileManager::Get().FindFilesRecursive(FileNames, *PluginPath, TEXT("ThreeGlasses"), false, true);
	}

	TrackerParh = FileNames[0];
	TrackerParh.Append(TEXT("\\Source\\ThreeGlasses\\3GlassesTracker.dll"));
	
	hModule = LoadLibrary(*TrackerParh);
	if (!hModule)
	{
		return false;
	}

	return true;
}

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
}

EHMDDeviceType::Type FThreeGlassesHMD::GetHMDDeviceType() const
{
	return EHMDDeviceType::DT_ES2GenericStereoMesh;
}

bool FThreeGlassesHMD::GetHMDMonitorInfo(MonitorInfo& MonitorDesc)
{
	MonitorDesc.MonitorName = FString(TEXT("ThreeGlasses"));
	MonitorDesc.MonitorId = 0;
	MonitorDesc.DesktopX = HMDDesktopX;
	MonitorDesc.DesktopY = HMDDesktopY;
	MonitorDesc.ResolutionX = MonitorDesc.WindowSizeX = HMDResX;
	MonitorDesc.ResolutionY = MonitorDesc.WindowSizeY = HMDResY;
	return false;
}

void FThreeGlassesHMD::GetFieldOfView(float& OutHFOVInDegrees, float& OutVFOVInDegrees) const
{
	OutHFOVInDegrees = 0.0f;
	OutVFOVInDegrees = 0.0f;
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
		float data[4] = { 0,0,0,0 };
		FQuat quat = { 0,0,0,1 };
		if (SZVR_GetHMDRotate(data) == 0)
		{
			quat.Y = -data[0];
			quat.Z = -data[1];
			quat.X = data[2];
			quat.W = data[3];

			if (quat.IsNormalized())
			{
				LastOrientation = quat;
			}
		}

		FVector loc(0, 0, 0);
		if (SZVR_GetHMDPos(&loc.X) == 0)
		{
			LastPosition = FVector(-loc.Z, loc.X, loc.Y)/7.f;
			LastPosition = ClampVector(LastPosition, -FVector(HALF_WORLD_MAX, HALF_WORLD_MAX, HALF_WORLD_MAX), FVector(HALF_WORLD_MAX, HALF_WORLD_MAX, HALF_WORLD_MAX));
		}
		CurrentHmdPosition = LastPosition;
		CurrentHmdOrientation = LastOrientation;
	}
}

void FThreeGlassesHMD::GetCurrentOrientationAndPosition(FQuat& CurrentOrientation, FVector& CurrentPosition)
{
	check(IsInGameThread());
	GetCurrentPose(CurrentOrientation, CurrentPosition);
}

TSharedPtr<ISceneViewExtension, ESPMode::ThreadSafe> FThreeGlassesHMD::GetViewExtension()
{
	TSharedPtr<FThreeGlassesHMD, ESPMode::ThreadSafe> ptr(AsShared());
	return StaticCastSharedPtr<ISceneViewExtension>(ptr);
}

void FThreeGlassesHMD::ApplyHmdRotation(APlayerController* PC, FRotator& ViewRotation)
{
	ViewRotation.Normalize();

	FQuat CurHmdOrientation;
	FVector CurHmdPosition;
	GetCurrentPose(CurHmdOrientation, CurHmdPosition);

	const FRotator DeltaRot = ViewRotation - PC->GetControlRotation();
	DeltaControlRotation = (DeltaControlRotation + DeltaRot).GetNormalized();

	// Pitch from other sources is never good, because there is an absolute up and down that must be respected to avoid motion sickness.
	// Same with roll.
	DeltaControlRotation.Pitch = 0;
	DeltaControlRotation.Roll = 0;
	DeltaControlOrientation = DeltaControlRotation.Quaternion();

	ViewRotation = FRotator(DeltaControlOrientation * CurHmdOrientation);
}

bool FThreeGlassesHMD::UpdatePlayerCamera(FQuat& CurrentOrientation, FVector& CurrentPosition)
{
	GetCurrentPose(CurrentOrientation, CurrentPosition);

	if (!bImplicitHmdPosition && GEnableVREditorHacks)
	{
		DeltaControlOrientation = CurrentOrientation;
		DeltaControlRotation = DeltaControlOrientation.Rotator();
	}

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
	return
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
}

void FThreeGlassesHMD::DrawDistortionMesh_RenderThread(struct FRenderingCompositePassContext& Context, const FIntPoint& TextureSize)
{
	check(IsInRenderingThread());
}

#if !UE_BUILD_SHIPPING
static void RenderLines(FCanvas* Canvas, int numLines, const FColor& c, float* x, float* y)
{
	for (int i = 0; i < numLines; ++i)
	{
		FCanvasLineItem line(FVector2D((x[i * 2] / 2 + 0.5) * 1280, (-y[i * 2] / 2 + 0.5) * 720), FVector2D((x[i * 2 + 1] / 2 + 0.5) * 1280, (-y[i * 2 + 1] / 2 + 0.5) * 720));
		line.SetColor(FLinearColor(c));
		Canvas->DrawItem(line);
	}
}
#endif // #if !UE_BUILD_SHIPPING

void FThreeGlassesHMD::DrawDebug(UCanvas* Canvas)
{
#if !UE_BUILD_SHIPPING
#endif //!UE_BUILD_SHIPPING
}

bool FThreeGlassesHMD::IsStereoEnabled() const
{
	return Flags.bStereoEnabled && Flags.bHMDEnabled;
}

bool FThreeGlassesHMD::EnableStereo(bool stereo)
{
	bool NewEnable = (IsHMDEnabled()) ? stereo : false;
	if (Flags.bStereoEnabled == NewEnable)
		return Flags.bStereoEnabled;

	Flags.bStereoEnabled = NewEnable;
	FSceneViewport* sceneViewport;
	if (!GIsEditor)
	{
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

	if (Flags.bStereoEnabled)
	{
		auto window = sceneViewport->FindWindow();

		sceneViewport->SetViewportSize(HMDResX, HMDResY);
		if (window.IsValid())
		{
			window->SetViewportSizeDrivenByWindow(false);
			window->FlashWindow();
		}
	}
	else
	{
		sceneViewport->SetViewportSize(GSystemResolution.ResX, GSystemResolution.ResY);
		auto window = sceneViewport->FindWindow();

		if (window.IsValid())
		{
			window->SetViewportSizeDrivenByWindow(true);
		}
	}

	return Flags.bStereoEnabled;
}

void FThreeGlassesHMD::AdjustViewRect(EStereoscopicPass StereoPass, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const
{
	SizeX = HMDResX;
	SizeY = HMDResY;
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
	
		if (!bImplicitHmdPosition)
		{
			FQuat CurHmdOrientation;
			FVector CurHmdPosition;
			GetCurrentPose(CurHmdOrientation, CurHmdPosition);
			const FVector vHMDPosition = DeltaControlOrientation.RotateVector(CurHmdPosition);
			ViewLocation += vHMDPosition;
		}
	}
}

FMatrix FThreeGlassesHMD::GetStereoProjectionMatrix(const enum EStereoscopicPass StereoPassType, const float FOV) const
{
	float Left, Right, Top, Bottom;
	SDKCompositorHMDFOVTanAngles(&Left, &Top, &Right, &Bottom);

	float ZNear = GNearClippingPlane;

	float SumRL = (Right + Left);
	float SumTB = (Top + Bottom);
	float InvRL = (1.0f / (Right - Left));
	float InvTB = (1.0f / (Top - Bottom));

	return FMatrix(
		FPlane((2.0f * InvRL), 0.0f, 0.0f, 0.0f),
		FPlane(0.0f, (2.0f * InvTB), 0.0f, 0.0f),
		FPlane((SumRL * InvRL), (SumTB * InvTB), 0.0f, 1.0f),
		FPlane(0.0f, 0.0f, ZNear, 0.0f)
	);
}

void FThreeGlassesHMD::InitCanvasFromView(FSceneView* InView, UCanvas* Canvas)
{
}

void FThreeGlassesHMD::RenderMirrorToBackBuffer(class FRHICommandListImmediate& rhiCmdList, class FRHITexture2D* srcTexture, class FRHITexture2D* backBuffer) const
{
	bool bSingleStereo = false;

	int viewportWidth = backBuffer->GetSizeX();
	int viewportHeight = backBuffer->GetSizeY();
	uint32 offset = 0;
	float SizeU = 1.0f;

	if (bSingleStereo)
	{
		viewportWidth = viewportWidth/2;
		offset = FMath::Max((int)backBuffer->GetSizeX() - viewportWidth, 0) / 2;
		SizeU = 0.5f;
	}

	FRHIRenderTargetView ColorView(backBuffer, 0, -1, ERenderTargetLoadAction::EClear, ERenderTargetStoreAction::EStore);
	FRHIDepthRenderTargetView DepthView(NULL, ERenderTargetLoadAction::ENoAction, ERenderTargetStoreAction::ENoAction, FExclusiveDepthStencil::DepthNop_StencilNop);
	FRHISetRenderTargetsInfo Info(1, &ColorView, DepthView);

	rhiCmdList.SetRenderTargetsAndClear(Info);

	rhiCmdList.SetViewport(offset, 0, 0, viewportWidth + offset, viewportHeight, 1.0f);

	rhiCmdList.SetBlendState(TStaticBlendState<>::GetRHI());
	rhiCmdList.SetRasterizerState(TStaticRasterizerState<>::GetRHI());
	rhiCmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());

	const auto featureLevel = GMaxRHIFeatureLevel;
	auto shaderMap = GetGlobalShaderMap(featureLevel);

	TShaderMapRef<FScreenVS> vertexShader(shaderMap);
	TShaderMapRef<FScreenPS> pixelShader(shaderMap);

	static FGlobalBoundShaderState boundShaderState;
	SetGlobalBoundShaderState(rhiCmdList, featureLevel, boundShaderState, RendererModule->GetFilterVertexDeclaration().VertexDeclarationRHI, *vertexShader, *pixelShader);

	pixelShader->SetParameters(rhiCmdList, TStaticSamplerState<SF_Bilinear>::GetRHI(), srcTexture);
	RendererModule->DrawRectangle(
		rhiCmdList,
		0, 0, // X, Y
		viewportWidth, viewportHeight, // SizeX, SizeY
		0.0f, 0.0f, // U, V
		SizeU, 1, // SizeU, SizeV
		FIntPoint(viewportWidth, viewportHeight), // TargetSize
		FIntPoint(1, 1), // TextureSize
		*vertexShader,
		EDRF_Default);
}

// Based off of the SteamVR Unreal Plugin implementation.
void FThreeGlassesHMD::RenderTexture_RenderThread(class FRHICommandListImmediate& rhiCmdList, class FRHITexture2D* backBuffer, class FRHITexture2D* srcTexture) const
{
	check(IsInRenderingThread());

	RenderMirrorToBackBuffer(rhiCmdList, srcTexture, backBuffer);
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
	GetCurrentPose(InView.BaseHmdOrientation, InView.BaseHmdLocation);
	WorldToMetersScale = InView.WorldToMetersScale;
	InViewFamily.bUseSeparateRenderTarget = ShouldUseSeparateRenderTarget();
}

void FThreeGlassesHMD::OnBeginPlay(FWorldContext& InWorldContext)
{
	if (!mCurrentPresent)
	{
		mCurrentPresent = new D3D11Present();
		SDKCompositorInit(hInstance);
	}
}

void FThreeGlassesHMD::OnEndPlay(FWorldContext& InWorldContext)
{
	EnableStereo(false);
	mCurrentPresent = nullptr;
	SDKCompositorDestroy();
}

void FThreeGlassesHMD::PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& View)
{
	check(IsInRenderingThread());

	FQuat HmdOrientation;
	FVector HmdPosition;
	GetCurrentPose(HmdOrientation, HmdPosition);

	// The last view location used to set the view will be in BaseHmdOrientation.  We need to calculate the delta from that, so that
	// cameras that rely on game objects (e.g. other components) for their positions don't need to be updated on the render thread.
	const FQuat DeltaOrient = View.BaseHmdOrientation.Inverse() * HmdOrientation;
	View.ViewRotation = FRotator(View.ViewRotation.Quaternion() * DeltaOrient);

	if (bImplicitHmdPosition)
	{
		const FQuat LocalDeltaControlOrientation = View.ViewRotation.Quaternion() * HmdOrientation.Inverse();
		const FVector DeltaPosition = HmdPosition - View.BaseHmdLocation;
		View.ViewLocation += LocalDeltaControlOrientation.RotateVector(DeltaPosition);
	}

	View.UpdateViewMatrix();
	SDKCompositorStereoRenderBegin();
}

void FThreeGlassesHMD::PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& ViewFamily)
{
	check(IsInRenderingThread());

	const FSceneView* MainView = ViewFamily.Views[0];

	const FTransform OldRelativeTransform(MainView->BaseHmdOrientation, MainView->BaseHmdLocation);

	FVector NewPosition;
	FQuat NewOrientation;
	GetCurrentPose(NewOrientation, NewPosition);
	const FTransform NewRelativeTransform(NewOrientation, NewPosition);

	ApplyLateUpdate(ViewFamily.Scene, OldRelativeTransform, NewRelativeTransform);
}

FThreeGlassesHMD::FThreeGlassesHMD() :
	BaseOrientation(FQuat::Identity),
	DeltaControlRotation(FRotator::ZeroRotator),
	DeltaControlOrientation(FQuat::Identity),
	ScreenPercentage(1.0f),
	AspectRatio(2)
{
	Flags.Raw = 0;
	Flags.bHmdDistortion = true;
	Flags.bHMDEnabled = true;

	InterpupillaryDistance = 0.1;
	bVsyncOn = true;
	bIsMotionPredictionOn = true;
	MotionPredictionFactor = 1.367f;
	HFOV = 45;
	GazePlane = 340.0f;

	bool bHMDConnected = false;
	bool bServerOpen = false;

	// load server process
	bServerOpen = SZVR_GetHMDConnectionStatus(&bHMDConnected) == 0;

	if(!bServerOpen)
	{
		if (LoadTrackDll())
		{
			Sleep(1000);
			bServerOpen = SZVR_GetHMDConnectionStatus(&bHMDConnected) == 0;
		}
	}
	
	if (bServerOpen&&bHMDConnected)
	{
		SDKCompositorHMDRenderInfo(&HMDDesktopX, &HMDDesktopY, &HMDResX, &HMDResY);

		HMDResX = HMDResX - HMDDesktopX;
		HMDResY = HMDResY - HMDDesktopY;
		AspectRatio = (float)HMDResX / (float)HMDResY;
	}	
	else
	{
		if (!bServerOpen)
		{
			::MessageBox(0, L"3Glasses Tracker service is not Open!!", L"Error", MB_OK);
		}
		
		if(!bHMDConnected)
		{
			::MessageBox(0, L"HMDDevice is not Connect!!", L"Error", MB_OK);
		}
		
		Flags.bHMDEnabled = false;
	}

	static const FName RendererModuleName("Renderer");
	RendererModule = FModuleManager::GetModulePtr<IRendererModule>(RendererModuleName);
}

FThreeGlassesHMD::~FThreeGlassesHMD()
{
	FreeLibrary(hModule);
}

bool FThreeGlassesHMD::IsInitialized() const
{
	return true;
}

bool FThreeGlassesHMD::IsHMDConnected()
{
	if (IsHMDEnabled())
	{
		return HMDResX != 0;
	}
	return false;
}

void FThreeGlassesHMD::CalculateRenderTargetSize(const class FViewport& Viewport, uint32& InOutSizeX, uint32& InOutSizeY)
{
	check(IsInGameThread());
	InOutSizeX = HMDResX;
	InOutSizeY = HMDResY;
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
	auto graphicsDevice = reinterpret_cast<ID3D11Device*>(RHIGetNativeDevice());
	HANDLE handle = SDKCompositorStereoRenderGetRenderTarget();
	ID3D11Texture2D *D3DTexture = nullptr;
	HRESULT hr = graphicsDevice->OpenSharedResource(handle, __uuidof(ID3D11Texture2D),
		(LPVOID*)&D3DTexture);

	D3D11_TEXTURE2D_DESC Desc;
	D3DTexture->GetDesc(&Desc);
	const DXGI_FORMAT platformShaderResourceFormat = FindShaderResourceDXGIFormat(Desc.Format, false);
	auto d3d11RHI = static_cast<FD3D11DynamicRHI*>(GDynamicRHI);
	
	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	memset(&renderTargetViewDesc, 0, sizeof(renderTargetViewDesc));
	// This must match what was created in the texture to be rendered
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
	shaderResourceViewDesc.Texture2D.MipLevels = Desc.MipLevels;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = Desc.MipLevels - 1;

	hr = graphicsDevice->CreateShaderResourceView(
		D3DTexture, &shaderResourceViewDesc, &shaderResourceView);
	check(!FAILED(hr));

	auto targetableTexture = new FD3D11Texture2D(
		d3d11RHI, D3DTexture, shaderResourceView, bCreatedRTVsPerSlice,
		rtvArraySize, renderTargetViews, depthStencilViews,
		Desc.Width, Desc.Height, sizeZ, numMips, numSamples, epFormat,
		bCubemap, flags, bPooled, FClearValueBinding::Black);

	outTargetableTexture = targetableTexture->GetTexture2D();
	outShaderResourceTexture = targetableTexture->GetTexture2D();

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
	FRHIViewport* const ViewportRHI = InViewport.GetViewportRHI().GetReference();
	if (!ViewportRHI)
	{
		return;
	}

	if (!IsStereoEnabled())
	{
		if (!bUseSeparateRenderTarget)
		{
			ViewportRHI->SetCustomPresent(nullptr);
		}
	}
	else
	{
		ViewportRHI->SetCustomPresent(mCurrentPresent);
	}
}

void FThreeGlassesHMD::SetMotionPredictionFactor(bool bPredictionOn, bool bOpenVsync, float scale, int maxFps)
{
	if (scale <= 0)
		return;

	bIsMotionPredictionOn = bPredictionOn;
	MotionPredictionFactor = scale;
	SetVsync(bOpenVsync, maxFps);
}

bool FThreeGlassesHMD::D3D11Present::Present(int32& InOutSyncInterval)
{
	return true;
}

FThreeGlassesHMD::D3D11Present::~D3D11Present()
{
	Sleep(1);
}

void  FThreeGlassesHMD::D3D11Present::PostPresent()
{
	SDKCompositorStereoRenderEnd();
}

void FThreeGlassesHMD::SetVsync(bool bOpenVsync, float maxFps)
{
	bVsyncOn = bOpenVsync;
	static IConsoleVariable* CVSyncVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync"));
	static IConsoleVariable* CMaxFPSVar = IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS"));
	CMaxFPSVar->Set(maxFps);
	int32 value = bVsyncOn;
	if (value != CVSyncVar->GetInt())
	{
		CVSyncVar->Set(value);
		UE_LOG(LogClass, Log, TEXT("Vsync Stat: %d"), value);
	}
}

void FThreeGlassesHMD::SetStereoEffectParam(float fov, float gazePlane)
{
	HFOV = fov;
	GazePlane = gazePlane;
}
#endif //THREE_GLASSES_SUPPORTED_PLATFORMS