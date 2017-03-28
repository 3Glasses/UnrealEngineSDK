// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "ThreeGlassesInput.h"
#include "Engine.h"
#include "Runtime/InputDevice/Public/IInputDevice.h"
#include "Runtime/InputDevice/Public/IHapticDevice.h"
#include "IMotionController.h"
#include "AllowWindowsPlatformTypes.h"
#include "SZVRSharedMemoryAPI.h"
#include "HideWindowsPlatformTypes.h"

#define LOCTEXT_NAMESPACE "FThreeGlassesInputModule"

const int ButtonNum = 12;
class F3GlassesController : public IInputDevice, public IMotionController, public IHapticDevice
{
public:
	/** handler to send all messages to */
	TSharedRef<FGenericApplicationMessageHandler> MessageHandler;
	bool BottonState[ButtonNum] = { 0 };
	uint8 AxisState[4] = { 0 };
	uint8 TriggerState[2] = { 0 };
	SZVR::MemoryManager ShareMemory;

	F3GlassesController(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler)
		: MessageHandler(InMessageHandler)
	{
		IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);
		ShareMemory.InitIfExist();
	}

	virtual void SetMessageHandler(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler) override
	{
		MessageHandler = InMessageHandler;
	}

	virtual IHapticDevice* GetHapticDevice() override
	{
		return this;
	}

	virtual bool GetControllerOrientationAndPosition(const int32 ControllerIndex, const EControllerHand DeviceHand, FRotator& OutOrientation, FVector& OutPosition) const
	{
		FQuat DeviceOrientation = FQuat::Identity;
		float PosData[6] = { 0 };
		SZVR_GetWandPos(PosData);
		float RotData[8] = { 0 };
		SZVR_GetWandRotate(RotData);

		switch (DeviceHand)
		{
		case EControllerHand::Left:
			DeviceOrientation = FQuat(RotData[0], RotData[1], RotData[2], RotData[3]);
			OutPosition = FVector(-PosData[2], -PosData[0], -PosData[1])*0.1f;
			break;
		case EControllerHand::Right:
			DeviceOrientation = FQuat(RotData[4], RotData[5], RotData[6], RotData[7]);
			OutPosition = FVector(-PosData[5], -PosData[3], -PosData[4])*0.1f;
			break;
		default:
			return false;
		}

		if (DeviceOrientation.W == 0)
		{
			DeviceOrientation = FQuat::Identity;
		}

		DeviceOrientation = FQuat(-DeviceOrientation.Z, -DeviceOrientation.X, DeviceOrientation.Y, DeviceOrientation.W);
		OutOrientation = DeviceOrientation.Rotator();
		return true;
	}

	ETrackingStatus GetControllerTrackingStatus(const int32 ControllerIndex, const EControllerHand DeviceHand) const override
	{
		return ETrackingStatus::Tracked;
	}

	void Tick(float DeltaTime) {};

	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override
	{
		return false;
	}

	virtual void GetHapticFrequencyRange(float& MinFrequency, float& MaxFrequency) const override
	{
		MinFrequency = 0.0f;
		MaxFrequency = 1.0f;
	}

	virtual float GetHapticAmplitudeScale() const override
	{
		return 1.0f;
	}

	void SetChannelValue(int32 UnrealControllerId, FForceFeedbackChannelType ChannelType, float Value) override
	{
	}

	void SetChannelValues(int32 UnrealControllerId, const FForceFeedbackValues& Values) override
	{
	}

	virtual void SetHapticFeedbackValues(int32 UnrealControllerId, int32 Hand, const FHapticFeedbackValues& Values) override
	{
		if (Hand>=0&&Hand<2)
		{
			uint16 data[2] = { 0 };
			ShareMemory.LoadDataMemory(SZVR::eWAND_VIBRATOR, data);
			data[Hand] = Values.Amplitude;
			ShareMemory.SaveDataMemory(SZVR::eWAND_VIBRATOR, data);
		}
	}

	virtual void SendControllerEvents() override
	{
		FName KeyNames[ButtonNum] =
		{
			FGamepadKeyNames::SpecialLeft,
			FGamepadKeyNames::MotionController_Left_Shoulder,
			FGamepadKeyNames::MotionController_Left_Grip1,
			FGamepadKeyNames::MotionController_Left_Grip2,
			FGamepadKeyNames::MotionController_Left_Trigger,
			FGamepadKeyNames::MotionController_Left_Thumbstick,
			FGamepadKeyNames::SpecialRight,
			FGamepadKeyNames::MotionController_Right_Shoulder,
			FGamepadKeyNames::MotionController_Right_Grip1,
			FGamepadKeyNames::MotionController_Right_Grip2,
			FGamepadKeyNames::MotionController_Right_Trigger,
			FGamepadKeyNames::MotionController_Right_Thumbstick,
		};

		bool Button[ButtonNum] = { 0 };
		if (SZVR_GetWandButton(Button) == 0)
		{
			for (int i = 0; i < ButtonNum; i++)
			{
				if (BottonState[i] != Button[i] && KeyNames[i] != FGamepadKeyNames::Invalid)
				{
					if (Button[i])
					{
						MessageHandler->OnControllerButtonPressed(KeyNames[i], 0, false);
					}
					else
					{
						MessageHandler->OnControllerButtonReleased(KeyNames[i], 0, false);
					}

					BottonState[i] = Button[i];
				}
			}
		}

		uint8 Axis[4] = { 0 };
		if (SZVR_GetWandStick(Axis) == 0)
		{
			FName ThumbstickKeyNames[4] = {
				FGamepadKeyNames::MotionController_Left_Thumbstick_X,
				FGamepadKeyNames::MotionController_Left_Thumbstick_Y,
				FGamepadKeyNames::MotionController_Right_Thumbstick_X,
				FGamepadKeyNames::MotionController_Right_Thumbstick_Y
			};

			for (int i = 0; i < 4; i++)
			{
				if (Axis[i] != AxisState[i])
				{
					float val = float(Axis[i]) / 255.0f*2.0f - 1.0f;
					MessageHandler->OnControllerAnalog(ThumbstickKeyNames[i], 0, val);
					AxisState[i] = Axis[i];
				}
			}
		}

		uint8 TriggerAxis[2];
		if (SZVR_GetWandTriggerProcess(TriggerAxis) == 0)
		{
			if (TriggerState[0] != TriggerAxis[0])
			{
				MessageHandler->OnControllerAnalog(FGamepadKeyNames::MotionController_Left_TriggerAxis, 0, float(TriggerAxis[0]) / 255.0f);
				TriggerState[0] = TriggerAxis[0];
			}

			if (TriggerState[1] != TriggerAxis[1])
			{
				MessageHandler->OnControllerAnalog(FGamepadKeyNames::MotionController_Right_TriggerAxis, 0, float(TriggerAxis[1]) / 255.0f);
				TriggerState[1] = TriggerAxis[1];
			}
		}
	}
};

TSharedPtr< class IInputDevice > FThreeGlassesInputModule::CreateInputDevice(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler)
{
	return TSharedPtr< class IInputDevice >(new F3GlassesController(InMessageHandler));
}
#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FThreeGlassesInputModule, ThreeGlassesInput)

