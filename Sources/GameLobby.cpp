#include "GameLobby.h"

#include "Logger.h"
#include "GameBool.h"

#include <arpa/inet.h>
#include <format>
#include <numeric>
#include <random>

#ifndef BIND_TIMEOUT
#define BIND_TIMEOUT [this]() { OnTimeout(); }
#endif


GameLobby::GameLobby(std::string address, uint32_t port)
{
    PrepareHandler();

    _comunicator = std::make_unique<Comunicator>(address, port);
    _messageManager = std::make_unique<MessageManager>();
    _requestManager = std::make_unique<RequestManager>();

    _messageManager->ConnectToComunicator(*_comunicator, 2000, 3);
    _requestManager->ConnectToMessageManager(*_messageManager);
    _requestManager->RegisterProcessingFunction([this] (std::shared_ptr<IncomingRequest> incomingRequest)
        {
            AddIncomingRequest(incomingRequest);
        });

    _running.store(true, std::memory_order_release);
    _mainWorker = std::thread(&GameLobby::MainLoop, this);
    _commandWorker = std::thread(&GameLobby::CommandLoop, this);

    std::random_device rd;
    std::mt19937 gen(rd()); 
    std::uniform_int_distribution<> distr(0, 1);

    /*for (int i = 0; i < 20; ++i)
    {
        // 1. Vytvoøení instance (ID se generuje uvnitø konstruktoru)
        std::unique_ptr<GameRoom> room = std::make_unique<GameRoom>();

        room->SetWhiteId(distr(gen));
        room->SetBlackId(distr(gen));

        _gameRooms.emplace(room->Id, std::move(room));
    }*/

}

GameLobby::~GameLobby()
{
    return;
    Stop();

    std::unique_lock<std::mutex> lock(_stopMutex);
    _cvStop.wait(lock, [this] ()
        {
            return !_stopping.load(std::memory_order_acquire);
        });
}


void GameLobby::Stop()
{
    if (!_running.load(std::memory_order_acquire) || _stopping.load(std::memory_order_acquire))
    {
        return;
    }

    _stopping.store(true, std::memory_order_release);
    _running.store(false, std::memory_order_release);
    
    std::thread t1(&Comunicator::Stop, _comunicator.get());
    std::thread t2(&MessageManager::Stop, _messageManager.get());
    std::thread t3(&RequestManager::Stop, _requestManager.get());

    _cvIncomming.notify_one();

    if (_mainWorker.joinable())
    {
        _mainWorker.join();
    }

    if (t1.joinable())
    {
        t1.join();
    }

    if (t2.joinable())
    {
        t2.join();
    }

    if (t3.joinable())
    {
        t3.join();
    }

    _stopping.store(false, std::memory_order_release);
    _cvStop.notify_one();
}

void GameLobby::CommandLoop()
{
    std::string vstup;

    Logger::LogMessage<GameLobby>("GameLobby byla spustena.");

    while (_running.load(std::memory_order_acquire))
    {
        std::cin >> vstup;

        ProcessCommand(vstup);
    }
}

void GameLobby::ProcessCommand(std::string& command)
{
    if (command == "clients")
    {
        std::stringstream ss("List klientu:\n");

        for (const auto& [id, client] : _clients)
        {
            char ipBuf[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client.Addr.sin_addr, ipBuf, INET_ADDRSTRLEN);
            ss << "Klient: " << id << "\t Jmeno: " << client.Name << "\t Adresa: " << std::string(ipBuf) << ":" << std::to_string(ntohs(client.Addr.sin_port)) << "\n";
        }

        Logger::LogMessage<GameLobby>(ss.str());
    }
    else if (command == "konec")
    {
        Logger::LogMessage<GameLobby>("Ukoncuji program");
        Stop();
    }
    else if (command == "rooms")
    {
        std::stringstream ss("List roomek:\n");

        for (const auto& [id, room] : _gameRooms)
        {
            ss << "Room: " << id << "\t Bily: " << std::to_string(room->GetWhiteId()) << "\t Cerny: " << std::to_string(room->GetWhiteId()) << "\n";
        }

        Logger::LogMessage<GameLobby>(ss.str());
    }
    else
    {
        Logger::LogError<GameLobby>(ERROR_CODES::INVALID_COMMAND);
    }
}

void GameLobby::PrepareHandler()
{
    _handlers[OPCODE::CON] = [this] (std::shared_ptr<IncomingRequest>& incomingRequest) { RegisterClient(incomingRequest); };
    _handlers[OPCODE::WANA_NAME] = [this] (std::shared_ptr<IncomingRequest>& incomingRequest) { AssignName(incomingRequest); };
    _handlers[OPCODE::WANA_ROOMS] = [this] (std::shared_ptr<IncomingRequest>& incomingRequest) { ListRooms(incomingRequest); };
    _handlers[OPCODE::CREATE_ROOM] = [this] (std::shared_ptr<IncomingRequest>& incomingRequest) { CreateRoom(incomingRequest); };
    _handlers[OPCODE::GET_TO_ROOM] = [this] (std::shared_ptr<IncomingRequest>& incomingRequest) { JoinRoom(incomingRequest); };
    _handlers[OPCODE::I_MOVED] = [this] (std::shared_ptr<IncomingRequest>& incomingRequest) { PlayedMove(incomingRequest); };
    _handlers[OPCODE::AM_LEAVING] = [this] (std::shared_ptr<IncomingRequest>& incomingRequest) { PlayerLeave(incomingRequest); };
    _handlers[OPCODE::GAME_QUIT] = [this] (std::shared_ptr<IncomingRequest>& incomingRequest) { PlayerQuit(incomingRequest); };
}

void GameLobby::AddIncomingRequest(std::shared_ptr<IncomingRequest> incomingRequest)
{
    {
        std::lock_guard<std::mutex> lock(_incomingMutex);

        _incomingRequests.push(std::move(incomingRequest));
    }

    _cvIncomming.notify_one();
}

void GameLobby::MainLoop()
{
    while (_running.load(std::memory_order_acquire))
    {
        std::shared_ptr<IncomingRequest> incommingRequest;

        {
            std::unique_lock<std::mutex> lock(_incomingMutex);

            _cvIncomming.wait(lock, [this] ()
                {
                    return !_incomingRequests.empty() || !_running.load(std::memory_order_acquire);
                });

            if (!_running.load(std::memory_order_acquire))
            {
                return;
            }

            incommingRequest = std::move(_incomingRequests.front());
            _incomingRequests.pop();
        }

        _handlers[incommingRequest->Opcode](incommingRequest);
    }
}

void GameLobby::OnTimeout()
{
    Logger::LogMessage<GameLobby>("Smolik");
}

void GameLobby::RegisterClient(std::shared_ptr<IncomingRequest>& incomingRequest)
{
    Client newClient(incomingRequest->SourceAddress);

    _requestManager->SendResponse(newClient, OPCODE::CON_AS, {std::to_string(newClient.Id)}, BIND_TIMEOUT);

    _clients.try_emplace(newClient.Id, newClient);
}

void GameLobby::AssignName(std::shared_ptr<IncomingRequest>& incomingRequest)
{
    for (const auto& [id, client] : _clients)
    {
        if (client.Name == incomingRequest->Parameters[0])
        {
            _requestManager->SendResponse(_clients.at(incomingRequest->ClientId), OPCODE::VALID_NAME, {std::to_string(static_cast<uint32_t>(GAME_BOOL::FALSE))}, BIND_TIMEOUT);
            return;
        }
    }

    _clients.at(incomingRequest->ClientId).Name = incomingRequest->Parameters[0];
    _requestManager->SendResponse(_clients.at(incomingRequest->ClientId), OPCODE::VALID_NAME, {std::to_string(static_cast<uint32_t>(GAME_BOOL::TRUE))}, BIND_TIMEOUT);
}

void GameLobby::ListRooms(std::shared_ptr<IncomingRequest>& incomingRequest)
{
    std::vector<std::string> roomList;
    roomList.reserve(_gameRooms.size() * 3); // Optimalizace alokace

    for (const auto& [_, gr] : _gameRooms)
    {
        roomList.push_back(std::to_string(gr->Id));
        roomList.push_back(std::to_string(static_cast<uint32_t>(gr->GetWhiteId() > 0 ? GAME_BOOL::TRUE : GAME_BOOL::FALSE)));
        roomList.push_back(std::to_string(static_cast<uint32_t>(gr->GetBlackId() > 0 ? GAME_BOOL::TRUE : GAME_BOOL::FALSE)));
    }

    _requestManager->SendResponse(_clients.at(incomingRequest->ClientId), OPCODE::ROOM_LIST, roomList, BIND_TIMEOUT);
}

void GameLobby::CreateRoom(std::shared_ptr<IncomingRequest>& incomingRequest)
{
    std::unique_ptr<GameRoom> room = std::make_unique<GameRoom>();

    switch (PLAYER_COLOR(Utils::Str2Uint(incomingRequest->Parameters[0])))
    {
        case PLAYER_COLOR::BLACK:
            room->SetBlackId(incomingRequest.get()->ClientId);
            break;
        case PLAYER_COLOR::WHITE:
            room->SetWhiteId(incomingRequest.get()->ClientId);
            break;
        default:
            break;
    }

    _clients.at(incomingRequest.get()->ClientId).AssignRoom(room->Id);
    _gameRooms.emplace(room->Id, std::move(room));

    _requestManager->SendResponse(_clients.at(incomingRequest.get()->ClientId), OPCODE::ROOM_CREATED, {std::to_string(static_cast<uint32_t>(GAME_BOOL::TRUE))}, BIND_TIMEOUT);
}

void GameLobby::JoinRoom(std::shared_ptr<IncomingRequest>& incomingRequest)
{
    uint32_t roomId = Utils::Str2Uint(incomingRequest->Parameters[0]);
    PLAYER_COLOR playerColor = static_cast<PLAYER_COLOR>(Utils::Str2Uint(incomingRequest->Parameters[1]));

    auto demandedRoom = _gameRooms.find(roomId);

    if (demandedRoom == _gameRooms.end() || demandedRoom->second.get()->GetPlayerId(playerColor) > 0)
    {
        _requestManager->SendResponse(_clients.at(incomingRequest.get()->ClientId), OPCODE::GOT_TO_ROOM, {std::to_string(static_cast<uint32_t>(GAME_BOOL::FALSE))}, BIND_TIMEOUT);
        return;
    }

    demandedRoom->second.get()->SetPlayerId(playerColor, incomingRequest.get()->ClientId);
    _clients.at(incomingRequest.get()->ClientId).AssignRoom(roomId);

    std::string oponentName = _clients.at(demandedRoom->second.get()->GetPlayerId(!playerColor)).Name;

    _requestManager->SendResponse(_clients.at(incomingRequest.get()->ClientId), OPCODE::GOT_TO_ROOM, {std::to_string(static_cast<uint32_t>(GAME_BOOL::TRUE)), oponentName}, BIND_TIMEOUT);
    _requestManager->CreateDemand(_clients.at(demandedRoom->second.get()->GetPlayerId(!playerColor)), OPCODE::OPPONENT_ARRIVED, {_clients.at(incomingRequest->ClientId).Name}, 5000, BIND_TIMEOUT, [this] (std::unique_ptr<IncomingRequest> ir)
        {
            PlayerGotOpponent(std::move(ir));
        },OPCODE::KNOW_ABOUT_HIM);
}


void GameLobby::PlayedMove(std::shared_ptr<IncomingRequest>& incomingRequest)
{
    uint32_t roomId = _clients.at(incomingRequest->ClientId).GameRoomId;

    auto demandedRoom = _gameRooms.find(roomId);

    if (demandedRoom == _gameRooms.end())
    {
        _requestManager->SendResponse(_clients.at(incomingRequest->ClientId), OPCODE::YOU_MOVED, {std::to_string(static_cast<uint32_t>(GAME_BOOL::FALSE))}, BIND_TIMEOUT);
        return;
    }

    if (!demandedRoom->second->PlayMove(Utils::Str2Uint(incomingRequest->Parameters[0]), Utils::Str2Uint(incomingRequest->Parameters[1]), Utils::Str2Uint(incomingRequest->Parameters[2]), Utils::Str2Uint(incomingRequest->Parameters[3])))
    {
        _requestManager->SendResponse(_clients.at(incomingRequest.get()->ClientId), OPCODE::YOU_MOVED, {std::to_string(static_cast<uint32_t>(GAME_BOOL::FALSE))}, BIND_TIMEOUT);
        return;
    }

    _requestManager->SendResponse(_clients.at(incomingRequest->ClientId), OPCODE::YOU_MOVED, {std::to_string(static_cast<uint32_t>(GAME_BOOL::TRUE))}, BIND_TIMEOUT);

    _requestManager->CreateDemand(_clients.at(demandedRoom->second->GetOtherPlayerId(incomingRequest->ClientId)), OPCODE::HE_MOVED, incomingRequest->Parameters, 5000, BIND_TIMEOUT, [this] (std::unique_ptr<IncomingRequest> ir)
        {
            PlayerGotMove(std::move(ir));
        }, OPCODE::I_KNOW_HE_MOVED);


    GAME_STATE gameState = demandedRoom->second->GetGameState();
    if (gameState == GAME_STATE::IN_PROGRESS)
    {
        return;
    }

    _requestManager->CreateDemand(_clients.at(demandedRoom->second->GetWhiteId()), OPCODE::GAME_FINISHED, {std::to_string(static_cast<uint32_t>(gameState))}, 5000, BIND_TIMEOUT, [this] (std::unique_ptr<IncomingRequest> ir)
        {
            PlayerKnowResult(std::move(ir));
        }, OPCODE::FINALY_END);

    _requestManager->CreateDemand(_clients.at(demandedRoom->second->GetBlackId()), OPCODE::GAME_FINISHED, {std::to_string(static_cast<uint32_t>(gameState))}, 5000, BIND_TIMEOUT, [this] (std::unique_ptr<IncomingRequest> ir)
        {
            PlayerKnowResult(std::move(ir));
        }, OPCODE::FINALY_END);

    _clients.at(demandedRoom->second->GetWhiteId()).AssignRoom(0);
    _clients.at(demandedRoom->second->GetBlackId()).AssignRoom(0);

    _gameRooms.erase(demandedRoom->second->Id);
}


void GameLobby::PlayerLeave(std::shared_ptr<IncomingRequest>& incomingRequest)
{
    Logger::LogMessage<GameLobby>("Lybuju");
    _requestManager->SendResponse(_clients.at(incomingRequest->ClientId), OPCODE::YOU_LEFT, {}, BIND_TIMEOUT);
    _clients.erase(incomingRequest->ClientId);
}

void GameLobby::PlayerQuit(std::shared_ptr<IncomingRequest>& incomingRequest)
{
    Logger::LogMessage<GameLobby>("Quitting");
    _requestManager->SendResponse(_clients.at(incomingRequest->ClientId), OPCODE::GAME_QUITED, {}, BIND_TIMEOUT);

    auto demandedRoom = _gameRooms.find(_clients.at(incomingRequest->ClientId).GameRoomId);

    if (demandedRoom == _gameRooms.end())
    {
        Logger::LogError<GameLobby>(ERROR_CODES::UNKNOWN_ROOM);
        return;
    }

    uint32_t roomId = demandedRoom->first;

    int otherPlayerId = _gameRooms.at(roomId)->GetOtherPlayerId(incomingRequest->ClientId);

    _clients.at(incomingRequest->ClientId).AssignRoom(0);
    _gameRooms.at(roomId)->RemovePlayer(incomingRequest->ClientId);

    if (otherPlayerId == 0)
    {
        _gameRooms.erase(roomId);
        return;
    }

    if (_gameRooms.at(roomId)->IsInitialPosition())
    {
        _requestManager->CreateDemand(
            _clients.at(otherPlayerId),
            OPCODE::OPPONENT_ARRIVED,
            {""},
            5000,
            BIND_TIMEOUT,
            [this] (std::unique_ptr<IncomingRequest> ir)
            {
                PlayerGotOpponent(std::move(ir));
            }, OPCODE::KNOW_ABOUT_HIM
        );
        return;
    }

    _requestManager->CreateDemand(
        _clients.at(otherPlayerId),
        OPCODE::GAME_FINISHED,
        {std::to_string(static_cast<uint32_t>(_gameRooms.at(roomId)->GetPlayerColor(incomingRequest->ClientId) == PLAYER_COLOR::WHITE ? GAME_STATE::BLACK_WIN : GAME_STATE::WHITE_WIN))},
        5000,
        BIND_TIMEOUT,
        [this] (std::unique_ptr<IncomingRequest> ir)
        {
            PlayerKnowResult(std::move(ir));
        }, OPCODE::FINALY_END
    );

    _clients.at(otherPlayerId).AssignRoom(0);
    _gameRooms.erase(_gameRooms.at(roomId)->Id);
}

void GameLobby::PlayerGotOpponent(std::unique_ptr<IncomingRequest> incomingRequest)
{

}

void GameLobby::PlayerGotMove(std::unique_ptr<IncomingRequest> incomingRequest)
{

}

void GameLobby::PlayerKnowResult(std::unique_ptr<IncomingRequest> incomingRequest)
{

}
