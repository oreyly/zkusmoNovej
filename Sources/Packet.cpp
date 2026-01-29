#include "Packet.h"

#include "Utils.h"

#include <sstream>
#include <ranges>
#include <iostream>

#include "magic_enum.hpp"

std::atomic<uint32_t> Packet::_maxId {1};

Packet::Packet(const uint32_t id, const uint32_t targetId, const uint32_t connectionID, const ORIGIN requestOrigin, const OPCODE opcode) : _id(id), _clientId(targetId), _connectionID(connectionID), _requestOrigin(requestOrigin), _opcode(opcode), _parameters({})
{
    _isValid = true;
}

Packet::Packet(const uint32_t targetId, const uint32_t connectionID, const ORIGIN requestOrigin, const OPCODE opcode, const std::vector<std::string>& params) : _id(NextId()), _clientId(targetId), _connectionID(connectionID), _requestOrigin(requestOrigin), _opcode(opcode), _parameters(params)
{
    _isValid = true;
}

Packet::Packet(const Packet& other) : _id(other.Id), _clientId(other.ClientId), _connectionID(other.ConnectionID), _requestOrigin(other.RequestOrigin), _opcode(other.Opcode), _parameters(other.Parameters)
{

}

Packet& Packet::operator =(const Packet& other)
{
    if (this != &other)
    {
        _id = other.Id;
        _clientId = other.ClientId;
        _requestOrigin = other.RequestOrigin;
        _opcode = other.Opcode;
        _parameters = other.Parameters;
    }
    return *this;
}

Packet::Packet(std::string message)
{
    int partIndex = 0;

    std::vector<std::string> parts = SplitString(message);
    if (!(parts.size() >= 5 && Utils::IsUint(parts[0]) && Utils::IsUint(parts[1]) && Utils::IsUint(parts[2]) && Utils::IsUint(parts[3]) && Utils::IsUint(parts[0])))
    {
        std::cout << "NE" << std::endl;
        _isValid = false;
        return;
    }

    _id = Utils::Str2Uint(parts[partIndex++]);
    _clientId = Utils::Str2Uint(parts[partIndex++]);
    _connectionID = Utils::Str2Uint(parts[partIndex++]);

    auto originCast = magic_enum::enum_cast<ORIGIN>(Utils::Str2Uint(parts[partIndex++]));

    if (!originCast.has_value())
    {
        std::cout << "NEeeeeee" << std::endl;
        _isValid = false;
        return;
    }

    _requestOrigin = originCast.value();

    auto opcodeCast = magic_enum::enum_cast<OPCODE>(Utils::Str2Uint(parts[partIndex++]));

    if (!opcodeCast.has_value())
    {
        std::cout << "NEEEEEEEEE" << std::endl;
        _isValid = false;
        return;
    }

    _opcode = opcodeCast.value();

    for (std::string s : parts | std::views::drop(partIndex))
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

    ss << Id << _delimiter << ClientId << _delimiter << ConnectionID << _delimiter << static_cast<uint32_t>(_requestOrigin) << _delimiter << static_cast<uint32_t>(Opcode);
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
