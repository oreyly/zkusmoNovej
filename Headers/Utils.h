#pragma once

#include <string>
#include <cstdint>

class Utils
{
public:
	static bool IsUint(std::string_view s);
	static uint32_t Str2Uint(std::string_view s);
	static std::string Demangle(const std::string& mangledName);
};