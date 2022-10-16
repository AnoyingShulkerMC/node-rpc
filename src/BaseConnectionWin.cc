#include "BaseConnectionWin.h"
#include "RegisterProtocol.h"

#define WIN32_LEAN_AND_MEAN
#define NOMCX
#define NOSERVICE
#define NOIME

BaseConnectionWin::BaseConnectionWin(const Napi::CallbackInfo& info) : ObjectWrap(info) {};

Napi::Value BaseConnectionWin::Open(const Napi::CallbackInfo& info) {
	int pipe = 0;
	if (info.Length() > 0 && info[0].IsNumber()) {
		napi_get_value_int32(info.Env(), info[0], &pipe);
	}
	wchar_t pipeName[] = { L"\\\\?\\pipe\\discord-ipc-0" };
	const size_t pipeDigit = sizeof(pipeName) / sizeof(wchar_t) - 2;
	for (int i = 0; i < pipe; i++) {
		pipeName[pipeDigit]++;
	}
	for (;;) {
		this->pipe = CreateFileW(pipeName, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
		if (this->pipe != INVALID_HANDLE_VALUE) {
			this->isOpen = true;
			return Napi::Boolean::New(info.Env(), true);
		}
		auto err = GetLastError();
		if (err == ERROR_FILE_NOT_FOUND) {
			if (pipeName[pipeDigit] < L'9') {
				pipeName[pipeDigit]++;
				continue;
			}
			else {
				Napi::Error::New(info.Env(), "Discord is not active").ThrowAsJavaScriptException();
			}
		}
		else if (err == ERROR_PIPE_BUSY) {
			if (!WaitNamedPipeW(pipeName, 10000)) {
				Napi::Error::New(info.Env(), "Pipe timed out").ThrowAsJavaScriptException();
				return Napi::Boolean::New(info.Env(), false);
			}
		}
		Napi::Error::New(info.Env(), "Windows failed with error ").ThrowAsJavaScriptException();
		return Napi::Boolean::New(info.Env(), false);
	}
	return Napi::Boolean::New(info.Env(), true);
}
Napi::Value BaseConnectionWin::Close(const Napi::CallbackInfo& info) {
	_Close();
	return Napi::Boolean::New(info.Env(), true);
}
Napi::Value BaseConnectionWin::Write(const Napi::CallbackInfo& info) {
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
	DWORD length = static_cast<DWORD>(sizeof(MessageFrameHeader) + len);
	DWORD written = 0;
	const bool success = WriteFile(this->pipe, &message, length, &written, nullptr) == TRUE;
	return Napi::Boolean::New(info.Env(), success  && written == length);
}
Napi::Value BaseConnectionWin::Read(const Napi::CallbackInfo& info) {
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
}

bool BaseConnectionWin::_Close() {
	CloseHandle(this->pipe);
	this->isOpen = false;
	return true;
}
bool BaseConnectionWin::_Read(void* data, size_t length) {
	if (!data) return false;
	if (this->pipe == INVALID_HANDLE_VALUE) {
		return false;
	}
	DWORD bytesAvailable = 0;
	if (PeekNamedPipe(this->pipe, nullptr, NULL, nullptr, &bytesAvailable, nullptr)) {
		if (bytesAvailable >= length) {
			DWORD bytestoRead = (DWORD)length;
			DWORD bytesRead = 0;
			if (ReadFile(this->pipe, data, bytestoRead, &bytesRead, nullptr) == TRUE) {
				return true;
			}
			else {
				_Close();
			}
		}
	}
	else {
		_Close();
	}
	return false;
}
Napi::Function BaseConnectionWin::GetClass(Napi::Env env) {
	return DefineClass(
		env,
		"BaseConnection",
		{
			InstanceMethod("open", &BaseConnectionWin::Open),
			InstanceMethod("close", &BaseConnectionWin::Close),
			InstanceMethod("write", &BaseConnectionWin::Write),
			InstanceMethod("read", &BaseConnectionWin::Read)
		}
	);
}
Napi::Object init(Napi::Env env, Napi::Object exports) {
	exports.Set(Napi::String::New(env, "Register"), Napi::Function::New(env, Register));
	exports.Set(Napi::String::New(env, "BaseConnection"), BaseConnectionWin::GetClass(env));
	return exports;
}

NODE_API_MODULE(addon, init);