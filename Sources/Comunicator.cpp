#include "Comunicator.h"
#include "Logger.h"
#include "Client.h"
#include "program.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

// --- Comunicator ---

Comunicator::Comunicator(std::string address, int port) : _sock(socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP))
{
    if (_sock < 0)
    {
        Logger::LogError<Comunicator>(ERROR_CODES::CANNOT_CREATE_SOCKET);
    }

    sockaddr_in comunicatorServer {};
    std::memset(&comunicatorServer, 0, sizeof(comunicatorServer));
    comunicatorServer.sin_family = AF_INET;
    comunicatorServer.sin_addr.s_addr = inet_addr(address.c_str());
    comunicatorServer.sin_port = htons(port);

    if (bind(_sock, reinterpret_cast<sockaddr*>(&comunicatorServer), sizeof(comunicatorServer)) < 0)
    {
        Logger::LogError<Comunicator>(ERROR_CODES::CANNOT_BIND_SOCKET);
    }

    socklen_t sockLen = sizeof(comunicatorServer);

    if (getsockname(_sock, reinterpret_cast<sockaddr*>(&comunicatorServer), &sockLen) == -1)
    {
        Logger::LogError<Comunicator>(ERROR_CODES::SOCEK_IFNO_UNAVAILABLE);
    }

    Logger::LogMessage<Comunicator>("Komunikator uspesne inicializovan na portu " + std::to_string(ntohs(comunicatorServer.sin_port)));

    _running.store(true, std::memory_order_release);

    _listener = std::make_unique<Listener>(*this);
    _sender = std::make_unique<Sender>(*this);
}

Comunicator::~Comunicator()
{
    Stop();

    std::unique_lock<std::mutex> lock(_stopMutex);
    _cvStop.wait(lock, [this] ()
        {
            return !_stopping.load(std::memory_order_acquire);
        });
}

void Comunicator::Stop()
{
    if (!_running.load(std::memory_order_acquire))
    {
        return;
    }

    _stopping.store(true, std::memory_order_release);
    _running.store(false, std::memory_order_release);

    _sender->Stop();
    _listener->Stop();

    close(_sock);

    _stopping.store(false, std::memory_order_release);
    _cvStop.notify_one();
}

void Comunicator::Send(const sockaddr_in& targetAddr, const std::string& message)
{
    _sender->Send(targetAddr, message);
}

void Comunicator::RegisterReceiver(std::function<void(sockaddr_in&,std::string&)> recieverFunction)
{
    _listener->RegisterReceiver(recieverFunction);
}

// --- Comunicator::Listener ---

Comunicator::Listener::Listener(Comunicator& comunicator) : _comunicator(comunicator)
{
    Start();
}

Comunicator::Listener::~Listener()
{
    Stop();
}

void Comunicator::Listener::Start()
{
    _running.store(true, std::memory_order_release);
    _worker = std::thread(&Listener::MainLoop, this);
}

void Comunicator::Listener::RegisterReceiver(std::function<void(sockaddr_in&,std::string&)> recieverFunction)
{
    if (_recieverFunction)
    {
        Logger::LogError<Listener>(ERROR_CODES::ALREAD_REGISTERED);
    }

    _recieverFunction = recieverFunction;
}

void Comunicator::Listener::Stop()
{
    if (!_running.load(std::memory_order_acquire))
    {
        return;
    }

    _running.store(false, std::memory_order_release);
    if (_worker.joinable())
    {
        _worker.join();
    }
}

void Comunicator::Listener::MainLoop()
{
    sockaddr_in sourceClient { };
    unsigned int clientlen;
    char buffer[_comunicator.BUFFSIZE];

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000; // 100 ms timeout
    setsockopt(_comunicator._sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while (_running.load(std::memory_order_acquire))
    {
        clientlen = sizeof(sourceClient);
        ssize_t received = recvfrom(_comunicator._sock, buffer, _comunicator.BUFFSIZE, 0,
            reinterpret_cast<sockaddr*>(&sourceClient), &clientlen);

        if (received > 0)
        {
            std::string message(buffer, received);

            Logger::LogMessage<Comunicator>("Prijmuta zprava: \"" + message + "\" z adresy: " +
                inet_ntoa(sourceClient.sin_addr) + ":" +
                std::to_string(ntohs(sourceClient.sin_port)));

            if (_recieverFunction)
            {
                _recieverFunction(sourceClient, message);
            }

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
                    if (_running.load(std::memory_order_acquire))
                    {
                        ForcedExit();
                    }
                    return;
                case EINTR:
                case ECONNREFUSED:
                    Logger::LogError<Comunicator>(ERROR_CODES::BAD_SOCKET, LOG_LEVEL::WARNING);
                case EAGAIN:
                    continue;
                default:
                    Logger::LogError<Comunicator>(ERROR_CODES::UNKNOWN_SOCKET_RETURN);
                    if (_running.load(std::memory_order_acquire))
                    {
                        ForcedExit();
                    }
                    return;
            }
        }
    }
}

// --- Comunicator::Sender ---

Comunicator::Sender::Sender(Comunicator& comunicator) : _comunicator(comunicator)
{
    Start();
}

Comunicator::Sender::~Sender()
{
    Stop();
    if (_worker.joinable())
    {
        _worker.join();
    }
}

void Comunicator::Sender::Start()
{
    _running.store(true, std::memory_order_release);
    _worker = std::thread(&Sender::MainLoop, this);
}

void Comunicator::Sender::Stop()
{
    if (!_running)
    {
        return;
    }

    _running.store(false, std::memory_order_release);
    _cvSendQueue.notify_one();
    if (_worker.joinable())
    {
        _worker.join();
    }
}

void Comunicator::Sender::Send(const sockaddr_in& targetAddr, const std::string& message)
{
    if (!_running.load(std::memory_order_acquire))
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(_sendQueueMutex);
        _sendQueue.push({targetAddr, message});
    }
    _cvSendQueue.notify_one();
}


void Comunicator::Sender::MainLoop()
{
    while (_running.load(std::memory_order_acquire))
    {
        std::pair<sockaddr_in, std::string> item;
        {
            std::unique_lock<std::mutex> lock(_sendQueueMutex);
            _cvSendQueue.wait(lock, [this]
                {
                    return !_sendQueue.empty() || !_running.load(std::memory_order_acquire);
                });

            if (!_running.load(std::memory_order_acquire) && _sendQueue.empty()) return;

            item = std::move(_sendQueue.front());
            _sendQueue.pop();
        }

        doSend(item.first, item.second);
    }
}

void Comunicator::Sender::doSend(const sockaddr_in& targetAddr, const std::string& message)
{
    Logger::LogMessage<Sender>("Odesilani zpravy: \"" + message + "\" na adresu: " + inet_ntoa(targetAddr.sin_addr) + ":" + std::to_string(ntohs(targetAddr.sin_port)));

    ssize_t sent = sendto(_comunicator._sock, message.c_str(), message.length(), 0,
        reinterpret_cast<const sockaddr*>(&targetAddr),
        sizeof(targetAddr));

    if (sent < 0)
    {
        Logger::LogError<Sender>(ERROR_CODES::FATAL_SOCKET);
        ForcedExit();
    }

    if (static_cast<size_t>(sent) != message.length())
    {
        Logger::LogError<Sender>(ERROR_CODES::CANNOT_SEND_VIA_SOCKET);
        ForcedExit();
    }
}
