//
//  CFAMicrophone.cpp
//  CFAMicrophone
//
//  Created by Jacob McDorman on 8/8/17.
//  Copyright © 2017 Cellaflora. All rights reserved.
//

#include <stdio.h>
#include "CFAAudioObject.h"

OSStatus deviceListCount (UInt32 *count) {
    AudioObjectPropertyAddress listAddress = {
        .mSelector = kAudioHardwarePropertyDevices,
        .mScope    = kAudioObjectPropertyScopeGlobal,
        .mElement  = kAudioObjectPropertyElementMaster
    };
    UInt32 dataSize = 0;
    OSStatus status = kAudioHardwareNoError;
    
    status = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &listAddress, 0, NULL, &dataSize);
    if (status != kAudioHardwareNoError) {
        *count = 0;
        return status;
    }
    
    *count = dataSize / sizeof(AudioObjectID);
    return kAudioHardwareNoError;
}

OSStatus deviceList (UInt32 *count, AudioObjectID *devices) {
    AudioObjectPropertyAddress listAddress = {
        .mSelector = kAudioHardwarePropertyDevices,
        .mScope    = kAudioObjectPropertyScopeGlobal,
        .mElement  = kAudioObjectPropertyElementMaster
    };
    UInt32 dataSize = (*count) * sizeof(AudioObjectID);
    OSStatus status = kAudioHardwareNoError;
    
    status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &listAddress, 0, NULL, &dataSize, (void *)devices);
    if (status != kAudioHardwareNoError) {
        *count = 0;
        return status;
    }
    
    *count = dataSize / sizeof(AudioObjectID);
    return kAudioHardwareNoError;
}

OSStatus deviceInputChannels (AudioObjectID device, UInt32 *count, UInt32 *channels) {
    AudioObjectPropertyAddress channelsAddress = {
        .mSelector = kAudioDevicePropertyPreferredChannelsForStereo,
        .mScope    = kAudioDevicePropertyScopeInput,
        .mElement  = kAudioObjectPropertyElementWildcard
    };
    UInt32 dataSize = (*count) * sizeof(UInt32);
    OSStatus status = kAudioHardwareNoError;
    
    status = AudioObjectGetPropertyData(device, &channelsAddress, 0, NULL, &dataSize, (void *)channels);
    if (status != kAudioHardwareNoError) {
        *count = 0;
        return status;
    }
    
    *count = dataSize / sizeof(UInt32);
    return kAudioHardwareNoError;
}

OSStatus inputDeviceList (UInt32 *count, AudioObjectID *inputDevices) {
    OSStatus status = kAudioHardwareNoError;
    UInt32 numDevices = 0;
    UInt32 numInputDevices = 0;
    AudioObjectID *allDevices = NULL;
    
    status = deviceListCount(&numDevices);
    if (status != kAudioHardwareNoError) {
        *count = 0;
        return status;
    }
    
    allDevices = (AudioObjectID *)malloc(numDevices * sizeof(AudioObjectID));
    status = deviceList(&numDevices, allDevices);
    if (status != kAudioHardwareNoError) {
        free(allDevices);
        *count = 0;
        return status;
    }
    
    for (UInt32 i = 0; i < numDevices; i++) {
        UInt32 channels[2];
        UInt32 numChannels = 2;
        
        status = deviceInputChannels(allDevices[i], &numChannels, channels);
        if (status != kAudioHardwareNoError) {
            continue;
        }
        
        if (numInputDevices > (*count)){
            break;
        }
        
        memcpy(inputDevices + numInputDevices++, allDevices + i, sizeof(AudioObjectID));
    }
    
    free(allDevices);
    *count = numInputDevices;
    return kAudioHardwareNoError;
}

OSStatus deviceName (AudioObjectID device, NSString * __autoreleasing *name) {
    AudioObjectPropertyAddress nameAddress = {
        .mSelector = kAudioDevicePropertyDeviceNameCFString,
        .mScope    = kAudioObjectPropertyScopeGlobal,
        .mElement  = kAudioObjectPropertyElementMaster
    };
    UInt32 dataSize = sizeof(CFStringRef);
    OSStatus status = kAudioHardwareNoError;
    
    NSString *_name = nil;
    status = AudioObjectGetPropertyData(device, &nameAddress, 0, NULL, &dataSize, (void *)&_name);
    if (status != kAudioHardwareNoError) {
        return status;
    }
    
    *name = _name;
    return kAudioHardwareNoError;
}

OSStatus deviceUID (AudioObjectID device, NSString * __autoreleasing *uid) {
    AudioObjectPropertyAddress nameAddress = {
        .mSelector = kAudioDevicePropertyDeviceUID,
        .mScope    = kAudioObjectPropertyScopeGlobal,
        .mElement  = kAudioObjectPropertyElementMaster
    };
    UInt32 dataSize = sizeof(CFStringRef);
    OSStatus status = kAudioHardwareNoError;
    
    NSString *_uid = nil;
    status = AudioObjectGetPropertyData(device, &nameAddress, 0, NULL, &dataSize, (void *)&_uid);
    if (status != kAudioHardwareNoError) {
        return status;
    }
    
    *uid = _uid;
    return kAudioHardwareNoError;
}

OSStatus deviceWithUID (NSString *uid, AudioObjectID *device) {
    OSStatus status = kAudioHardwareNoError;
    OSStatus foundStatus = kAudioHardwareNotRunningError;
    UInt32 numDevices = 0;
    AudioObjectID *devices = NULL;
    
    status = deviceListCount(&numDevices);
    if (status != kAudioHardwareNoError) {
        return status;
    }
    
    devices = (AudioObjectID *)malloc(numDevices * sizeof(AudioObjectID));
    status = deviceList(&numDevices, devices);
    if (status != kAudioHardwareNoError) {
        free(devices);
        return status;
    }
    
    for (UInt32 i = 0; i < numDevices; i++) {
        NSString *searchUID = nil;
        status = deviceUID(devices[i], &searchUID);
        
        if (status != kAudioHardwareNoError) {
            continue;
        }
        
        if ([uid isEqualToString:searchUID]) {
            foundStatus = kAudioHardwareNoError;
            *device = devices[i];
            break;
        }
    }
    
    free(devices);
    return foundStatus;
}

OSStatus deviceInputVolume (AudioObjectID device, Float32 *volume) {
    AudioObjectPropertyAddress volumeAddress = {
        .mSelector = kAudioDevicePropertyVolumeScalar,
        .mScope    = kAudioDevicePropertyScopeInput,
        .mElement  = kAudioObjectPropertyElementMaster
    };
    OSStatus status = kAudioHardwareNoError;
    UInt32 channels[2];
    UInt32 numChannels = 2;
    Float32 totalVolume = 0;
    UInt32 numVolumeChannels = 0;
    UInt32 volumeDataSize = sizeof(totalVolume);
    Boolean hasVolumeProperty = NO;
    
    status = deviceInputChannels(device, &numChannels, channels);
    if (status != kAudioHardwareNoError) {
        *volume = -1;
        return status;
    }
    
    hasVolumeProperty = AudioObjectHasProperty(device, &volumeAddress);
    if (hasVolumeProperty) {
        status = AudioObjectGetPropertyData(device, &volumeAddress, 0, NULL, &volumeDataSize, (void *)volume);
        
        if (status == kAudioHardwareNoError) {
            totalVolume += *volume;
            numVolumeChannels++;
        }
    }
    
    for (UInt32 i = 0; i < numChannels; i++) {
        volumeAddress.mElement = channels[i];
        hasVolumeProperty = AudioObjectHasProperty(device, &volumeAddress);
        
        if (hasVolumeProperty) {
            status = AudioObjectGetPropertyData(device, &volumeAddress, 0, NULL, &volumeDataSize, (void *)volume);
            
            if (status == kAudioHardwareNoError) {
                totalVolume += *volume;
                numVolumeChannels++;
            }
        }
    }
    
    *volume = numVolumeChannels > 0? totalVolume / (Float32)numVolumeChannels: -1;
    return kAudioHardwareNoError;
}

OSStatus setDeviceInputVolume (AudioObjectID device, Float32 volume) {
    AudioObjectPropertyAddress volumeAddress = {
        .mSelector = kAudioDevicePropertyVolumeScalar,
        .mScope    = kAudioDevicePropertyScopeInput,
        .mElement  = kAudioObjectPropertyElementMaster
    };
    OSStatus status = kAudioHardwareNoError;
    Boolean gotOneToSet = NO;
    UInt32 channels[2];
    UInt32 numChannels = 2;
    Boolean hasVolumeProperty = NO;
    UInt32 volumeDataSize = sizeof(volume);
    
    status = deviceInputChannels(device, &numChannels, channels);
    if (status != kAudioHardwareNoError) {
        return status;
    }
    
    hasVolumeProperty = AudioObjectHasProperty(device, &volumeAddress);
    if (hasVolumeProperty) {
        status = AudioObjectSetPropertyData(device, &volumeAddress, 0, NULL, volumeDataSize, (void *)&volume);
        if (status == kAudioHardwareNoError) {
            gotOneToSet = YES;
        }
    }
    
    for (UInt32 i = 0; i < numChannels; i++) {
        volumeAddress.mElement = channels[i];
        hasVolumeProperty = AudioObjectHasProperty(device, &volumeAddress);
        
        if (hasVolumeProperty) {
            status = AudioObjectSetPropertyData(device, &volumeAddress, 0, NULL, volumeDataSize, (void *)&volume);
            if (status == kAudioHardwareNoError) {
                gotOneToSet = YES;
            }
        }
    }
    
    return gotOneToSet? kAudioHardwareNoError: status;
}
