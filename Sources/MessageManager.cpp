#include "MessageManager.h"

#include "Logger.h"

MessageManager::MessageManager()
{
	Logger::LogMessage<MessageManager>("Vytvořen MessageManager.");
}

MessageManager::~MessageManager()
{
	Stop();

	/*std::unique_lock<std::mutex> lock(_stopMutex);
	_cvStop.wait(lock, [this] ()
		{
			return !_stopping.load(std::memory_order_acquire);
		});*/
}

void MessageManager::Stop()
{
	if (!_running.load(std::memory_order_acquire))
	{
		return;
	}

	_stopping.store(true, std::memory_order_release);
	_running.store(false, std::memory_order_release);
	_cvMessageArrived.notify_one();

	std::cout << "Stopnuto2" << std::endl;

	if (_messageWorker.joinable())
	{
		_messageWorker.join();
	}

	std::cout << "Stopnuto2" << std::endl;

	if (_cleaningWorker.joinable())
	{
		_cleaningWorker.join();
	}
	std::cout << "Stopnuto2" << std::endl;

	_stopping.store(false, std::memory_order_release);
	_cvStop.notify_one();
}

void MessageManager::ConnectToComunicator(Comunicator& comunicator, const uint32_t messageTimeout, const uint32_t maxMessageAttempts)
{
	if (_running.load(std::memory_order_acquire))
	{
		Logger::LogError<MessageManager>(ERROR_CODES::ALREADY_CONNECTED_MANAGER);
		return;
	}

	_messageTimeout = messageTimeout;
	_maxMessageAttempts = maxMessageAttempts;

	_running.store(true, std::memory_order_release);

	_cleaningWorker = std::thread(&MessageManager::CleaningLoop, this);
	_messageWorker = std::thread(&MessageManager::MainLoop, this);

	_comunicator = comunicator;
	_comunicator->get().RegisterReceiver([this] (const sockaddr_in& targetAddr,std::string message)
		{
			OnStringRecieve(targetAddr, message);
		});

	Logger::LogMessage<MessageManager>("MessageManager připojen ke komunikátoru.");
}

void MessageManager::RegisterProcessingFunction(std::function<void(IncomingMessage&)> processingFunction)
{
	if (_processingFunction)
	{
		Logger::LogError<MessageManager>(ERROR_CODES::ALREAD_REGISTERED);
	}

	_processingFunction = processingFunction;
}

void MessageManager::SendMessage(std::shared_ptr<OutgoingMessage> outgoingMessage)
{
	std::lock_guard<std::mutex> lock(_outgoingMutex);

	outgoingMessage->RegisterMessageManager(*this, _messageTimeout, _maxMessageAttempts);
	_outgoingMessages[outgoingMessage->MainPacket.Id] = outgoingMessage;
	SendPacket(outgoingMessage->ClientAddres, outgoingMessage->MainPacket);
	outgoingMessage->OnMessageSent();
}

void MessageManager::SendPacket(const sockaddr_in& targetAddr, const Packet& packet)
{
	if (!_comunicator)
	{
		Logger::LogError<MessageManager>(ERROR_CODES::COMMUNICATOR_NOT_INITIALIZED);
	}

	_comunicator->get().Send(targetAddr, packet.CreateString());
}

void MessageManager::CleaningLoop()
{
	while (_running.load(std::memory_order_acquire))
	{
		CleanFinished();
		std::this_thread::sleep_for(std::chrono::milliseconds(CLEANER_SLEEP_MS));
	}

	CleanFinished();
}

void MessageManager::CleanFinished()
{
	std::lock_guard<std::mutex> lock(_finishedMutex);

	auto now = std::chrono::steady_clock::now();
	auto limit = std::chrono::minutes(1);

	while (!_finishedIdTimesListOur.empty() && (now - _finishedIdTimesListOur.front().second) > limit)
	{
		_finishedIdTimesMapOur.erase(_finishedIdTimesListOur.front().first);
		_finishedIdTimesListOur.pop_front();
	}

	while (!_finishedIdTimesListTheir.empty() && (now - _finishedIdTimesListTheir.front().second) > limit)
	{
		_finishedIdTimesMapTheir.erase(_finishedIdTimesListTheir.front().first);
		_finishedIdTimesListTheir.pop_front();
	}
}

void MessageManager::OnStringRecieve(const sockaddr_in& targetAddr, std::string& message)
{
	if (!_running.load(std::memory_order_acquire))
	{
		return;
	}

	{
		std::lock_guard<std::mutex> lock(_arrivingMutex);

		_arrivedMessages.push({targetAddr, message});
	}

	_cvMessageArrived.notify_one();
}

void MessageManager::MainLoop()
{
	while(_running.load(std::memory_order_acquire))
	{
		std::string message;
		sockaddr_in sourceAddr;
		{
			std::unique_lock<std::mutex> lock(_arrivingMutex);

			_cvMessageArrived.wait(lock, [this] ()
				{
					return !_arrivedMessages.empty() || !_running.load(std::memory_order_acquire);
				});

			if (!_running.load(std::memory_order_acquire))
			{
				return;
			}

			std::tie(sourceAddr, message) = std::move(_arrivedMessages.front());
			_arrivedMessages.pop();
		}

		Packet recievedPacket(message);
		std::cout << "V:" << recievedPacket.IsValid << std::endl;
		if (!recievedPacket.IsValid)
		{
			continue;
		}

		if (recievedPacket.Opcode == OPCODE::ACK)
		{
			{
				std::scoped_lock lock(_outgoingMutex, _finishedMutex);

				auto reqIt = _outgoingMessages.find(recievedPacket.Id);

				if (reqIt == _outgoingMessages.end())
				{
					continue;
				}

				_finishedIdTimesListOur.push_back({recievedPacket.Id,std::chrono::steady_clock::now()});
				_finishedIdTimesMapOur[recievedPacket.Id] = std::prev(_finishedIdTimesListOur.end());

				_outgoingMessages.at(recievedPacket.Id)->Finish();
			}

			continue;
		}

		{
			std::lock_guard<std::mutex> lock(_finishedMutex);

			if (recievedPacket.RequestOrigin == ORIGIN::CLIENT && _finishedIdTimesMapTheir.contains({recievedPacket.Id, sourceAddr}))
			{
				continue;
			}
		}

		IncomingMessage incommingMessage(recievedPacket, sourceAddr);
		ConfirmReceiving(incommingMessage);

		{
			std::scoped_lock lock(_incomingMutex, _finishedMutex);

			auto reqIt = _incomingMessages.find({incommingMessage.MainPacket.Id, sourceAddr});

			if (reqIt != _incomingMessages.end())
			{
				continue;
			}

			_finishedIdTimesListTheir.push_back({{incommingMessage.MainPacket.Id, incommingMessage.ClientAddres},std::chrono::steady_clock::now()});
			_finishedIdTimesMapTheir[{incommingMessage.MainPacket.Id, incommingMessage.ClientAddres}] = std::prev(_finishedIdTimesListTheir.end());
		}

		if (_processingFunction)
		{
			_processingFunction(incommingMessage);
		}
	}
}

void MessageManager::ConfirmReceiving(IncomingMessage& incomingMessage)
{
	SendPacket(incomingMessage.ClientAddres, Packet(incomingMessage.MainPacket.Id,incomingMessage.MainPacket.ClientId, incomingMessage.MainPacket.RequestOrigin, OPCODE::ACK));
}
