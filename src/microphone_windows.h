#include <node.h>

using v8::FunctionCallbackInfo;
using v8::Value;

void list (const FunctionCallbackInfo<Value>& args);
void getVolume(const FunctionCallbackInfo<Value>& args);
void setVolume(const FunctionCallbackInfo<Value>& args);