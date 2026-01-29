#pragma once

#include "Message.h"
#include "Client.h"

class IncomingRequest;

class OutgoingRequest
{
public:
	OutgoingRequest(const Client& client, const OPCODE opcode, const std::vector<std::string>& params, const uint32_t timeout, std::function<void(uint32_t, uint32_t)> onFailure, std::function<void(const std::unique_ptr<IncomingRequest>)> onSuccess, const OPCODE expectedOpcode);
	~OutgoingRequest();

	void Stop();

	void OnRequestSend();

	bool ValidateIncomingMessage(const IncomingMessage& incomingMessage);

	std::shared_ptr<OutgoingMessage> DemandMessage;

private:
	std::atomic<bool> _stopping;
	std::mutex _stopMutex;
	std::condition_variable _cvStop;

	const uint32_t _timeout;

	std::function<void(uint32_t, uint32_t)> _onFailure;
	std::function<void(const std::unique_ptr<IncomingRequest>)> _onSuccess;

	const OPCODE _expectedOpcode;

	std::unique_ptr<IncomingMessage> ResponseMessage;

	std::atomic<bool> _hasResult;
	std::atomic<bool> _waiting;

	std::mutex _resMutex;
	std::condition_variable _cvRes;
	std::thread _worker;
	void WaitForResponse();
};

class IncomingRequest
{
public:
	IncomingRequest(const uint32_t clientId, const sockaddr_in& sourceAddr, const OPCODE opcode, std::vector<std::string> params);
	const uint32_t ClientId;
	const OPCODE Opcode;

	const sockaddr_in SourceAddress;

	std::vector<std::string> Parameters;
private:
};
