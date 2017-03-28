/*******************************************************************************
3Glasses CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with ShenZhen Virtual Reality Technology Co., Ltd. and may not be copied or
disclosed except in accordance with the terms of that agreement
Copyright(c) 2016-2017 ShenZhen Virtual Reality Technology Co., Ltd. All Rights Reserved.
********************************************************************************/

#pragma once

#ifndef __SZVR_SHARED_MEMORY_API__
#define __SZVR_SHARED_MEMORY_API__

#include <windows.h>
#include <stdint.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(WINAPI_FAMILY)
#define SZVR_EXPORT __declspec(dllexport)
#endif

#ifdef __cplusplus

namespace SZVR
{
	enum eMemoryGuidFlag
	{
		eFLAG = 0,

		/* HMD Session */
		eHMD_CONNECTION,
		eHMD_PRESENT,
		eHMD_INFO,
		eHMD_QUAT,
		eHMD_POS,
		eHMD_POS_TRACK,
		eHMD_TP,
		eHMD_MENU_BT,
		eHMD_BACK_BT,
		eHMD_FIRMWARE_VERSION,
		eHMD_SERIAL_NUMBER,
		eHMD_PRODUCT_NAME,
		eHMD_IPD_VALUE,
		eHMD_DEVICE_PATH,
		eHMD_FIRMWARE_VERSION_UPDATE_PROGREES,
		eHMD_LIGHT_SENSOR_STATUS,
		eHMD_QUAT_FUSION_RESET,
		eHMD_POS_RESET,

		/* WAND Session */
		eWAND_CONNECTION,
		eWAND_QUAT,
		eWAND_POS,
		eWAND_POS_TRACK,
		eWAND_BTS,
		eWAND_TRIGGER_VALUE,
		eWAND_STICK_XY,
		eWAND_FIRMWARE_VERSION,
		eWAND_SERIAL_NUMBER,
		eWAND_PRODUCT_NAME,
		eWAND_FIRMWARE_VERSION_UPDATE_PROGREES,
		eWAND_VIBRATOR,
		eWAND_POWER_LEVEL,

		/* Camera Session */
		eCAMERA_CONNECTION,
		eCAMERA_FIRMWARE_VERSION,
		eCAMERA_SERIAL_NUMBER,
		eCAMERA_PRODUCT_NAME,
		eCAMERA_FIRMWARE_VERSION_UPDATE_PROGRESS,

		/* SDK Session */
		eSDK_3GLASSES_API_BUILDTIME,
		eSDK_VRSHOW_API_BUILDTIME,
		eSDK_VRSHOW_SERVER_EXIST,

		/* Max of elements */
		eMEM_ENUM_MAX
	};


	struct MemoryListData
	{
		eMemoryGuidFlag EnumIndex;
		DWORD SizeOfMemory;
		TCHAR * StringOfMemoryLabel;
	};

	class MemoryManager
	{
	public:
		MemoryManager();
		virtual ~MemoryManager();

		struct ShareMemoryObject
		{
			DWORD MemorySize;
			HANDLE MapHandle;
			char * MappedData;
			TCHAR MemoryLabel[MAX_PATH + 1];
		};

		bool InitIfExist();
		BOOL ReleaseMemoryManager();

		bool SaveDataMemory(eMemoryGuidFlag eFlag, void * data);
		bool LoadDataMemory(eMemoryGuidFlag eFlag, void * data);
	protected:
		static HANDLE CreateMemorySpace(TCHAR * lable, DWORD len, char ** mappedData);
		static HANDLE OpenMemorySpace(TCHAR * lable, DWORD len, char ** mappedData);
		static bool ReadMemory(char* mData, long start, size_t size, void * result);
		static bool WriteMemory(char* mData, long start, DWORD size, void * data);

	private:
		ShareMemoryObject MemoryObjList[eMEM_ENUM_MAX];
	};
}

#endif


#ifdef __cplusplus
extern "C" {
#endif

/*
[Functionality]
Get HMD Connection Status.

[Parameters]
result
Type: bool*
It returns true hmd is connection otherwise it returns false

[Return value]
Type: int
It returns 0 it success, othe value is false
*/
__declspec(deprecated("API is Old")) SZVR_EXPORT uint32_t SZVR_GetHMDConnectionStatus(bool *result);

/*
[Functionality]
Get HMD quaternion value.

[Parameters]
result
Type: float[4]
    0: orientation quaternion x
    1: orientation quaternion y
    2: orientation quaternion z
    3: orientation quaternion w

[Return value]
Type: int
It returns 0 it success, othe value is false
*/
__declspec(deprecated("API is Old")) SZVR_EXPORT uint32_t SZVR_GetHMDRotate(float result[]);

/*
[Functionality]
Get HMD position value.

[Parameters]
result
Type: float[3]
    0: position  x
    1: position  y
    2: position  z

[Return value]
Type: int
It returns 0 it success, othe value is false
*/
__declspec(deprecated("API is Old")) SZVR_EXPORT uint32_t SZVR_GetHMDPos(float result[]);

/*
[Functionality]
Get HMD touchpad value.

[Parameters]
result
Type: float[2]
    0: position  x
    1: position  y

[Return value]
Type: int
It returns 0 it success, othe value is false
*/
__declspec(deprecated("API is Old")) SZVR_EXPORT uint32_t SZVR_GetHMDTouchpad(uint8_t result[]);

/*
[Functionality]
Get HMD present status.

[Parameters]
result
Type: bool*

[Return value]
Type: int
It returns 0 it success, othe value is false
*/
__declspec(deprecated("API is Old")) SZVR_EXPORT uint32_t SZVR_GetHMDPresent(bool *result);

/*
[Functionality]
Get HMD menu button status.

[Parameters]
result
Type: bool*
    down: true
    up: false

[Return value]
Type: int
It returns 0 it success, othe value is false
*/
__declspec(deprecated("API is Old")) SZVR_EXPORT uint32_t SZVR_GetHMDMenuButton(bool *result);

/*
[Functionality]
Get HMD exit button status. (only D2)

[Parameters]
result
Type: bool*
    down: true
    up: false

[Return value]
Type: int
It returns 0 it success, othe value is false
*/
__declspec(deprecated("API is Old")) SZVR_EXPORT uint32_t SZVR_GetHMDExitButton(bool *result);

/*
[Functionality]
Get HMD device name.

[Parameters]
result
Type: char[64]

[Return value]
Type: int
It returns 0 it success, othe value is false
*/
__declspec(deprecated("API is Old")) SZVR_EXPORT uint32_t SZVR_GetHMDDevName(char *name);

/*
[Functionality]
Get HMD serial number.

[Parameters]
result
Type: char[64]

[Return value]
Type: int
It returns 0 it success, othe value is false
*/
__declspec(deprecated("API is Old")) SZVR_EXPORT uint32_t SZVR_GetHMDDevSerialNumber(char *serialNumber);

/*
[Functionality]
Get HMD ipd value.

[Parameters]
result
Type: uint8_t*

[Return value]
Type: int
It returns 0 it success, othe value is false
*/
__declspec(deprecated("API is Old")) SZVR_EXPORT uint32_t SZVR_GetHMDDevIPD(uint8_t *value);

/*
[Functionality]
Get wand serial number.

[Parameters]
result
Type: char[64]

[Return value]
Type: int
It returns 0 it success, othe value is false
*/
__declspec(deprecated("API is Old")) SZVR_EXPORT uint32_t SZVR_GetWandDevSerialNumber(char *serial);


/*
[Functionality]
Get Wand Connection Status.

[Parameters]
result
Type: bool[2]
It returns true wand is connection otherwise it returns false
0 for first wand
1 for second wand

[Return value]
Type: int
It returns 0 it success, othe value is false
*/
__declspec(deprecated("API is Old")) SZVR_EXPORT uint32_t SZVR_GetWandConnectionStatus(bool status[]);

/*
[Functionality]
Get Wand quaternion value.

[Parameters]
result
Type: float[6]
    0: orientation quaternion x for first wand
    1: orientation quaternion y for first wand
    2: orientation quaternion z for first wand
    3: orientation quaternion w for first wand

    4: orientation quaternion x for second wand
    5: orientation quaternion y for second wand
    6: orientation quaternion z for second wand
    7: orientation quaternion w for second wand

[Return value]
Type: int
It returns 0 it success, othe value is false
*/
__declspec(deprecated("API is Old")) SZVR_EXPORT uint32_t SZVR_GetWandRotate(float result[]);

/*
[Functionality]
Get Wand position value.

[Parameters]
result
Type: float[6]
    0: position x for second wand
    1: position y for second wand
    2: position z for second wand

    3: position x for first wand
    4: position y for first wand
    5: position z for first wand


[Return value]
Type: int
It returns 0 it success, othe value is false
*/
__declspec(deprecated("API is Old")) SZVR_EXPORT uint32_t SZVR_GetWandPos(float result[]);

/*
[Functionality]
Get Wand button status.

[Parameters]
result
Type: bool[12]

first wand
    0 Menu button;
    1 Back button;
    2 Left handle button;
    3 Right handle button;
    4 Trigger press down;
    5 Trigger press all the way down;

second wand
    6 Menu button;
    7 Back button;
    8 Left handle button;
    9 Right handle button;
    10 Trigger press down;
    11 Trigger press all the way down;


[Return value]
Type: int
It returns 0 it success, othe value is false
*/
__declspec(deprecated("API is Old")) SZVR_EXPORT uint32_t SZVR_GetWandButton(bool result[]);

/*
[Functionality]
Get Wand trigger process value.

[Parameters]
result
Type: uint8_t[2]
0 for first wand
1 for second wand

[Return value]
Type: int
It returns 0 it success, othe value is false
*/
__declspec(deprecated("API is Old")) SZVR_EXPORT uint32_t SZVR_GetWandTriggerProcess(uint8_t result[]);

/*
[Functionality]
Get Wand stick value.

[Parameters]
result
Type: uint8_t[4]
    0: position  x first wand
    1: position  y first wand
    2: position  x second wand
    3: position  y second wand

[Return value]
Type: int
It returns 0 it success, othe value is false
*/
__declspec(deprecated("API is Old")) SZVR_EXPORT uint32_t SZVR_GetWandStick(uint8_t result[]);

/*
[Functionality]
Get camera connection status.

[Parameters]
result
Type: bool*
It returns true camera is connection otherwise it returns false

[Return value]
Type: int
It returns 0 it success, othe value is false
*/
__declspec(deprecated("API is Old")) SZVR_EXPORT uint32_t SZVR_GetCameraConnectionStatus(bool *result);

#ifdef __cplusplus  
}
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(WINAPI_FAMILY)
#undef SZVR_EXPORT
#endif


#endif
