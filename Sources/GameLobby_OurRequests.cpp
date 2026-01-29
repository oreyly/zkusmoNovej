#include <GameLobby.h>

void GameLobby::ClientPinged(std::unique_ptr<IncomingRequest> incomingRequest)
{
    {
        std::lock_guard<std::mutex> lock(_clientsMutex);

        _clients.at(incomingRequest->ClientId).LastEcho = std::chrono::steady_clock::now();
        _clients.at(incomingRequest->ClientId).Pinged = false;
    }
}

void GameLobby::PlayerGotOpponent(std::unique_ptr<IncomingRequest> incomingRequest)
{
    {
        std::lock_guard<std::mutex> lock(_clientsMutex);

        _clients.at(incomingRequest->ClientId).LastEcho = std::chrono::steady_clock::now();
    }
}

void GameLobby::PlayerGotMove(std::unique_ptr<IncomingRequest> incomingRequest)
{
    {
        std::lock_guard<std::mutex> lock(_clientsMutex);

        _clients.at(incomingRequest->ClientId).LastEcho = std::chrono::steady_clock::now();
    }
}

void GameLobby::PlayerKnowResult(std::unique_ptr<IncomingRequest> incomingRequest)
{
    {
        std::lock_guard<std::mutex> lock(_clientsMutex);

        _clients.at(incomingRequest->ClientId).LastEcho = std::chrono::steady_clock::now();

        uint32_t roomId = _gameRooms.at(_clients.at(incomingRequest->ClientId).GameRoomId)->Id;

        _gameRooms.at(roomId)->RemovePlayer(incomingRequest->ClientId);
        _clients.at(incomingRequest->ClientId).AssignRoom(0);

        if (_gameRooms.at(roomId)->GetWhiteId() == _gameRooms.at(roomId)->GetBlackId())
        {
            _gameRooms.erase(roomId);
        }
    }
}

void GameLobby::PlayerKnowOthreMissing(std::unique_ptr<IncomingRequest> incomingRequest)
{
    {
        std::lock_guard<std::mutex> lock(_clientsMutex);

        _clients.at(incomingRequest->ClientId).LastEcho = std::chrono::steady_clock::now();
    }
}