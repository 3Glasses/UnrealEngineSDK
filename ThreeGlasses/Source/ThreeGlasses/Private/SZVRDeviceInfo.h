#pragma once

#ifndef SZVR_DEVICE_INFO
#define SZVR_DEVICE_INFO

#include <Windows.h>
#include <string>
#include <memory>

namespace SZVRDeviceInfo
{
	namespace
	{
		float distortion(float k1, float k2, float r)
		{
			auto delta = (1 + k1*r*r + k2*r*r*r*r);
			return delta;
		}
	}

	const auto HMD_DEV_ID = L"MONITOR\\DSJ0003";

	static BOOL CALLBACK MonitorEnumProc(
		_In_ HMONITOR hMonitor,
		_In_ HDC      hdcMonitor,
		_In_ LPRECT   lprcMonitor,
		_In_ LPARAM   dwData
	)
	{
		MONITORINFOEX mi = {};
		mi.cbSize = sizeof(MONITORINFOEX);
		::GetMonitorInfo(hMonitor, &mi);
		auto pmi = reinterpret_cast<MONITORINFOEX*>(dwData);

		for (auto i = 0; i < 3; ++i)
		{
			if (reinterpret_cast<int*>(pmi)[0] == 0)
			{
				break;
			}
			++pmi;
		}

		memcpy(pmi, &mi, sizeof(MONITORINFOEX));
		return TRUE;
	}

	static bool GetMonitorDeviceName(std::wstring &deviceName, std::wstring devID)
	{
		DISPLAY_DEVICE dd;
		dd.cb = sizeof(dd);

		DWORD deviceNum = 0;
		while (EnumDisplayDevices(nullptr, deviceNum, &dd, 0))
		{
			DISPLAY_DEVICE ddMon;
			ZeroMemory(&ddMon, sizeof(ddMon));
			ddMon.cb = sizeof(ddMon);
			DWORD devMon = 0;

			while (EnumDisplayDevices(dd.DeviceName, devMon, &ddMon, 0))
			{
				devMon++;

				std::wstring dd_devid(ddMon.DeviceID);
				if (dd_devid.find(devID) != std::wstring::npos)
				{
					std::wstring name(dd.DeviceName);
					deviceName.append(name);
					return true;
				}
			}
			deviceNum++;
		}

		return false;
	}

	static bool FindHMDRect(RECT &rect)
	{
		auto device_name = std::make_unique<std::wstring>();
		if (GetMonitorDeviceName(*device_name, HMD_DEV_ID))
		{
			MONITORINFOEX monitor_data[16];
			ZeroMemory(&monitor_data, sizeof(monitor_data));
			auto ret = ::EnumDisplayMonitors(nullptr, nullptr, &MonitorEnumProc, reinterpret_cast<LPARAM>(monitor_data));

			if (ret != TRUE)
			{
				return false;
			}

			for (size_t i = 0; i < 16; i++)
			{
				std::wstring szDevice(monitor_data[i].szDevice);
				if (szDevice.find(*device_name) != std::wstring::npos)
				{
					rect = monitor_data[i].rcMonitor;
					return true;
				}
			}
		}

		return false;
	}

	static void D2Distortion(
		float uv[],
		float red_uv[],
		float green_uv[],
		float blue_uv[])
	{
		float uv2[2] = {0};
		uv2[0] = (uv[0] - 0.5f) * 1.0f;
		uv2[1] = (uv[1] - 0.5f) * 1.0f;

		auto RSq = uv2[0] * uv2[0] + uv2[1] * uv2[1];
		auto NumSegments = 11;
		auto MaxR = 0.50f * sqrt(2.0f);

		auto scaledRsq = static_cast<float>(NumSegments - 1) * sqrt(RSq) / MaxR;
		auto scaledValFloor = int(scaledRsq);

		auto t = scaledRsq - scaledValFloor;
		auto k = static_cast<int>(scaledValFloor);
		float K[11] = {0};

		float Kd[11] = {0.723f, 0.7257f, 0.7345f, 0.7506f, 0.77657f, 0.8173f, 0.8834f, 1.0018f, 1.225f, 1.6f, 2.3f};
		// GOOD WITH D2 FOV=110;  Jiang Yanbing 20160625;
		for (auto i = 0; i < 11; i++)
		{
			K[i] = Kd[i];
		}

		float p0, p1;
		float m0, m1;
		//float K[11] = { 1.0f,1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
		switch (k)
		{
		case 0:
			// Curve starts at 1.0 with gradient K[1]-K[0]
			p0 = K[0];
			m0 = (K[1] - K[0]); // general case would have been (K[1]-K[-1])/2
			p1 = K[1];
			m1 = 0.5f * (K[2] - K[1]);
			break;
		default:
			// General case
			p0 = K[k];
			m0 = 0.5f * (K[k + 1] - K[k - 1]);
			p1 = K[k + 1];
			m1 = 0.5f * (K[k + 2] - K[k]);
			break;
		case 9:
			// Last tangent is just the slope of the last two points.
			p0 = K[NumSegments - 2];
			m0 = 0.5f * (K[NumSegments - 1] - K[NumSegments - 2]);
			p1 = K[NumSegments - 1];
			m1 = K[NumSegments - 1] - K[NumSegments - 2];
			break;
		case 10:
			// Beyond the last segment it's just a straight line
			p0 = K[NumSegments - 1];
			m0 = K[NumSegments - 1] - K[NumSegments - 2];
			p1 = p0 + m0;
			m1 = m0;
			break;
		}

		auto omt = 1.0f - t;
		auto res = (p0 * (1.0f + 2.0f * t) + m0 * t) * omt * omt
			+ (p1 * (1.0f + 2.0f * omt) - m1 * omt) * t * t;

		auto delta = 1.0f * res;
		auto ratio = 1.0f;

		auto Rsq = uv[0] * uv[0] + uv[1] * uv[1];
		float ScaleB;
		float ScaleR;

		ScaleB = 1.0f + 0.00083032f + 0.015478f * Rsq; //blue Chromatic Correction; for D2
		ScaleR = 1.0f - 0.00035032f - 0.007194f * Rsq; //Red  Chromatic Correction; for D2

		float scaleR[2] = {ScaleR, ScaleR};
		float scaleG[2] = {1.0f, 1.0f};
		float scaleB[2] = {ScaleB, ScaleB};


		red_uv[0] = (uv[0] - 0.5f) * delta * scaleR[0] + 0.5f;
		red_uv[1] = (uv[1] - 0.5f) * delta * ratio * scaleR[1] + 0.5f;

		green_uv[0] = (uv[0] - 0.5f) * delta * scaleG[0] + 0.5f;
		green_uv[1] = (uv[1] - 0.5f) * delta * ratio * scaleG[1] + 0.5f;

		blue_uv[0] = (uv[0] - 0.5f) * delta * scaleB[0] + 0.5f;
		blue_uv[1] = (uv[1] - 0.5f) * delta * ratio * scaleB[1] + 0.5f;
	}

	static void S1Distortion(
		float uv[],
		float red_uv[],
		float green_uv[],
		float blue_uv[])
	{
		auto uv_x = (uv[0] * 2.0f - 1.0f) * 0.75f;
		auto uv_y = (uv[1] * 2.0f - 1.0f) * 0.75f;
		auto RSq = uv_x*uv_x + uv_y*uv_y;
		auto r = ::sqrt(RSq);

		const auto _k1 = 0.203f;
		const auto _k2 = 0.887f;
		const auto _red_k1 = 0.0252f;
		const auto _red_k2 = -0.0322f;
		const auto _blue_k1 = -0.0221f;
		const auto _blue_k2 = 0.0288f;

		auto delta = distortion(_k1, _k2, r);

		auto delta_red = delta *(1.0f + _red_k1) + _red_k2;
		auto delta_green = delta;
		auto delta_blue = delta *(1.0f + _blue_k1) + _blue_k2;

		red_uv[0] = (uv_x * delta_red + 1.0f) / 2.0f;
		red_uv[1] = (uv_y * delta_red + 1.0f) / 2.0f;

		green_uv[0] = (uv_x * delta_green + 1.0f) / 2.0f;
		green_uv[1] = (uv_y * delta_green + 1.0f) / 2.0f;

		blue_uv[0] = (uv_x * delta_blue + 1.0f) / 2.0f;
		blue_uv[1] = (uv_y * delta_blue + 1.0f) / 2.0f;
	}

	static void GetDistortion(
		uint32_t width,
		float uv[],
		float red_uv[],
		float green_uv[],
		float blue_uv[])
	{
		if(width == 2560) // D2
		{
			D2Distortion(uv, red_uv, green_uv, blue_uv);
		}
		else
		{
			S1Distortion(uv, red_uv, green_uv, blue_uv);
		}
	}
}

#endif
