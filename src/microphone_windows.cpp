#include <stdio.h>
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <math.h>
#include <node.h>
#include <FunctionDiscoveryKeys_devpkey.h>

#include "microphone_windows.h"

using v8::Isolate;
using v8::Local;
using v8::Integer;
using v8::Number;
using v8::Object;
using v8::String;
using v8::Null;
using v8::Exception;

#define GOTO_ERROR_ON_ERROR(HRES)  \
	if (FAILED(HRES)) { goto Error; }

#define SAFE_RELEASE(RES) \
	if ((RES) != NULL) \
		{ (RES)->Release(); (RES) = NULL; }

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);

void list (const FunctionCallbackInfo<Value>& args) {
	HRESULT hr = S_OK;
	IMMDeviceEnumerator *devEnumerator = NULL;
	IMMDeviceCollection *devCollection = NULL;
	IMMDevice *device = NULL;
	LPWSTR devId = NULL;
	IPropertyStore *devProps = NULL;
	Isolate* isolate = args.GetIsolate();
	Local<Object> deviceReturnTable = Object::New(isolate);

	CoInitialize(NULL);

	hr = CoCreateInstance(
		CLSID_MMDeviceEnumerator, NULL,
		CLSCTX_ALL, IID_IMMDeviceEnumerator,
		(void **)&devEnumerator);
	GOTO_ERROR_ON_ERROR(hr);

	hr = devEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &devCollection);
	GOTO_ERROR_ON_ERROR(hr);

	UINT devCount = 0;
	hr = devCollection->GetCount(&devCount);
	GOTO_ERROR_ON_ERROR(hr);

	for (UINT i = 0; i < devCount; i++) {
		// Fetch device by index
		hr = devCollection->Item(i, &device);
		GOTO_ERROR_ON_ERROR(hr);

		// Get device Id
		hr = device->GetId(&devId);
		GOTO_ERROR_ON_ERROR(hr);

		// Get device properties
		hr = device->OpenPropertyStore(STGM_READ, &devProps);
		GOTO_ERROR_ON_ERROR(hr);

		PROPVARIANT devPropValue;
		PropVariantInit(&devPropValue);

		hr = devProps->GetValue(PKEY_Device_FriendlyName, &devPropValue);
		GOTO_ERROR_ON_ERROR(hr);

		// Toss into table
		char *devId_c = (char *)calloc(wcslen(devId) + 1, sizeof(char));
		char *devPropValue_c = (char *)calloc(wcslen(devPropValue.pwszVal) + 1, sizeof(char));

		wsprintfA(devId_c, "%S", devId);
		wsprintfA(devPropValue_c, "%S", devPropValue.pwszVal);

		Local<String> key = String::NewFromUtf8(isolate, devId_c);
		Local<String> value = String::NewFromUtf8(isolate, devPropValue_c);
		deviceReturnTable->Set(key, value);

		// Cleanup
		free(devPropValue_c);
		free(devId_c);
		PropVariantClear(&devPropValue);
		SAFE_RELEASE(devProps);
		CoTaskMemFree(devId);
		devId = NULL;
		SAFE_RELEASE(device);
	}

	SAFE_RELEASE(devCollection);
	SAFE_RELEASE(devEnumerator);
	CoUninitialize();

	args.GetReturnValue().Set(deviceReturnTable);
	return;

	Error:
		SAFE_RELEASE(devProps);
		CoTaskMemFree(devId);
		SAFE_RELEASE(device);
		SAFE_RELEASE(devCollection);
		SAFE_RELEASE(devEnumerator);
		CoUninitialize();

	isolate->ThrowException(Exception::Error(
		String::NewFromUtf8(isolate, "Could not enumerate capture devices.")));
	return;
}

void getVolume(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	HRESULT hr = S_OK;
	IMMDeviceEnumerator *devEnumerator = NULL;
	IMMDevice *device = NULL;
	IAudioEndpointVolume *volumeInterface = NULL;
	float volume = 0;

	if (args.Length() < 1 || !args[0]->IsString()) {
		isolate->ThrowException(Exception::TypeError(
			String::NewFromUtf8(isolate, "Device identifier (String) argument must be defined.")));
		return;
	}

	v8::String::Utf8Value str(args[0]);
	char *devId_c = *str;
	wchar_t *devId = (wchar_t *)calloc(strlen(devId_c) + 1, sizeof(wchar_t));
	mbstowcs(devId, devId_c, strlen(devId_c) + 1);

	// Fetch device by id
	CoInitialize(NULL);

	hr = CoCreateInstance(
		CLSID_MMDeviceEnumerator, NULL,
		CLSCTX_ALL, IID_IMMDeviceEnumerator,
		(void **)&devEnumerator);
	GOTO_ERROR_ON_ERROR(hr);

	hr = devEnumerator->GetDevice(devId, &device);
	GOTO_ERROR_ON_ERROR(hr);

	// Read device volume
	hr = device->Activate(
		__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER,
		NULL, (LPVOID *) &volumeInterface);
	GOTO_ERROR_ON_ERROR(hr);

	hr = volumeInterface->GetMasterVolumeLevelScalar(&volume);
	GOTO_ERROR_ON_ERROR(hr);

	// Cleanup
	SAFE_RELEASE(volumeInterface);
	SAFE_RELEASE(device);
	SAFE_RELEASE(devEnumerator);
	CoUninitialize();
	free(devId);

	args.GetReturnValue().Set(v8::Number::New(isolate, volume));
	return;

	Error:
		SAFE_RELEASE(volumeInterface);
		SAFE_RELEASE(device);
		SAFE_RELEASE(devEnumerator);
		CoUninitialize();
		free(devId);

	isolate->ThrowException(Exception::Error(
		String::NewFromUtf8(isolate, "Could not read volume from capture device.")));
	return;
}

void setVolume(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	HRESULT hr = S_OK;
	IMMDeviceEnumerator *devEnumerator = NULL;
	IMMDevice *device = NULL;
	IAudioEndpointVolume *volumeInterface = NULL;

	if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsNumber()) {
		isolate->ThrowException(Exception::TypeError(
			String::NewFromUtf8(
				isolate,
				"Device identifier (String) and Volume (Number) arguments must be defined.")));
		return;
	}

	v8::String::Utf8Value str(args[0]);
	char *devId_c = *str;
	wchar_t *devId = (wchar_t *)calloc(strlen(devId_c) + 1, sizeof(wchar_t));
	mbstowcs(devId, devId_c, strlen(devId_c) + 1);

	float volume = (float)args[1]->NumberValue();
	volume = fmaxf(volume, 0.0f);
	volume = fminf(volume, 1.0f);

	// Fetch device by id
	CoInitialize(NULL);

	hr = CoCreateInstance(
		CLSID_MMDeviceEnumerator, NULL,
		CLSCTX_ALL, IID_IMMDeviceEnumerator,
		(void **)&devEnumerator);
	GOTO_ERROR_ON_ERROR(hr);

	hr = devEnumerator->GetDevice(devId, &device);
	GOTO_ERROR_ON_ERROR(hr);

	// Write device volume
	hr = device->Activate(
		__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER,
		NULL, (LPVOID *) &volumeInterface);
	GOTO_ERROR_ON_ERROR(hr);

	hr = volumeInterface->SetMasterVolumeLevelScalar(volume, NULL);
	GOTO_ERROR_ON_ERROR(hr);

	// Read device volume
	hr = volumeInterface->GetMasterVolumeLevelScalar(&volume);
	GOTO_ERROR_ON_ERROR(hr);

	// Cleanup
	SAFE_RELEASE(volumeInterface);
	SAFE_RELEASE(device);
	SAFE_RELEASE(devEnumerator);
	CoUninitialize();
	free(devId);

	args.GetReturnValue().Set(v8::Number::New(isolate, volume));
	return;

	Error:
		SAFE_RELEASE(volumeInterface);
		SAFE_RELEASE(device);
		SAFE_RELEASE(devEnumerator);
		CoUninitialize();
		free(devId);

	isolate->ThrowException(Exception::Error(
		String::NewFromUtf8(isolate, "Could not write volume to capture device.")));
	return;
}