#include <node.h>

using v8::Local;
using v8::Object;

#ifdef __APPLE__
#include "microphone_darwin.h"
#endif

#ifdef WIN32
#include "microphone_windows.h"
#endif

void init(Local<Object> exports) {
	NODE_SET_METHOD(exports, "list", list);
	NODE_SET_METHOD(exports, "getVolume", getVolume);
	NODE_SET_METHOD(exports, "setVolume", setVolume);
}

NODE_MODULE(addon, init)