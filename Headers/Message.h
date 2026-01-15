#pragma once

#include "Packet.h"

#include <netinet/in.h>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <optional>

/*enum class MESSAGE_TYPE
{
	INCOMING,
	OUTGOING
};

class Message
{
public:
	Message(const MESSAGE_TYPE MessageType, const Packet packet, const sockaddr_in& clientAddr, const int timeout);
	//~Message();
	void Finish();

	const MESSAGE_TYPE MessageType;
	const sockaddr_in ClientAddres;
	const int Timeout;
	const bool Finished = &_finished;
private:
	bool _finished;
};*/

class MessageManager;

class IncomingMessage
{
	public:
		IncomingMessage(const Packet& packet, const sockaddr_in& clientAddr);

		const Packet& MainPacket;
		const sockaddr_in ClientAddres;
};

class OutgoingMessage
{
public:
	OutgoingMessage(const Packet& mainPacket, const sockaddr_in& clientAddr, std::function<void()> onTimeout);
	~OutgoingMessage();

	void Stop();

	void OnMessageSent();
	void WaitForAck();
	void RegisterMessageManager(MessageManager& MessageManager, const uint32_t timeout, const uint32_t maxAttempts);


	void Finish();
	const bool& Finished = _finished;
	const Packet& MainPacket;
	const sockaddr_in ClientAddres;
private:
	std::atomic<bool> _running;

	uint32_t _timeout;
	const std::function<void()> _onTimeout;

	uint32_t _maxAttempts;
	uint32_t _attempts;

	std::optional<std::reference_wrapper<MessageManager>> _messageManager;

	std::atomic<bool> _finished;

	std::mutex _ackMutex;
	std::condition_variable _cvAck;

	std::thread _worker;
};