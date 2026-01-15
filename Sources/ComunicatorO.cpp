#include "Errors.h"

#include "Logger.h"
#include "Client.h"
#include "program.h"

#include<string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <thread>
#include <mutex>
#include <queue>
#include <arpa/inet.h>
#include <condition_variable>

class Comunicator
{
public:
	Comunicator(int port)
	{
		_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

		if (_sock < 0)
		{
			Logger::LogError<Comunicator>(ERROR_CODES::CANNOT_CREATE_SOCKET);
		}

		sockaddr_in comunicatorServer {};
		std::memset(&comunicatorServer, 0, sizeof(comunicatorServer));
		comunicatorServer.sin_family = AF_INET;
		comunicatorServer.sin_addr.s_addr = htonl(INADDR_ANY);
		comunicatorServer.sin_port = htons(port);

		if (bind(_sock, reinterpret_cast<sockaddr*>(&comunicatorServer), sizeof(comunicatorServer)) < 0)
		{
			Logger::LogError<Comunicator>(ERROR_CODES::CANNOT_BIND_SOCKET);
		}

		Logger::LogMessage<Comunicator>("Komunikátor úspěšně inicializován na portu " + std::to_string(ntohs(comunicatorServer.sin_port)));

		std::thread senderThread(&Comunicator::Sender, this);
		std::thread receiverThread(&Comunicator::Listener, this);

		senderThread.join();
		receiverThread.join();
	}

	~Comunicator()
	{
		close(_sock);
	}

	void Send(const sockaddr_in& targetAddr, const std::string& message)
	{
		{
			std::lock_guard<std::mutex> lock(_sendQueueMutex);
			_sendQueue.push({targetAddr, message});
		}
		_cvSendQueue.notify_one();
	}


private:
	int _sock;
	const int BUFFSIZE = 255;



	std::mutex _sendQueueMutex;
	std::condition_variable _cvSendQueue;
	std::queue<std::pair<sockaddr_in, std::string>> _sendQueue;

	std::atomic<bool> _stopSending = false;

	void Sender()
	{
		while (true)
		{
			std::pair<sockaddr_in, std::string> item;
			{
				std::unique_lock<std::mutex> lock(_sendQueueMutex);
				_cvSendQueue.wait(lock, [this]
					{
						return !_sendQueue.empty() || _stopSending;
					});

				if (_stopSending && _sendQueue.empty()) return;

				item = std::move(_sendQueue.front());
				_sendQueue.pop();
			}

			doSend(item.first, item.second);
		}
	}

	void doSend(const sockaddr_in& targetAddr, const std::string& message)
	{
		Logger::LogMessage<Comunicator>("Odesílání zprávy: \"" + message + "\" na adresu: " + inet_ntoa(targetAddr.sin_addr) + ":" + std::to_string(ntohs(targetAddr.sin_port)));

		ssize_t sent = sendto(_sock, message.c_str(), message.length(), 0,
			reinterpret_cast<const sockaddr*>(&targetAddr),
			sizeof(targetAddr));

		if (sent < 0)
		{
			Logger::LogError<Comunicator>(ERROR_CODES::FATAL_SOCKET);
		}

		if (static_cast<size_t>(sent) != message.length())
		{
			Logger::LogError<Comunicator>(ERROR_CODES::CANNOT_SEND_VIA_SOCKET);
		}
	}


	std::atomic<bool> _stopListening = false;

	void Listener()
	{
		sockaddr_in sourceClient { };
		unsigned int clientlen;
		char buffer[BUFFSIZE];

		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 100000; // 100 ms
		setsockopt(_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

		while (!_stopListening)
		{
			clientlen = sizeof(sourceClient);

			ssize_t received = recvfrom(_sock, buffer, BUFFSIZE, 0,
				reinterpret_cast<sockaddr*>(&sourceClient), &clientlen);

			if (received > 0)
			{
				std::string message(buffer, received);

				Logger::LogMessage<Comunicator>("Přijmuta zpráva: \"" + message + "\" z adresy: " + inet_ntoa(sourceClient.sin_addr) + ":" + std::to_string(ntohs(sourceClient.sin_port)));

			}
			else if (received < 0)
			{
				switch (errno)
				{
					case EBADF:
					case ENOTSOCK:
					case EINVAL:
					case EFAULT:
						Logger::LogError<Comunicator>(ERROR_CODES::FATAL_SOCKET);
						ForcedExit();
					case EINTR:
					case ECONNREFUSED:
						Logger::LogError<Comunicator>(ERROR_CODES::BAD_SOCKET, LOG_LEVEL::WARNING);
					case EAGAIN:
						continue;
					default:
						Logger::LogError<Comunicator>(ERROR_CODES::UNKNOWN_SOCKET_RETURN);
						break;
				}

				Logger::LogError<Comunicator>(ERROR_CODES::BAD_SOCKET);
			}
		}
	}
};


