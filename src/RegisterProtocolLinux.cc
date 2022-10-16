#include "RegisterProtocol.h"
Napi::Value Register(const Napi::CallbackInfo& info) {
	if (info.Length() != 2) {
		Napi::TypeError::New(info.Env(), "Two Arguments are required").ThrowAsJavaScriptException();
	}
	if (!info[0].IsString()) {
		Napi::TypeError::New(info.Env(), "`client_id` must be a string`").ThrowAsJavaScriptException();
	}
	if (!info[1].IsString()) {
		Napi::TypeError::New(info.Env(), "`command` must be a string`").ThrowAsJavaScriptException();
	}
	const std::string command = info[1].As<Napi::String>().Utf8Value();
	const std::string clientID = info[0].As<Napi::String>().Utf8Value();
	/*std::string protocolDesc = "URL:Connect to " + clientID;
	std::string classPath = "Software\\Classes\\discord-" + clientID;
	HKEY key;
	auto status = RegCreateKeyExA(HKEY_CURRENT_USER, classPath.c_str(), NULL, nullptr, NULL, KEY_WRITE, nullptr, &key, nullptr);
	if (status != ERROR_SUCCESS) {
		Napi::Error::New(info.Env(), "Error creating registry class").ThrowAsJavaScriptException();
	}
	status = RegSetValueExA(key, nullptr, NULL, REG_SZ, (BYTE*)protocolDesc.c_str(), protocolDesc.length());
	if (status != ERROR_SUCCESS) {
		Napi::Error::New(info.Env(), "Error writing protocol description").ThrowAsJavaScriptException();
	}
	status = RegSetKeyValueA(key, nullptr, "URL Protocol", REG_SZ, nullptr, 0);
	if (status != ERROR_SUCCESS) {
		Napi::Error::New(info.Env(), "Error linkingg as protocol").ThrowAsJavaScriptException();
	}
	status = RegSetKeyValueA(key, "shell\\open\\command", nullptr, REG_SZ, command.c_str(), command.length());
	if (status != ERROR_SUCCESS) {
		Napi::Error::New(info.Env(), "Error linking command").ThrowAsJavaScriptException();
	}
	return info.Env().Undefined();*/
}