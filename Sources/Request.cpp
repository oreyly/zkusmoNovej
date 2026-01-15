#include "Request.h"

#include "Packet.h"

OutgoingRequest::OutgoingRequest(const Client& client, const OPCODE opcode, std::initializer_list<std::string> params, const uint32_t timeout, std::function<void()> onFailure, std::function<void()> onSuccess, OPCODE expectedOpcode) : _timeout(timeout), _expectedOpcode(expectedOpcode), _onFailure(onFailure), _onSuccess(onSuccess)
{
	_waiting.store(false, std::memory_order_release);
	_hasResult.store(false, std::memory_order_release);
	Packet p(client.Id, ORIGIN::SERVER, opcode, params);
	DemandMessage = std::make_shared<OutgoingMessage>(p, client.Addr, [this] ()
		{
			if (_hasResult.load(std::memory_order_acquire))
			{
				return;
			}

			_waiting.store(false, std::memory_order_release);
			_cvRes.notify_one();
			_onFailure();
		});
}

OutgoingRequest::~OutgoingRequest()
{
	Stop();
}

void OutgoingRequest::Stop()
{
	if (!_waiting.load(std::memory_order_acquire))
	{
		return;
	}

	DemandMessage->Stop();
	_waiting.store(false, std::memory_order_release);
	_cvRes.notify_one();
	if (_worker.joinable())
	{
		_worker.join();
	}
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
		_onFailure();
		return;
	}

	_onSuccess();
}

bool OutgoingRequest::ValidateIncomingMessage(const IncomingMessage& incomingMessage)
{
	if (!_waiting.load(std::memory_order_acquire))
	{
		return false;
	}

	bool validResponse = incomingMessage.MainPacket.Id == DemandMessage->MainPacket.Id && incomingMessage.MainPacket.Opcode == _expectedOpcode;

	if (validResponse)
	{
		ResponseMessage = std::make_unique<IncomingMessage>(incomingMessage);

		_waiting.store(false, std::memory_order_release);
		_hasResult.store(true, std::memory_order_release);
		_cvRes.notify_one();
	}

	return validResponse;
}

IncomingRequest::IncomingRequest(const uint32_t clientId, const sockaddr_in& sourceAddr, const OPCODE opcode, std::vector<std::string> params) : _clientId(clientId), _sourceAddress(sourceAddr), _opcode(opcode), _parameters(params)
{

}
