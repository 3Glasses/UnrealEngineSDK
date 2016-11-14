#pragma once
// ReSharper disable CppUseAuto

#ifndef SZVR_MEMORY_API
#define SZVR_MEMORY_API

#include "WinSharedMemoryIO.h"
#include <cstdint>

namespace SZVR_MEMORY_API
{
	const auto HMD_ROTATE = L"{8BD6E391-412A-44F2-A1F8-399ED51AA479}";
	const auto HMD_POS = L"{0B82D733-34B9-47A8-87BA-4A185C7B86EF}";
	const auto HMD_TOUCHPAD = L"{CD60D82A-534D-45B5-8B43-5FCF996A871F}";
	const auto HMD_MENU_BUTTOM = L"{E65B779A-EDDD-415A-A8B3-75BC24FA0BE2}";
	const auto HMD_EXIT_BUTTOM = L"{6CB8611A-2E7B-4366-9B98-8FB3424619DF}";
	const auto WAND_ROTATE = L"{095AFF05-0D2C-4BE5-92D9-66531334FA48}";
	const auto WAND_POST = L"{ED095216-968C-4B41-8B2A-8C894E36F277}";
	const auto WAND_BUTTOMS = L"{FAF40360-3C47-432C-B08C-E1D3E9570C7F}";
	const auto WAND_TRIGGER_PROCESS = L"{EDDD8578-6027-457E-AC76-B9AF96917CAE}";
	const auto WAND_STICK = L"{88793986-6E00-4B01-9832-F2B34BEE821B}";
	
	bool GetHMDRotate(float result[])
	{
		return WinSharedMemoryIO::ReadMemory(HMD_ROTATE, 0, sizeof(float) * 4, result);
	}

	bool GetHMDPos(float result[])
	{
		return WinSharedMemoryIO::ReadMemory(HMD_POS, 0, sizeof(float) * 3, result);
	}

	bool GetHMDTouchpad(float result[])
	{
		return WinSharedMemoryIO::ReadMemory(HMD_TOUCHPAD, 0, sizeof(float) * 2, result);
	}

	bool GetHMDMenuButton(bool &result)
	{
		return WinSharedMemoryIO::ReadMemory(HMD_MENU_BUTTOM, 0, sizeof(bool) * 1, &result);
	}

	bool GetHMDExitButton(bool &result)
	{
		return WinSharedMemoryIO::ReadMemory(HMD_EXIT_BUTTOM, 0, sizeof(bool) * 1, &result);
	}

	bool GetWandRotate(float result[])
	{
		return WinSharedMemoryIO::ReadMemory(WAND_ROTATE, 0, sizeof(float) * 8, result);
	}

	bool GetWandPos(float result[])
	{
		return WinSharedMemoryIO::ReadMemory(WAND_POST, 0, sizeof(float) * 6, result);
	}

	bool GetWandButton(bool result[])
	{
		return WinSharedMemoryIO::ReadMemory(WAND_BUTTOMS, 0, sizeof(bool) * 12, result);
	}

	bool GetWandTriggerProcess(uint8_t result[])
	{
		return WinSharedMemoryIO::ReadMemory(WAND_TRIGGER_PROCESS, 0, sizeof(uint8_t) * 2, result);
	}

	bool GetWandStick(uint8_t result[])
	{
		return WinSharedMemoryIO::ReadMemory(WAND_STICK, 0, sizeof(uint8_t) * 4, result);
	}
}

#endif
