/*******************************************************************************
3Glasses CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with ShenZhen Virtual Reality Technology Co., Ltd. and may not be copied or
disclosed except in accordance with the terms of that agreement
Copyright(c) 2016-2017 ShenZhen Virtual Reality Technology Co., Ltd. All Rights Reserved.
********************************************************************************/

#pragma once

#ifndef __SDK_COMPOSITOR__
#define __SDK_COMPOSITOR__

#include <Windows.h>
#include <cstdint>
#include <string>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(WINAPI_FAMILY)
#define SDK_COMPOSITOR_EXPORT __declspec(dllexport)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

	SDK_COMPOSITOR_EXPORT bool SDKCompositorInit(HMODULE);
	SDK_COMPOSITOR_EXPORT void SDKCompositorDestroy();

	SDK_COMPOSITOR_EXPORT void SDKCompositorHMDRenderInfo(
		int32_t* left,
		int32_t* top,
		int32_t* right,
		int32_t* bottom
	);

	SDK_COMPOSITOR_EXPORT void SDKCompositorHMDFOVTanAngles(
		float* left,
		float* top,
		float* right,
		float* bottom
	);

	SDK_COMPOSITOR_EXPORT HANDLE SDKCompositorStereoRenderGetRenderTarget();
	SDK_COMPOSITOR_EXPORT void SDKCompositorStereoRenderBegin();
	SDK_COMPOSITOR_EXPORT void SDKCompositorStereoRenderEnd();

	SDK_COMPOSITOR_EXPORT void SDKCompositorShowDebugMessageBox(std::wstring);

	SDK_COMPOSITOR_EXPORT void SDKCompositorEnableATW();
	SDK_COMPOSITOR_EXPORT void SDKCompositorDisableATW();

#ifdef __cplusplus
}
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(WINAPI_FAMILY)
#undef SDK_COMPOSITOR_EXPORT
#endif

#endif
