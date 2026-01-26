#pragma once

#include "Comunicator.h"
#include "MessageManager.h"
#include "RequestManager.h"
#include "GameRoom.h"
#include "Chessboard.h"

#include <vector>
#include <memory>


class GameLobby
{
public:
    GameLobby(std::string address, uint32_t port);
    ~GameLobby();

    void Stop();

private:
    std::atomic<bool> _stopping;
    std::mutex _stopMutex;
    std::condition_variable _cvStop;

    std::atomic<bool> _running;
    std::thread _mainWorker;

    std::thread _commandWorker;

    std::queue<std::shared_ptr<IncomingRequest>> _incomingRequests; 
    std::unordered_map<OPCODE, std::function<void(std::shared_ptr<IncomingRequest>&)>> _handlers;

    std::unordered_map<uint32_t, std::unique_ptr<GameRoom>> _gameRooms;

    std::condition_variable _cvIncomming;
    std::mutex _incomingMutex;

    std::unique_ptr<Comunicator> _comunicator;
    std::unique_ptr<MessageManager> _messageManager;
    std::unique_ptr<RequestManager> _requestManager;

    std::unordered_map<uint32_t, Client> _clients;
    std::mutex _clientsMutex;

    void CommandLoop();

    void ProcessCommand(std::string& command);

    void PrepareHandler();
    void AddIncomingRequest(std::shared_ptr<IncomingRequest> incomingRequest);
    void MainLoop();
    void OnTimeout();

    void RegisterClient(std::shared_ptr<IncomingRequest>& incomingRequest);
    void AssignName(std::shared_ptr<IncomingRequest>& incomingRequest);
    void ListRooms(std::shared_ptr<IncomingRequest>& incomingRequest);
    void CreateRoom(std::shared_ptr<IncomingRequest>& incomingRequest);
    void JoinRoom(std::shared_ptr<IncomingRequest>& incomingRequest);
    void PlayedMove(std::shared_ptr<IncomingRequest>& incomingRequest);
    void PlayerLeave(std::shared_ptr<IncomingRequest>& incomingRequest);
    void PlayerQuit(std::shared_ptr<IncomingRequest>& incomingRequest);

    void PlayerGotOpponent(std::unique_ptr<IncomingRequest> incomingRequest);
    void PlayerGotMove(std::unique_ptr<IncomingRequest> incomingRequest);
    void PlayerKnowResult(std::unique_ptr<IncomingRequest> incomingRequest);
};
