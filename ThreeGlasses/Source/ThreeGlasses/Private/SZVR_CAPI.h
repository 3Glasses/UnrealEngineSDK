//----------------------------------------------------------------------------------
#ifndef SZVR_CAPI_h
#define SZVR_CAPI_h
//----------------------------------------------------------------------------------
#include <stdint.h>
//----------------------------------------------------------------------------------
typedef char szvrBool;
//----------------------------------------------------------------------------------

#define SZVR_EXPORT __declspec(dllexport)

typedef struct szvrVector2i_
{
	int x, y;
} szvrVector2i;

typedef struct szvrSizei_
{
	int w, h;
} szvrSizei;

typedef struct szvrRecti_
{
	szvrVector2i Pos;
	szvrSizei    Size;
} szvrRecti;

typedef struct szvrQuatf_
{
	float x, y, z, w;
}szvrQuatf;

typedef struct szvrVector2f_
{
	float x, y;
} szvrVector2f;

typedef struct szvrVector3f_
{
	float x, y, z;
} szvrVector3f;

typedef struct szvrMatrix4f_
{
	float M[4][4];
} szvrMatrix4f;

// Position and orientation together.
typedef struct szvrPosef_
{
	szvrQuatf     Orientation;
	szvrVector3f  Position;
} szvrPosef;

// Full pose (rigid body) configuration with first and second derivatives.
typedef struct szvrPoseStatef_
{
	szvrPosef     Pose;
	szvrVector3f  AngularVelocity;
	szvrVector3f  LinearVelocity;
	szvrVector3f  AngularAcceleration;
	szvrVector3f  LinearAcceleration;
	double        TimeInSeconds;         // Absolute time of this state sample.
} szvrPoseStatef;

// Field Of View (FOV) in tangent of the angle units.
// As an example, for a standard 90 degree vertical FOV, we would 
// have: { UpTan = tan(90 degrees / 2), DownTan = tan(90 degrees / 2) }.
typedef struct szvrFovPort_
{
	float UpTan;
	float DownTan;
	float LeftTan;
	float RightTan;
} szvrFovPort;

// State of the sensor at a given absolute time.
typedef struct szvrSensorState_
{
	// Predicted pose configuration at requested absolute time.
	// One can determine the time difference between predicted and actual
	// readings by comparing ovrPoseState.TimeInSeconds.
	szvrPoseStatef  Predicted;
	// Actual recorded pose configuration based on the sensor sample at a 
	// moment closest to the requested time.
	szvrPoseStatef  Recorded;

	// Sensor temperature reading, in degrees Celsius, as sample time.
	float           Temperature;
	// Sensor status described by ovrStatusBits.
	unsigned int    StatusFlags;
} szvrSensorState;

typedef struct DistMeshVert_
{
	float ScreenPosNDC_x;
	float ScreenPosNDC_y;
	float TimewarpLerp;
	float Shade;

	float UVIR_u;   // R-channel uv for image
	float UVIR_v;
	float UVIG_u;   // G-channel uv for image
	float UVIG_v;
	float UVIB_u;   // B-channel uv for image
	float UVIB_v;
}DistMeshVert;

typedef struct DistScaleOffsetUV_
{
	float Scale_x;
	float Scale_y;
	float Offset_x;
	float Offset_y;
}DistScaleOffsetUV;

// Specifies which eye is being used for rendering.
// This type explicitly does not include a third "NoStereo" option, as such is
// not required for an HMD-centered API.
typedef enum
{
	szvrEye_Left = 0,
	szvrEye_Right = 1,
	szvrEye_Count = 2
} szvrEyeType;

typedef struct szvrHeadDisplayDevice_
{
	char szDevice[32];
	size_t DisplayId;
	szvrVector2i WindowsPos;
	szvrSizei Resolution;
	float CameraFrustumHFovInRadians;
	float CameraFrustumVFovInRadians;
	float CameraFrustumNearZInMeters;
	float CameraFrustumFarZInMeters;
}szvrHeadDisplayDevice;

struct MessageList
{
	char isHMDSensorAttached;
	char isHMDAttached;
	char isLatencyTesterAttached;

	MessageList(char HMDSensor, char HMD, char LatencyTester)
	{
		isHMDSensorAttached = HMDSensor;
		isHMDAttached = HMD;
		isLatencyTesterAttached = LatencyTester;
	}
};

// Handle to HMD; returned by szvrHmd_Create.
typedef struct szvrHmdStruct* szvrHmd;

#define PAY_ORDER 0x16 // Request about pay an order

#ifdef __cplusplus 
extern "C" {
#endif

	SZVR_EXPORT int SZVR_GetVer();

	SZVR_EXPORT void SZVR_CopyDistortionMesh(DistMeshVert leftEye[], int leftEyeIndicies[],
		DistScaleOffsetUV* scaleOffset, szvrBool rightEye);

	SZVR_EXPORT void SZVR_GenerateDistortionMesh(int* numVerts, int* numIndicies, szvrBool rightEye);

	SZVR_EXPORT szvrBool SZVR_GetCameraPositionOrientation(float*  px, float*  py, float*  pz,
		float*  ox, float*  oy, float*  oz, float*  ow);

	SZVR_EXPORT void SZVR_GetDistortionMeshInfo(int* resH, int* resV, float* fovH, float* fovV);

	SZVR_EXPORT szvrBool SZVR_GetInterpupillaryDistance(float* interpupillaryDistance);

	SZVR_EXPORT szvrBool SZVR_SetInterpupillaryDistance(const float interpupillaryDistance);

	SZVR_EXPORT szvrBool SZVR_Initialize();

	SZVR_EXPORT szvrBool SZVR_Destroy();

	SZVR_EXPORT szvrBool SZVR_Is3GlassesPresent();

	SZVR_EXPORT void SZVR_ResetSensorOrientation();

	SZVR_EXPORT void SZVR_SetDistortionMeshInfo(int resH, int resV, float fovH, float fovV);

	SZVR_EXPORT szvrBool SZVR_Update(MessageList* messageList);

	SZVR_EXPORT szvrBool SZVR_Get3GlassesResolution(char* szWidth, char* szHeight);

	SZVR_EXPORT szvrBool SZVR_SaveRunAs3Glasses(const char* szCmdLine);

	SZVR_EXPORT szvrBool SZVR_GetHeadDisplayDevice(szvrHeadDisplayDevice* info);

	/**
	 * Request a payment
	 */
	SZVR_EXPORT szvrBool SZVR_MakePayment(const char* szAppkey);

	/**
	 * Read the Quaternion from share memory directly
	 */
	SZVR_EXPORT szvrBool SZVR_GetHmdOrientationWithQuaternion(float *ox, float *oy, float *oz, float *ow);

	/**
	 * Read the Touch Pad coordinates from share memory directly
	 */
	SZVR_EXPORT szvrBool SZVR_GetHmdTouchPad(float *tx, float *ty);

	/**
	 * Get HMD position and resolution
	 */
	SZVR_EXPORT szvrBool SZVR_GetHmdMonitor(int *x, int *y, int *w, int *h);

	/**
	* Get App exit request flag
	*/
	SZVR_EXPORT szvrBool SZVR_GetAppExitFlag(bool *bFlag);

	/**
	 * Read the HMD vector of position from share memory directly
	 */
	SZVR_EXPORT szvrBool SZVR_GetHmdPositionWithVector(float *px, float *py, float *pz);

#ifdef __cplusplus 
} // extern "C"
#endif


#endif	// SZVR_CAPI_h
