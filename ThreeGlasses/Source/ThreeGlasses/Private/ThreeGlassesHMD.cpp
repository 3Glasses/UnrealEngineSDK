﻿#include "ThreeGlassesHMD.h"

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

typedef int(*DevicesFunc)();
typedef int(*StartTrackingFunc)(void* hmdCallBack, void* wandCallBack);
DevicesFunc InitDevices = NULL;
DevicesFunc FinishTracking = NULL;
DevicesFunc ReleaseDevices = NULL;
StartTrackingFunc StartTracking = NULL;

static bool LoadTrackDll()
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

	DllPath.Append(TEXT("\\Source\\ThreeGlasses\\lib\\x64\\3GlassesTracker.dll"));
	HMODULE hModule = LoadLibrary(*DllPath);
	if (!hModule)
	{
		return false;
	}

	InitDevices = (DevicesFunc)GetProcAddress(hModule, "InitDevices");
	StartTracking = (StartTrackingFunc)GetProcAddress(hModule, "StartTracking");
	FinishTracking = (DevicesFunc)GetProcAddress(hModule, "FinishTracking");
	ReleaseDevices = (DevicesFunc)GetProcAddress(hModule, "ReleaseDevices");

	if (InitDevices&&StartTracking&&FinishTracking&&ReleaseDevices)
	{
		return true;
	}

	return false;
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
	return EHMDDeviceType::DT_OculusRift;
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
	OutHFOVInDegrees = FMath::RadiansToDegrees(HFOV);
	OutVFOVInDegrees = FMath::RadiansToDegrees(HFOV);
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
		if (SZVR_GetHMDRotate(data))
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
		if (SZVR_GetHMDPos(&loc.X))
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

FVector GetPos(float i, float j, uint32_t GirdNum)
{
	auto unitUV = 1.0f / GirdNum;
	return FVector(i*unitUV * 2 - 1, 1.0f - j*unitUV * 2, 0.5f);
}

void GetMash(TArray<FVector>& vertices, TArray<uint16>& indices, uint32_t GirdNum)
{
	for (uint32_t j = 0; j < GirdNum + 1; j++)
	{
		for (uint32_t i = 0; i < GirdNum + 1; i++)
		{
			vertices.Add(GetPos(static_cast<float>(i), static_cast<float>(j), GirdNum));
		}
	}
	auto mid = GirdNum / 2;
	for (uint32_t j = 0; j < GirdNum; j++)
	{
		for (uint32_t i = 0; i < GirdNum; i++)
		{
			if ((i < mid&&j < mid) || (i >= mid&&j >= mid))
			{
				indices.Add(i + j*(GirdNum + 1));
				indices.Add(i + 1 + j*(GirdNum + 1));
				indices.Add(i + (j + 1)*(GirdNum + 1));
				indices.Add(i + (j + 1)*(GirdNum + 1));
				indices.Add(i + 1 + j*(GirdNum + 1));
				indices.Add(i + 1 + (j + 1)*(GirdNum + 1));
			}
			else
			{
				indices.Add(i + j*(GirdNum + 1));
				indices.Add(i + 1 + j*(GirdNum + 1));
				indices.Add(i + 1 + (j + 1)*(GirdNum + 1));
				indices.Add(i + j*(GirdNum + 1));
				indices.Add(i + 1 + (j + 1)*(GirdNum + 1));
				indices.Add(i + (j + 1)*(GirdNum + 1));
			}
		}
	}
}

void FThreeGlassesHMD::UpdateStereoRenderingParams()
{
	if (IsInitialized())
	{
		TArray<FVector> vertices;
		TArray<uint16> Indices;
		int GirdNum = 64;
		GetMash(vertices, Indices, GirdNum);

		for (int eyeIndex = szvrEye_Left; eyeIndex < szvrEye_Count; ++eyeIndex)
		{
			//SZVR_CopyDistortionMesh(DistorMesh[eyeIndex].pVertices, DistorMesh[eyeIndex].pIndices, &DummyDistScale, eyeIndex == szvrEye_Right);
			int VertNum = vertices.Num();
			int IndexNum = Indices.Num();
			DistorMesh[eyeIndex].NumVertices = VertNum;
			DistorMesh[eyeIndex].NumIndices = IndexNum;
			DistorMesh[eyeIndex].NumTriangles = IndexNum / 3;
			DistorMesh[eyeIndex].pVerticesCached = new FDistortionVertex[VertNum];
			DistorMesh[eyeIndex].pIndices = new int[IndexNum];
			for (int i = 0; i < VertNum; ++i)
			{
				DistorMesh[eyeIndex].pVerticesCached[i].Position.X = vertices[i].X*0.5f - 0.5f + eyeIndex;
				DistorMesh[eyeIndex].pVerticesCached[i].Position.Y = vertices[i].Y;

				DistorMesh[eyeIndex].pVerticesCached[i].VignetteFactor = 1.0f;
				DistorMesh[eyeIndex].pVerticesCached[i].TimewarpFactor = 0;
				FVector2D uv = { (vertices[i].X / 2.0f) + 0.5f, 0.5f - vertices[i].Y / 2.0f };

				SZVR_GetDistortion(HMDResX, &uv.X, &DistorMesh[eyeIndex].pVerticesCached[i].TexR.X, &DistorMesh[eyeIndex].pVerticesCached[i].TexG.X, &DistorMesh[eyeIndex].pVerticesCached[i].TexB.X);
				DistorMesh[eyeIndex].pVerticesCached[i].TexR.X = DistorMesh[eyeIndex].pVerticesCached[i].TexR.X*0.5f + 0.5f*eyeIndex;
				DistorMesh[eyeIndex].pVerticesCached[i].TexG.X = DistorMesh[eyeIndex].pVerticesCached[i].TexG.X*0.5f + 0.5f*eyeIndex;
				DistorMesh[eyeIndex].pVerticesCached[i].TexB.X = DistorMesh[eyeIndex].pVerticesCached[i].TexB.X*0.5f + 0.5f*eyeIndex;
			}

			for (int i = 0; i < IndexNum; i++)
			{
				DistorMesh[eyeIndex].pIndices[i] = Indices[i];
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
		//FRenderingCompositeOutput* Input = OutputRef->GetOutput();

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
		FCanvasLineItem line(FVector2D((x[i * 2] / 2 + 0.5) * 1280, (-y[i * 2] / 2 + 0.5) * 720), FVector2D((x[i * 2 + 1] / 2 + 0.5) * 1280, (-y[i * 2 + 1] / 2 + 0.5) * 720));
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
		FQuat CurHmdOrientation;
		FVector CurHmdPosition;
		GetCurrentPose(CurHmdOrientation, CurHmdPosition);
		const FVector vHMDPosition = DeltaControlOrientation.RotateVector(CurHmdPosition);
		ViewLocation += vHMDPosition;
	}
}

FMatrix FThreeGlassesHMD::GetStereoProjectionMatrix(const enum EStereoscopicPass StereoPassType, const float FOV) const
{
	//const float ProjectionCenterOffset = 0.1f;
	//const float PassProjectionOffset = (StereoPassType == eSSP_LEFT_EYE) ? ProjectionCenterOffset : -ProjectionCenterOffset;

	//return FMatrix(
	//	FPlane(FMath::Tan(0.5*PI - HFOVInRadians), 0.0f, 0.0f, 0.0f),
	//	FPlane(0.0f, FMath::Tan(0.5*PI - HFOVInRadians), 0.0f, 0.0f),
	//	FPlane(0.0f, 0.0f, 0.0f, 1.0f),
	//	FPlane(0.0f, 0.0f, GNearClippingPlane, 0.0f))
	//	* FTranslationMatrix(FVector(PassProjectionOffset, 0, 0));

	float ZNear = GNearClippingPlane;
	float wd = ZNear * FMath::Abs(FMath::Tan(HFOV * PI / 180.0f * 0.5f) * 2.0f);
	float r = wd;
	float l = -wd;
	float t = wd / (AspectRatio * 0.5f);
	float b = -wd / (AspectRatio * 0.5f);

	float offest = GetInterpupillaryDistance() * WorldToMetersScale * 0.5f / GazePlane;
	if (StereoPassType == eSSP_RIGHT_EYE)//eSSP_LEFT_EYE eSSP_RIGHT_EYE
	{
		r = r - offest;
		l = l - offest;
	}
	else
	{
		r = r + offest;
		l = l + offest;
	}

	float SumRL = (r + l);
	float SumTB = (t + b);
	float InvRL = (1.0f / (r - l));
	float InvTB = (1.0f / (t - b));

	FMatrix Mat = FMatrix(
		FPlane((2.0f * ZNear * InvRL), 0.0f, 0.0f, 0.0f),
		FPlane(0.0f, (2.0f * ZNear * InvTB), 0.0f, 0.0f),
		FPlane((SumRL * InvRL), (SumTB * InvTB), 0.0f, 1.0f),
		FPlane(0.0f, 0.0f, ZNear, 0.0f)
	);

	return Mat;
}

void FThreeGlassesHMD::InitCanvasFromView(FSceneView* InView, UCanvas* Canvas)
{
}

void FThreeGlassesHMD::RenderMirrorToBackBuffer(class FRHICommandListImmediate& rhiCmdList, class FRHITexture2D* backBuffer) const
{
	const int viewportWidth = backBuffer->GetSizeY();
	const int viewportHeight = backBuffer->GetSizeY();

	FRHIRenderTargetView ColorView(backBuffer, 0, -1, ERenderTargetLoadAction::EClear, ERenderTargetStoreAction::EStore);
	FRHIDepthRenderTargetView DepthView(NULL, ERenderTargetLoadAction::ENoAction, ERenderTargetStoreAction::ENoAction, FExclusiveDepthStencil::DepthNop_StencilNop);
	FRHISetRenderTargetsInfo Info(1, &ColorView, DepthView);

	rhiCmdList.SetRenderTargetsAndClear(Info);

	uint32 offset = FMath::Max((int)backBuffer->GetSizeX() - viewportWidth, 0) / 2;
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

	RenderMirrorToBackBuffer(rhiCmdList, backBuffer);
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
	GetCurrentPose(InView.BaseHmdOrientation, InView.BaseHmdLocation);
	WorldToMetersScale = InView.WorldToMetersScale;
	InViewFamily.bUseSeparateRenderTarget = ShouldUseSeparateRenderTarget();
}

void FThreeGlassesHMD::OnBeginPlay(FWorldContext& InWorldContext)
{
	FApp::SetHasVRFocus(true);
}

void FThreeGlassesHMD::OnEndPlay(FWorldContext& InWorldContext)
{
	FApp::SetHasVRFocus(false);
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

void FThreeGlassesHMD::Startup()
{
	static const FName RendererModuleName("Renderer");
	RendererModule = FModuleManager::GetModulePtr<IRendererModule>(RendererModuleName);

	UpdateStereoRenderingParams();

	// grab a pointer to the renderer module for displaying our mirror window
	GEngine->bSmoothFrameRate = false;
	static IConsoleVariable* CVSyncVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync"));
	CVSyncVar->Set(true);

	SetVsync(bVsyncOn, 120.f);
}

void FThreeGlassesHMD::Shutdown()
{
	if(MonitorWindow)
		DestroyWindow(MonitorWindow);

	UnregisterClass(TEXT("HMD"), GetModuleHandle(NULL));
	if (SwapChain)
	{
		SwapChain->SetFullscreenState(false, nullptr);
		SwapChain->Release();
		SwapChain = NULL;
	}

	if (D3DContext)
	{
		D3DContext->ClearState();
		D3DContext->Release();
		D3DContext = NULL;
	}

	if (Device)
	{
		Device->Release();
		Device = NULL;
	}

	if (FinishTracking)
		FinishTracking();
	if (ReleaseDevices)
		ReleaseDevices();
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
	HFOV = 65.0f;
	GazePlane = 340.0f;

	// load server dll
	if (LoadTrackDll())
	{
		InitDevices();
		StartTracking(NULL, NULL);
	}

	RECT rect;
	if (SZVR_FindHMDRect(rect))
	{
		HMDDesktopX = rect.left;
		HMDDesktopY = rect.top;
		HMDResX = rect.right - rect.left;
		HMDResY = rect.bottom - rect.top;
		AspectRatio = (float)HMDResX / (float)HMDResY;
		InitWindow(GetModuleHandle(NULL));
	}
	else
	{
		::MessageBox(0, L"HMDDevice is not Connect!!", L"Error", MB_OK);
		Flags.bHMDEnabled = false;
	}

	Startup();
}

FThreeGlassesHMD::~FThreeGlassesHMD()
{
	Shutdown();
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
	textureDesc.Width = 800;
	textureDesc.Height = 800;
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
	textureDesc.Format = platformResourceFormat;
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
	if (!Device)
	{
		return;
	}

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
	int Width = HMDResX;
	int Height = HMDResY;

	int X = HMDDesktopX, Y = HMDDesktopY;

	HINSTANCE HInstance = hInst;

	WNDCLASSEX WndClassEx;
	ZeroMemory(&WndClassEx, sizeof(WndClassEx));
	WndClassEx.cbSize = sizeof(WndClassEx);
	WndClassEx.style = CS_NOCLOSE;
	WndClassEx.lpfnWndProc = &WndProc;
	WndClassEx.hIcon = 0;
	WndClassEx.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClassEx.hInstance = HInstance;
	WndClassEx.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	WndClassEx.lpszClassName = TEXT("HMD");
	ATOM WndClassAtom = RegisterClassEx(&WndClassEx);

	unsigned long WindowStyle = WS_VISIBLE | WS_POPUP;

	int winW = Width;
	int winH = Height;

	RECT WindowRect;
	ZeroMemory(&WindowRect, sizeof(WindowRect));
	WindowRect.left = X;
	WindowRect.top = Y;
	WindowRect.right = WindowRect.left + winW;
	WindowRect.bottom = WindowRect.top + winH;
	AdjustWindowRectEx(&WindowRect, WindowStyle, 0, 0);

	MonitorWindow = CreateWindow(TEXT("HMD"), TEXT("Unreal Engine"),
		WindowStyle, WindowRect.left, WindowRect.top,
		WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top,
		NULL, NULL, HInstance, NULL);

	SetWindowLong(MonitorWindow, GWL_EXSTYLE, GetWindowLong(MonitorWindow, GWL_EXSTYLE) | WS_EX_NOACTIVATE);
	//ShowWindow(MonitorWindow, SW_HIDE);
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
	sd.BufferDesc.RefreshRate.Numerator = 0;
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

	SwapChain->SetFullscreenState(true, NULL);
}

void FThreeGlassesHMD::SetMotionPredictionFactor(bool bPredictionOn, bool bOpenVsync, float scale, int maxFps)
{
	if (scale <= 0)
		return;

	bIsMotionPredictionOn = bPredictionOn;
	MotionPredictionFactor = scale;
	SetVsync(bOpenVsync, maxFps);
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