#pragma once
#include <napi.h>
#include <windows.h>
#include <string>

const size_t MaxFrameSize = 64 * 1024;
struct MessageFrameHeader {
	uint32_t opcode;
	uint32_t length;
};
struct MessageFrame : public MessageFrameHeader {
	char message[MaxFrameSize - sizeof(MessageFrameHeader)];
};

class BaseConnectionWin : public Napi::ObjectWrap<BaseConnectionWin> {
public:
	BaseConnectionWin(const Napi::CallbackInfo& info);
	Napi::Value Open(const Napi::CallbackInfo& info);
	Napi::Value Close(const Napi::CallbackInfo& info);
	Napi::Value Write(const Napi::CallbackInfo& info);
	Napi::Value Read(const Napi::CallbackInfo& info);
	static Napi::Function GetClass(Napi::Env env);
	bool isOpen{ true };
private:
	HANDLE pipe{ INVALID_HANDLE_VALUE };
	bool _Read(void* data, size_t length);
	bool _Close();
};