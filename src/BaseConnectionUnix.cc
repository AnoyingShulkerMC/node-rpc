#include "BaseConnectionUnix.h"
#include "RegisterProtocol.h"

static sockaddr_un PipeAddr{};

#ifdef MSG_NOSIGNAL
static int MsgFlags = MSG_NOSIGNAL;
#else
static int MsgFlags = 0;
#endif
static const char* GetTempPath()
{
    const char* temp = getenv("XDG_RUNTIME_DIR");
    temp = temp ? temp : getenv("TMPDIR");
    temp = temp ? temp : getenv("TMP");
    temp = temp ? temp : getenv("TEMP");
    temp = temp ? temp : "/tmp";
    return temp;
}
BaseConnectionUnix::BaseConnectionUnix(const Napi::CallbackInfo& info) : ObjectWrap(info) {};
Napi::Value BaseConnectionUnix::Open(const Napi::CallbackInfo& info) {
  int pipeNum = 0;
  if (info.Length() > 0 && info[0].IsNumber()) {
    napi_get_value_int32(info.Env(), info[0], &pipeNum);
  }
  const char* tempPath = GetTempPath();
  this->sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (this->sock == -1) Napi::Error::New(info.Env(), "Error connecting").ThrowAsJavaScriptException();
  fcntl(this->sock, F_SETFL, O_NONBLOCK);
#ifdef SO_NOSIGPIPE
  int optval = 1;
  setsocopt(this->sock, SOL_SOCKET, SO_NOSIGPIPE, &optval, sizeof(optval));
#endif
  for (; pipeNum < 10; ++pipeNum) {
    snprintf(PipeAddr.sun_path, sizeof(PipeAddr.sun_path), "%s/discord-ipc-%d", tempPath, pipeNum);
    int err = connect(this->sock, (const sockaddr*)&PipeAddr, sizeof(PipeAddr));
    if (err == 0) {
      this->isOpen = true;
      return info.Env().Undefined();
    }
  }
  _Close();
  Napi::Error::New(info.Env(), "Discord is not Active").ThrowAsJavaScriptException();
  return info.Env().Undefined();
};
Napi::Value BaseConnectionUnix::Close(const Napi::CallbackInfo& info) {
  _Close();
  return info.Env().Undefined();
};
Napi::Value BaseConnectionUnix::Write(const Napi::CallbackInfo& info) {
  if (info.Length() != 2) {
    napi_throw_type_error(info.Env(), "", "You need two argument!");
    return Napi::Boolean::New(info.Env(), false);
  }
  if (!info[0].IsNumber()) {
    napi_throw_type_error(info.Env(), "", "`opcode` must be a number");
    return Napi::Boolean::New(info.Env(), false);
  }
  if (!info[1].IsString()) {
    napi_throw_type_error(info.Env(), "", "`message` must be a string");
    return Napi::Boolean::New(info.Env(), false);
  }
  MessageFrame message;
  napi_get_value_uint32(info.Env(), info[0], &message.opcode);
  size_t len;
  napi_status status = napi_get_value_string_utf8(info.Env(), info[1], message.message, sizeof(message.message), &len);
  message.length = len;
  if (status != napi_ok) {
    Napi::Error::New(info.Env(), "NAPI failed with error ").ThrowAsJavaScriptException();
    return Napi::Boolean::New(info.Env(), false);
  }
  if (this->sock == -1) {
    Napi::Error::New(info.Env(), "The connection isn't open yet.").ThrowAsJavaScriptException();
  }
  ssize_t sentBytes = send(this->sock, &message, (ssize_t)(sizeof(MessageFrameHeader) + message.length), MsgFlags);
  if (sentBytes != (ssize_t)(sizeof(MessageFrameHeader) + message.length)) Napi::Error::New(info.Env(), "Error writing message").ThrowAsJavaScriptException();
  return info.Env().Undefined();
};
Napi::Value BaseConnectionUnix::Read(const Napi::CallbackInfo& info) {

  MessageFrame message;
  bool didRead = _Read(&message, sizeof(MessageFrameHeader));
  if (!didRead) {
    if (!this->isOpen) {
      napi_throw_error(info.Env(), "", "The Connection is not open yet.");
      _Close();
      return Napi::Boolean::New(info.Env(), false);
    }
    return info.Env().Null();
  }
  if (message.length > 0) {
    didRead = _Read(message.message, message.length);
    if (!didRead) {
      napi_throw_error(info.Env(), "", "Read corrupt");
      _Close();
      return Napi::Boolean::New(info.Env(), false);
    }
    message.message[message.length] = 0;
  }
  Napi::Object returnObj = Napi::Object::New(info.Env());
  returnObj.Set("opcode", Napi::Number::New(info.Env(), message.opcode));
  returnObj.Set("message", Napi::String::New(info.Env(), message.message, message.length));
  return returnObj;
};

bool BaseConnectionUnix::_Close() {
  if (this->sock == -1) return false;
  close(this->sock);
  this->sock = -1;
  this->isOpen = false;
  return true;
};
bool BaseConnectionUnix::_Read(void* data, size_t length) {
  if (this->sock == -1) return false;
  int res = (int)recv(this->sock, data, length, MsgFlags);
  if (res < 0) {
    if (errno == EAGAIN) {
      return false;
    }
    _Close();
  }
  else if (res == 0) {
    _Close();
  }
  return res == (int)length;
};
Napi::Function BaseConnectionUnix::GetClass(Napi::Env env) {
  return DefineClass(
    env,
    "BaseConnection",
    {
      InstanceMethod("open", &BaseConnectionUnix::Open),
      InstanceMethod("close", &BaseConnectionUnix::Close),
      InstanceMethod("write", &BaseConnectionUnix::Write),
      InstanceMethod("read", &BaseConnectionUnix::Read)
    }
  );
}
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "Register"), Napi::Function::New(env, Register));
  exports.Set(Napi::String::New(env, "BaseConnection"), BaseConnectionUnix::GetClass(env));
  return exports;
}
NODE_API_MODULE(addon, Init)