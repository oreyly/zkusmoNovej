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

std::size_t Utils::IntSockaddrKeyHash::operator()(const std::pair<uint32_t, sockaddr_in>& p) const
{
    auto h1 = std::hash<uint32_t> {}(p.first);
    auto h2 = std::hash<uint32_t> {}(p.second.sin_addr.s_addr);
    auto h3 = std::hash<uint16_t> {}(p.second.sin_port);

    std::size_t seed = h1;
    seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);

    return seed;
}

bool Utils::IntSockaddrKeyEqual::operator()(const std::pair<uint32_t, sockaddr_in>& lhs, const std::pair<uint32_t, sockaddr_in>& rhs) const
{
    return lhs.first == rhs.first &&
        lhs.second.sin_addr.s_addr == rhs.second.sin_addr.s_addr &&
        lhs.second.sin_port == rhs.second.sin_port &&
        lhs.second.sin_family == rhs.second.sin_family;
}

std::string Utils::Base64Encode(const std::string& in)
{
    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : in)
    {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0)
        {
            out.push_back(b64_table[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(b64_table[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}