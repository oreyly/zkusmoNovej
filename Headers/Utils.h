#pragma once

#include <string>
#include <cstdint>
#include <netinet/in.h>


class Utils
{
public:
	static bool IsUint(std::string_view s);
	static uint32_t Str2Uint(std::string_view s);
	static std::string Demangle(const std::string& mangledName);
	static std::string Base64Encode(const std::string& in);

	struct IntSockaddrKeyHash
	{
		std::size_t operator()(const std::pair<uint32_t, sockaddr_in>& p) const;
	};


	struct IntSockaddrKeyEqual
	{
		bool operator()(const std::pair<uint32_t, sockaddr_in>& lhs, const std::pair<uint32_t, sockaddr_in>& rhs) const;
	};


private:
	static inline const std::string b64_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
};
