//
//  CFAAudioObject.hpp
//  CFAMicrophone
//
//  Created by Jacob McDorman on 8/8/17.
//  Copyright © 2017 Cellaflora. All rights reserved.
//

#ifndef CFAAudioObject_h
#define CFAAudioObject_h

#include <stdio.h>
#import <Cocoa/Cocoa.h>
#include <CoreServices/CoreServices.h>
#include <CoreAudio/CoreAudio.h>

FOUNDATION_EXPORT double CFAMicrophoneVersionNumber;
FOUNDATION_EXPORT const unsigned char CFAMicrophoneVersionString[];

OSStatus deviceListCount (UInt32 *count);
OSStatus deviceList (UInt32 *count, AudioObjectID *devices);
OSStatus inputDeviceList (UInt32 *count, AudioObjectID *inputDevices);
OSStatus deviceWithUID (NSString *uid, AudioObjectID *device);

OSStatus deviceName (AudioObjectID device, NSString * __autoreleasing *name);
OSStatus deviceUID (AudioObjectID device, NSString * __autoreleasing *uid);
OSStatus deviceInputChannels (AudioObjectID device, UInt32 *count, UInt32 *channels);

OSStatus deviceInputVolume (AudioObjectID device, Float32 *volume);
OSStatus setDeviceInputVolume (AudioObjectID device, Float32 volume);

#endif
