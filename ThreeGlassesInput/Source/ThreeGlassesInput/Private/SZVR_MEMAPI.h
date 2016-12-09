#pragma once
// ReSharper disable CppUseAuto

#ifndef SZVR_MEMORY_API
#define SZVR_MEMORY_API

#include <Windows.h>

extern	bool SZVR_GetHMDRotate(float result[]);

extern	bool SZVR_GetHMDPos(float result[]);

extern	bool SZVR_GetHMDTouchpad(float result[]);

extern	bool SZVR_GetHMDMenuButton(bool &result);

extern	bool SZVR_GetHMDExitButton(bool &result);

extern	bool SZVR_GetWandRotate(float result[]);

extern	bool SZVR_GetWandPos(float result[]);

extern	bool SZVR_GetWandButton(bool result[]);

extern	bool SZVR_GetWandTriggerProcess(unsigned char result[]);

extern	bool SZVR_GetWandStick(unsigned char result[]);

extern	bool SZVR_FindHMDRect(RECT &rect);

extern	void SZVR_GetDistortion(unsigned int width, float uv[], float red_uv[], float green_uv[], float blue_uv[]);
#endif
