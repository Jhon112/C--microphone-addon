#include <stdio.h>
#include <node.h>

#include "microphone_darwin.h"
#include "CFAMicrophone/CFAMicrophone/CFAMicrophone.h"

using v8::Isolate;
using v8::Local;
using v8::Number;
using v8::Object;
using v8::String;
using v8::Exception;

#define GOTO_ERROR_ON_ERROR(STATUS) \
	if ((STATUS) != kAudioHardwareNoError) { goto Error; }

#define SAFE_FREE(RES) \
	if ((RES) != NULL) { free(RES); (RES) = NULL; }


void list (const FunctionCallbackInfo<Value>& args) {
	OSStatus status = kAudioHardwareNoError;
	UInt32 numDevices = 0;
	AudioObjectID *inputDevices = NULL;

	Isolate* isolate = args.GetIsolate();
	Local<Object> deviceReturnTable = Object::New(isolate);
	
	// Fetch Devices
	status = deviceListCount(&numDevices);
	GOTO_ERROR_ON_ERROR(status);
	
	inputDevices = (AudioObjectID *)malloc(numDevices * sizeof(AudioObjectID));
	status = inputDeviceList(&numDevices, inputDevices);
	GOTO_ERROR_ON_ERROR(status);
	
	for (UInt32 i = 0; i < numDevices; i++) {
		NSString *uid = nil;
		status = deviceUID(inputDevices[i], &uid);

		if (status != kAudioHardwareNoError) {
			continue;
		}

		NSString *name = nil;
		status = deviceName(inputDevices[i], &name);
		
		if (status != kAudioHardwareNoError) {
			continue;
		}
		
		Local<String> key = String::NewFromUtf8(isolate, [uid UTF8String]);
		Local<String> value = String::NewFromUtf8(isolate, [name UTF8String]);
		deviceReturnTable->Set(key, value);
	}
	
	SAFE_FREE(inputDevices);

	args.GetReturnValue().Set(deviceReturnTable);
	return;

Error:
	SAFE_FREE(inputDevices);

	isolate->ThrowException(Exception::Error(
		String::NewFromUtf8(isolate, "Could not enumerate capture devices.")));
	return;
}

void getVolume(const FunctionCallbackInfo<Value>& args) {
	OSStatus status = kAudioHardwareNoError;
	AudioObjectID device;
	Float32 volume = -1;

	Isolate* isolate = args.GetIsolate();

	if (args.Length() < 1 || !args[0]->IsString()) {
		isolate->ThrowException(Exception::TypeError(
			String::NewFromUtf8(isolate, "Device identifier (String) argument must be defined.")));
		return;
	}

	v8::String::Utf8Value str(args[0]);
	NSString *uid = [NSString stringWithUTF8String:*str];
	
	status = deviceWithUID(uid, &device);
	GOTO_ERROR_ON_ERROR(status);
	
	status = deviceInputVolume(device, &volume);
	GOTO_ERROR_ON_ERROR(status);
	
	args.GetReturnValue().Set(v8::Number::New(isolate, volume));
	return;
	
Error:

	isolate->ThrowException(Exception::Error(
		String::NewFromUtf8(isolate, "Could not read volume from capture device.")));
	return;
}

void setVolume(const FunctionCallbackInfo<Value>& args) {
	OSStatus status = kAudioHardwareNoError;
	AudioObjectID device;

	Isolate* isolate = args.GetIsolate();

	if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsNumber()) {
		isolate->ThrowException(Exception::TypeError(
			String::NewFromUtf8(
				isolate,
				"Device identifier (String) and Volume (Number) arguments must be defined.")));
		return;
	}

	v8::String::Utf8Value str(args[0]);
	NSString *uid = [NSString stringWithUTF8String:*str];

	Float32 volume = (float)args[1]->NumberValue();
	volume = fmaxf(volume, 0.0f);
	volume = fminf(volume, 1.0f);
	
	status = deviceWithUID(uid, &device);
	GOTO_ERROR_ON_ERROR(status);
	
	status = setDeviceInputVolume(device, volume);
	GOTO_ERROR_ON_ERROR(status);
	
	status = deviceInputVolume(device, &volume);
	GOTO_ERROR_ON_ERROR(status);
	
	args.GetReturnValue().Set(v8::Number::New(isolate, volume));
	return;
	
Error:

	isolate->ThrowException(Exception::Error(
		String::NewFromUtf8(isolate, "Could not write volume to capture device.")));
	return;
}