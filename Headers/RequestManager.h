#pragma once

#include "Request.h"
#include "MessageManager.h"

#include <queue>

class RequestManager
{
public:
	RequestManager(uint32_t timeout);
	~RequestManager();

	void Stop();

	void ConnectToMessageManager(MessageManager& comunicator);
	void CreateDemand(const Client& client, const OPCODE opcode, const std::vector<std::string>& params, std::function<void(uint32_t, uint32_t)> onFailure, std::function<void(std::unique_ptr<IncomingRequest>)> onSuccess, const OPCODE expectedOpcode);
	void SendResponse(const Client& client, const OPCODE opcode, const std::vector<std::string>& params, std::function<void(uint32_t, uint32_t)> onTimeout);
	void RegisterProcessingFunction(std::function<void(std::shared_ptr<IncomingRequest>)> processingFunction);

private:
	const uint32_t _timeout;

	std::function<void(std::shared_ptr<IncomingRequest>)> _processingFunction;

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
