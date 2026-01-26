#include "Request.h"

#include "Packet.h"

#include <iostream>

OutgoingRequest::OutgoingRequest(const Client& client, const OPCODE opcode, const std::vector<std::string>& params, const uint32_t timeout, std::function<void(uint32_t)> onFailure, std::function<void(const std::unique_ptr<IncomingRequest>)> onSuccess, const OPCODE expectedOpcode) : _timeout(timeout), _expectedOpcode(expectedOpcode), _onFailure(onFailure), _onSuccess(onSuccess)
{
	_waiting.store(false, std::memory_order_release);
	_hasResult.store(false, std::memory_order_release);
	Packet p(client.Id, ORIGIN::SERVER, opcode, params);
	DemandMessage = std::make_shared<OutgoingMessage>(p, client.Addr, [this] (uint32_t id)
		{
			if (_hasResult.load(std::memory_order_acquire))
			{
				return;
			}

			_waiting.store(false, std::memory_order_release);
			_cvRes.notify_one();
			_onFailure(id);
		});
}

OutgoingRequest::~OutgoingRequest()
{
	Stop();

	std::unique_lock<std::mutex> lock(_stopMutex);
	_cvStop.wait(lock, [this] ()
		{
			return !_stopping.load(std::memory_order_acquire);
		});
}

void OutgoingRequest::Stop()
{
	if (_stopping.load(std::memory_order_acquire))
	{
		return;
	}

	_stopping.store(true, std::memory_order_release);

	DemandMessage->Stop();
	_waiting.store(false, std::memory_order_release);
	_cvRes.notify_one();
	if (_worker.joinable())
	{
		_worker.join();
	}

	_stopping.store(false, std::memory_order_release);
	_cvStop.notify_one();
}

void OutgoingRequest::OnRequestSend()
{
	_waiting.store(true, std::memory_order_release);
	_worker = std::thread(&OutgoingRequest::WaitForResponse, this);
}

void OutgoingRequest::WaitForResponse()
{
	if (!_waiting.load(std::memory_order_acquire))
	{
		return;
	}

	std::unique_lock<std::mutex> lock(_resMutex);

	_cvRes.wait_for(lock, std::chrono::milliseconds(_timeout), [this] ()
		{
			return !_waiting.load(std::memory_order_acquire);
		});

	if (!_hasResult.load(std::memory_order_acquire))
	{
		_waiting.store(false, std::memory_order_release);
		_onFailure(DemandMessage->MainPacket.ClientId);
		return;
	}

	_onSuccess(std::make_unique<IncomingRequest>(ResponseMessage->MainPacket.ClientId, ResponseMessage->ClientAddres, ResponseMessage->MainPacket.Opcode, ResponseMessage->MainPacket.Parameters));
}

bool OutgoingRequest::ValidateIncomingMessage(const IncomingMessage& incomingMessage)
{
	if (!_waiting.load(std::memory_order_acquire))
	{
		return false;
	}

	bool validResponse = incomingMessage.MainPacket.Opcode == _expectedOpcode;

	if (validResponse)
	{
		ResponseMessage = std::make_unique<IncomingMessage>(incomingMessage);

		_waiting.store(false, std::memory_order_release);
		_hasResult.store(true, std::memory_order_release);
		_cvRes.notify_one();
	}

	return validResponse;
}

IncomingRequest::IncomingRequest(const uint32_t clientId, const sockaddr_in& sourceAddr, const OPCODE opcode, std::vector<std::string> params) : ClientId(clientId), SourceAddress(sourceAddr), Opcode(opcode), Parameters(params)
{

}
