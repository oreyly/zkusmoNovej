#include "Message.h"

#include "Logger.h"
#include "MessageManager.h"

IncomingMessage::IncomingMessage(const Packet& packet, const sockaddr_in& clientAddr) : MainPacket(packet), ClientAddres(clientAddr)
{

}

OutgoingMessage::OutgoingMessage(const Packet& mainPacket, const sockaddr_in& clientAddr, const std::function<void()> onTimeout) : MainPacket(mainPacket), ClientAddres(clientAddr), _attempts(0), _onTimeout(onTimeout)
{ }

OutgoingMessage::~OutgoingMessage()
{
    Stop();
}

void OutgoingMessage::Stop()
{
    if (!_running.load(std::memory_order_acquire))
    {
        return;
    }

    _running.store(false, std::memory_order_release);
    _cvAck.notify_one();

    if (_worker.joinable())
    {
        _worker.join();
    }
}

void OutgoingMessage::RegisterMessageManager(MessageManager& messageManager, const uint32_t timeout, const uint32_t maxAttempts)
{
    if (_messageManager)
    {
        Logger::LogError<OutgoingMessage>(ERROR_CODES::ALREAD_REGISTERED);
    }

    _timeout = timeout;
    _maxAttempts = maxAttempts;
    _messageManager = messageManager;
}

void OutgoingMessage::OnMessageSent()
{
    _attempts += 1;

    _finished.store(false, std::memory_order_release);
    _running.store(true, std::memory_order_release);
    _worker = std::thread(&OutgoingMessage::WaitForAck, this);
}

void OutgoingMessage::Finish()
{
    _finished.store(true, std::memory_order_release);

    {
        std::lock_guard<std::mutex> lock(_ackMutex);
    }

    _cvAck.notify_one();
}

void OutgoingMessage::WaitForAck()
{
    if (_finished.load(std::memory_order_acquire))
    {
        return;
    }

    std::unique_lock<std::mutex> lock(_ackMutex);

    while (!_finished.load(std::memory_order_acquire) && _attempts < _maxAttempts)
    {
        _cvAck.wait_for(lock, std::chrono::milliseconds(_timeout), [this] ()
            {
                return _finished.load(std::memory_order_acquire) || !_running.load(std::memory_order_acquire);
            });

        if (_finished.load(std::memory_order_acquire) || !_running.load(std::memory_order_acquire))
        {
            return;
        }

        Logger::LogError<OutgoingMessage>(ERROR_CODES::MISSING_ACKNOWLEDGEMENT, LOG_LEVEL::WARNING);

        ++_attempts;
        _messageManager->get().SendPacket(ClientAddres, MainPacket);
    }

    _onTimeout();
}