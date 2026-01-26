#pragma once

#include "Errors.h"

#include<string>
#include <netinet/in.h>
#include <thread>
#include <queue>
#include <condition_variable>
#include <functional>

class Comunicator
{
public:
	Comunicator(std::string address, int port);
	~Comunicator();

	void Stop();

	void Send(const sockaddr_in& targetAddr, const std::string& message);
	void RegisterReceiver(std::function<void(sockaddr_in&, std::string&)> recieverFunction);
private:
	const int _sock;
	const int BUFFSIZE = 255;

	std::atomic<bool> _running = false;

	std::atomic<bool> _stopping;
	std::mutex _stopMutex;
	std::condition_variable _cvStop;

	class Listener
	{
	public:
		Listener(Comunicator& comunicator);
		~Listener();

		void RegisterReceiver(std::function<void(sockaddr_in&, std::string&)> recieverFunction);
		void Stop();
	private:
		Comunicator& _comunicator;

		std::function<void(sockaddr_in&, std::string&)> _recieverFunction;

		std::thread _worker;
		std::atomic<bool> _running = false;

		void Start();
		void MainLoop();
	};

	class Sender
	{
	public:
		Sender(Comunicator& comunicator);
		~Sender();

		void Send(const sockaddr_in& targetAddr, const std::string& message);
		void Stop();
	private:
		Comunicator& _comunicator;

		std::mutex _sendQueueMutex;
		std::condition_variable _cvSendQueue;
		std::queue<std::pair<sockaddr_in, std::string>> _sendQueue;

		std::thread _worker;
		std::atomic<bool> _running = false;

		void Start();
		void MainLoop();
		void doSend(const sockaddr_in& targetAddr, const std::string& message);
	};

	std::unique_ptr<Listener> _listener;
	std::unique_ptr<Sender> _sender;
};
