#pragma once

#include "Packet.h"
#include "Comunicator.h"
#include "Message.h"
#include "Utils.h"

#include <memory>
#include <unordered_map>
#include <queue>
#include <string>
#include <list>
#include <chrono>

class MessageManager
{
public:
	MessageManager();
	~MessageManager();

	void Stop();

	void ConnectToComunicator(Comunicator& comunicator, const uint32_t _messageTimeout, const uint32_t _maxMessageAttempts);
	void RegisterProcessingFunction(std::function<void(IncomingMessage&)> processingFunction);
	void SendMessage(std::shared_ptr<OutgoingMessage> outgoingMessage);
	void SendPacket(const sockaddr_in& targetAddr, const Packet& packet);

private:
	uint32_t _messageTimeout;
	uint32_t _maxMessageAttempts;

	std::atomic<bool> _running;

	std::atomic<bool> _stopping;
	std::mutex _stopMutex;
	std::condition_variable _cvStop;

	std::optional<std::reference_wrapper<Comunicator>> _comunicator;
	std::function<void(IncomingMessage&)> _processingFunction;

	std::thread _messageWorker;

	std::unordered_map<uint32_t, std::shared_ptr<OutgoingMessage>> _outgoingMessages;
	std::unordered_map<std::pair<uint32_t, sockaddr_in>, std::shared_ptr<IncomingMessage>, Utils::IntSockaddrKeyHash, Utils::IntSockaddrKeyEqual> _incomingMessages;

	std::mutex _incomingMutex;
	std::mutex _outgoingMutex;

	std::mutex _finishedMutex;
	std::list<std::pair<uint32_t,std::chrono::steady_clock::time_point>> _finishedIdTimesListOur;
	std::unordered_map<uint32_t, std::list<std::pair<uint32_t, std::chrono::steady_clock::time_point>>::iterator> _finishedIdTimesMapOur;

	std::list<std::pair<std::pair<uint32_t, sockaddr_in>, std::chrono::steady_clock::time_point>> _finishedIdTimesListTheir;
	std::unordered_map<std::pair<uint32_t, sockaddr_in>, std::list<std::pair<std::pair<uint32_t, sockaddr_in>, std::chrono::steady_clock::time_point>>::iterator,Utils::IntSockaddrKeyHash, Utils::IntSockaddrKeyEqual> _finishedIdTimesMapTheir;

	std::mutex _arrivingMutex;
	std::condition_variable _cvMessageArrived;
	std::queue<std::pair<sockaddr_in, std::string>> _arrivedMessages;
	std::thread _cleaningWorker;

	const int CLEANER_SLEEP_MS = 1000;

	void CleaningLoop();
	void CleanFinished();

	void OnStringRecieve(const sockaddr_in& targetAddr, std::string& message);
	void MainLoop();
	void ConfirmReceiving(IncomingMessage& incomingMessage);
};
