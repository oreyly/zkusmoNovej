#include "Packet.h"

#include "Utils.h"

#include <sstream>
#include <ranges>
#include <iostream>

#include "magic_enum.hpp"

std::atomic<uint32_t> Packet::_maxId {1};

Packet::Packet(const uint32_t targetId, const ORIGIN requestOrigin, const OPCODE opcode, std::initializer_list<std::string> params) : _id(NextId()), _clientId(targetId), _requestOrigin(requestOrigin), _opcode(opcode), _parameters(params)
{
    _isValid = true;
}

Packet::Packet(std::string message)
{
    std::vector<std::string> parts = SplitString(message);
    if (!(parts.size() >= 4 && Utils::IsUint(parts[0]) && Utils::IsUint(parts[1]) && Utils::IsUint(parts[2]) && Utils::IsUint(parts[3])))
    {
        std::cout << "NE" << std::endl;
        _isValid = false;
        return;
    }

    _id = Utils::Str2Uint(parts[0]);
    _clientId = Utils::Str2Uint(parts[1]);

    auto originCast = magic_enum::enum_cast<ORIGIN>(Utils::Str2Uint(parts[2]));

    if (!originCast.has_value())
    {
        _isValid = false;
        return;
    }

    _requestOrigin = originCast.value();

    auto opcodeCast = magic_enum::enum_cast<OPCODE>(Utils::Str2Uint(parts[3]));

    if (!opcodeCast.has_value())
    {
        _isValid = false;
        return;
    }

    _opcode = opcodeCast.value();

    for (std::string s : parts | std::views::drop(2))
    {
        _parameters.push_back(std::move(s));
    }

    _isValid = true;
}

std::vector<std::string> Packet::SplitString(const std::string& message)
{
    std::vector<std::string> parts;

    std::stringstream ss(message);
    std::string part;
    while (std::getline(ss, part, _delimiter))
    {
        parts.push_back(part);
    }

    return parts;
}

std::string Packet::CreateString() const
{
    std::stringstream ss;

    ss << Id << _delimiter << ClientId << _delimiter << static_cast<uint32_t>(_requestOrigin) << _delimiter << static_cast<uint32_t>(Opcode);
    for (const auto& param : Parameters)
    {
        ss << _delimiter << param;
    }

    return ss.str();
}

uint32_t Packet::NextId()
{
    return _maxId.fetch_add(1, std::memory_order_relaxed);
}