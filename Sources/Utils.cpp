#include "Utils.h"

#include <charconv>
#include <memory>
#include <cxxabi.h>

bool Utils::IsUint(std::string_view s)
{
    unsigned int value;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);

    return ec == std::errc {} && ptr == s.data() + s.size();
}

uint32_t Utils::Str2Uint(std::string_view s)
{
    uint32_t res = 0;

    std::from_chars(s.data(), s.data() + s.size(), res);

    return res;
}

std::string Utils::Demangle(const std::string& mangledName)
{
    int status = -1;

    std::unique_ptr<char, void(*)(void*)> res {
        abi::__cxa_demangle(mangledName.c_str(), NULL, NULL, &status),
        std::free
    };

    return (status == 0) ? res.get() : mangledName;
}