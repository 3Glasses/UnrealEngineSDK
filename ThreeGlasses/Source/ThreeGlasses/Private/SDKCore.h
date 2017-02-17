#pragma once

#ifndef __SDK_CORE__
#define __SDK_CORE__

#include <Windows.h>
#include <cstdint>
#include <string>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(WINAPI_FAMILY)
#define SDK_CORE_EXPORT __declspec(dllexport)
#endif


SDK_CORE_EXPORT bool SDKCoreInit(HMODULE);
SDK_CORE_EXPORT void SDKCoreDestroy();

SDK_CORE_EXPORT HANDLE SDKCoreStereoRenderGetRenderTarget();
SDK_CORE_EXPORT void SDKCoreStereoRenderBegin();
SDK_CORE_EXPORT void SDKCoreStereoRenderEnd();

SDK_CORE_EXPORT void SDKCoreShowDebugMessageBox(std::wstring);

SDK_CORE_EXPORT void SDKCoreEnableATW();
SDK_CORE_EXPORT void SDKCoreDisableATW();


#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(WINAPI_FAMILY)
#undef SDK_CORE_EXPORT
#endif

#endif
