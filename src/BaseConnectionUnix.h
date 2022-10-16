#include <napi.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

const size_t MaxFrameSize = 64 * 1024;
struct MessageFrameHeader {
	uint32_t opcode;
	uint32_t length;
};
struct MessageFrame : public MessageFrameHeader {
	char message[MaxFrameSize - sizeof(MessageFrameHeader)];
};

class BaseConnectionUnix : public Napi::ObjectWrap<BaseConnectionUnix> {
public:
	BaseConnectionUnix(const Napi::CallbackInfo& info);
	Napi::Value Open(const Napi::CallbackInfo& info);
	Napi::Value Close(const Napi::CallbackInfo& info);
	Napi::Value Write(const Napi::CallbackInfo& info);
	Napi::Value Read(const Napi::CallbackInfo& info);
	static Napi::Function GetClass(Napi::Env env);
	bool isOpen{ true };
private:
	int sock{ -1 };
  bool _Read(void* data, size_t length);
	bool _Close();
};