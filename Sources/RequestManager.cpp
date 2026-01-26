#include "RequestManager.h"

#include "Logger.h"

RequestManager::RequestManager(uint32_t timeout): _timeout(timeout)
{

}

RequestManager::~RequestManager()
{
	Stop();
}

void RequestManager::Stop()
{
	if (!_running.load(std::memory_order_acquire))
	{
		return;
	}

	_running.store(false, std::memory_order_release);
	_cvQueue.notify_one();

	if (_worker.joinable())
	{
		_worker.join();
	}
}


void RequestManager::CreateDemand(Client& client, const OPCODE opcode, const std::vector<std::string>& params, std::function<void(uint32_t)> onFailure, std::function<void(std::unique_ptr<IncomingRequest>)> onSuccess, const OPCODE expectedOpcode)
{
	{
		std::lock_guard<std::mutex> lock(_queueMutex);

		_demandQueue.push(std::make_unique<OutgoingRequest>(client, opcode, params, _timeout, onFailure, onSuccess, expectedOpcode));
	}

	_cvQueue.notify_one();
}

void RequestManager::SendResponse(Client& client, const OPCODE opcode, const std::vector<std::string>& params, std::function<void(uint32_t)> onTimeout)
{
	_messageManager->get().SendMessage(std::make_shared<OutgoingMessage>(Packet(client.Id, ORIGIN::CLIENT, opcode, params), client.Addr, onTimeout));
}

void RequestManager::RegisterProcessingFunction(std::function<void(std::shared_ptr<IncomingRequest>)> processingFunction)
{
	if (_processingFunction)
	{
		Logger::LogError<MessageManager>(ERROR_CODES::ALREAD_REGISTERED);
	}

	_processingFunction = processingFunction;
}

void RequestManager::ConnectToMessageManager(MessageManager& messageManager)
{
	if (_running.load(std::memory_order_acquire))
	{
		Logger::LogError<RequestManager>(ERROR_CODES::ALREADY_CONNECTED_MANAGER);
	}

	_running.store(true, std::memory_order_release);

	_messageManager = messageManager;
	_messageManager->get().RegisterProcessingFunction([this] (const IncomingMessage incommingMessage)
		{
			OnMessageRecieve(incommingMessage);
		});

	_worker = std::thread(&RequestManager::MainLoop, this);
}

void RequestManager::OnMessageRecieve(IncomingMessage incomingMessage)
{
	if (incomingMessage.MainPacket.RequestOrigin == ORIGIN::SERVER)
	{
		std::cout << incomingMessage.MainPacket.CreateString() << std::endl;
		{
			std::lock_guard<std::mutex> lock(_ourDemandMutex);
			auto corespondingDemand = _ourDemands.find(incomingMessage.MainPacket.ClientId);

			if (corespondingDemand != _ourDemands.end())
			{
				auto& targetVec = corespondingDemand->second;

				auto itVec = std::find_if(targetVec.begin(), targetVec.end(), [&] (const auto& demand)
					{
						return demand->ValidateIncomingMessage(incomingMessage);
					});

				if (itVec != targetVec.end())
				{
					targetVec.erase(itVec);
				}
			}

			return;
		}
	}

	if (incomingMessage.MainPacket.RequestOrigin == ORIGIN::CLIENT && _processingFunction)
	{
		_processingFunction(std::make_shared<IncomingRequest>(incomingMessage.MainPacket.ClientId, incomingMessage.ClientAddres, incomingMessage.MainPacket.Opcode, incomingMessage.MainPacket.Parameters));

		return;
	}

	Logger::LogError<RequestManager>(ERROR_CODES::UNKNOWN_ORIGIN);
}

void RequestManager::MainLoop()
{
	while (_running.load(std::memory_order_acquire))
	{
		std::unique_ptr<OutgoingRequest> outgoingRequest;

		{
			std::unique_lock<std::mutex> lock(_queueMutex);

			_cvQueue.wait(lock, [this] ()
				{
					return !_running.load(std::memory_order_acquire) || !_demandQueue.empty();
				});

			if (!_running.load(std::memory_order_acquire))
			{
				return;
			}

			outgoingRequest = std::move(_demandQueue.front());
			_demandQueue.pop();
		}

		{
			std::lock_guard<std::mutex> lock(_ourDemandMutex);
			uint32_t clientId = outgoingRequest->DemandMessage->MainPacket.ClientId;
			_ourDemands[clientId].push_back(std::move(outgoingRequest));
			_ourDemands[clientId].back()->OnRequestSend();
			_messageManager->get().SendMessage(_ourDemands[clientId].back()->DemandMessage);
			//_ourDemands.try_emplace(outgoingRequest.get()->DemandMessage->MainPacket.TargetId, std::move(outgoingRequest));
		}
	}
}
