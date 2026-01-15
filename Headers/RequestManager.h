#pragma once

#include "Request.h"
#include "MessageManager.h"

#include <queue>

class RequestManager
{
public:
	RequestManager();
	~RequestManager();

	void Stop();

	void ConnectToComunicator(MessageManager& comunicator);
	void CreateDemand(Client& client, const OPCODE opcode, std::initializer_list<std::string> params, const uint32_t timeout, std::function<void()> onFailure, std::function<void()> onSuccess, const OPCODE expectedOpcode);
	void SendResponse(Client& client, const OPCODE opcode, std::initializer_list<std::string> params, std::function<void()> onTimeout);
	void RegisterProcessingFunction(std::function<void(IncomingRequest)> processingFunction);

private:
	std::function<void(IncomingRequest)> _processingFunction;

	std::queue<std::unique_ptr<OutgoingRequest>> _demandQueue;

	std::unordered_map<uint32_t, std::vector<std::unique_ptr<OutgoingRequest>>> _ourDemands;

	std::queue<IncomingMessage> _incomingQueue;

	std::optional<std::reference_wrapper<MessageManager>> _messageManager;

	std::atomic<bool> _running;
	std::thread _worker;

	std::mutex _queueMutex;
	std::condition_variable _cvQueue;

	std::mutex _ourDemandMutex;

	void OnMessageRecieve(IncomingMessage incomingMessage);
	void MainLoop();
};