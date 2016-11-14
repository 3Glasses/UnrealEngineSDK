#pragma once
// ReSharper disable CppUseAuto

#ifndef WINDOWS_SHARED_MEMORY_IO
#define WINDOWS_SHARED_MEMORY_IO

#include <windows.h>
#include <sddl.h>
#include <memoryapi.h>
#include <tchar.h>


namespace
{
	void CreateACL(SECURITY_ATTRIBUTES* pSA)
	{
		TCHAR* szSD = TEXT("D:")
			TEXT("(A;OICI;GA;;;BA)")
			TEXT("(A;OICI;GAGXGRGWFAFRFWFX;;;S-1-15-2-1)")
			TEXT("(A;OICI;GAGXGRGWFAFRFWFX;;;S-1-1-0)");

		ConvertStringSecurityDescriptorToSecurityDescriptor(
			szSD,
			SDDL_REVISION_1,
			&(pSA->lpSecurityDescriptor),
			nullptr);
	}
}

namespace WinSharedMemoryIO
{
	//Create a shared memory space with a specific name (@lable) and size (@len)
	//Return true if creation successes, else false
	static HANDLE CreateMemorySpace(const TCHAR* lable, DWORD len, bool enableUWPACL = false)
	{
		HANDLE hMapFile = OpenFileMapping(FILE_MAP_READ, FALSE, lable);

		if (NULL == hMapFile)
		{
			if(enableUWPACL){
				SECURITY_ATTRIBUTES sa;
				sa.nLength = sizeof(SECURITY_ATTRIBUTES);
				sa.bInheritHandle = FALSE;

				CreateACL(&sa);
				
				hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, len, lable);
			}
			else
			{
				hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, len, lable);
			}
		}

		return hMapFile;
	}


	//Write a shared memory space named (@lable), from the start byte index to total size of (@size), (@data) is the data to write
	//Return true if writing successes, else false;
	static bool ReadMemory(const TCHAR* lable, long start, size_t size, void* result)
	{
		if (result == nullptr)
		{
			return false;
		}

		HANDLE hMapFile = OpenFileMapping(FILE_MAP_READ, false, lable);

		if (NULL == hMapFile)
		{
			return false;
		}

		char* mData = static_cast<char*>(MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 0));
		if (mData != nullptr && (mData + start) != nullptr)
		{
			memset(result, 0, size);
			memcpy(result, mData + start, size);
			UnmapViewOfFile(mData);
			CloseHandle(hMapFile);
			return true;
		}

		if (mData != nullptr) {
			UnmapViewOfFile(mData);
		}

		if (hMapFile != nullptr) {
			CloseHandle(hMapFile);
		}
		return false;
	}

	static bool WriteMemory(const TCHAR* lable, long start, DWORD size, void* data)
	{
		HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, lable);

		if (NULL == hMapFile)
		{
			return false;
		}

		char* mData = static_cast<char*>(MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0));
		if (mData != nullptr && (mData + start) != nullptr)
		{
			memset(mData + start, 0, size);
			memcpy(mData + start, data, size);
			FlushViewOfFile(mData + start, size);
			UnmapViewOfFile(mData);
			CloseHandle(hMapFile);
			return true;
		}

		if (mData != nullptr) {
			UnmapViewOfFile(mData);
		}

		if (hMapFile != nullptr) {
			CloseHandle(hMapFile);
		}

		return false;
	}
}

#endif
