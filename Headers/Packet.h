#pragma once

#include "opcode.h"

#include <string>
#include <vector>
#include <atomic>

enum class ORIGIN : uint32_t
{
	SERVER,
	CLIENT
};

class Packet
{
public:
	const uint32_t& Id = _id;
	const uint32_t& ClientId = _clientId;
	const ORIGIN& RequestOrigin = _requestOrigin;
	const OPCODE& Opcode = _opcode;
	const std::vector<std::string>& Parameters = _parameters;
	const bool& IsValid = _isValid;

	Packet(const uint32_t targetId, const ORIGIN requestOrigin, const OPCODE opcode, std::initializer_list<std::string> params);
	Packet(const uint32_t targetId, const ORIGIN requestOrigin, const OPCODE opcode) : Packet(targetId, requestOrigin, opcode, {}) { }
	Packet(std::string message);

	std::string CreateString() const;

private:
	uint32_t _id;
	uint32_t _clientId;
	ORIGIN _requestOrigin;
	OPCODE _opcode;
	std::vector<std::string> _parameters;
	bool _isValid;

	static const char _delimiter = '-';
	static std::atomic<uint32_t> _maxId;

	static uint32_t NextId();
	static std::vector<std::string> SplitString(const std::string& message);
};