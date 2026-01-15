#pragma once

#include "Message.h"
#include "Client.h"

class OutgoingRequest
{
public:
	OutgoingRequest(const Client& client, const OPCODE opcode, std::initializer_list<std::string> params, const uint32_t timeout, std::function<void()> onFailure, std::function<void()> onSuccess, const OPCODE expectedOpcode);
	~OutgoingRequest();

	void Stop();

	void OnRequestSend();
	void WaitForResponse();

	bool ValidateIncomingMessage(const IncomingMessage& incomingMessage);

	std::shared_ptr<OutgoingMessage> DemandMessage;

private:
	const uint32_t _timeout;

	std::function<void()> _onFailure;
	std::function<void()> _onSuccess;

	const OPCODE _expectedOpcode;

	std::unique_ptr<IncomingMessage> ResponseMessage;

	std::atomic<bool> _hasResult;
	std::atomic<bool> _waiting;

	std::mutex _resMutex;
	std::condition_variable _cvRes;
	std::thread _worker;
};

class IncomingRequest
{
public:
	IncomingRequest(const uint32_t clientId, const sockaddr_in& sourceAddr, const OPCODE opcode, std::vector<std::string> params);
private:
	const uint32_t _clientId;
	const OPCODE _opcode;

	const sockaddr_in _sourceAddress;

	std::vector<std::string> _parameters;
};