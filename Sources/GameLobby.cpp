#include "GameLobby.h"

#include "Logger.h"
#include "GameBool.h"

#include <arpa/inet.h>
#include <format>
#include <numeric>
#include <random>


GameLobby::GameLobby(std::string address, uint32_t port, uint32_t maxRooms, uint32_t maxPlayers) : _maxRooms(maxRooms), _maxPlayers(maxPlayers)
{
    PrepareHandler();

    _comunicator = std::make_unique<Comunicator>(address, port);
    _messageManager = std::make_unique<MessageManager>();
    _requestManager = std::make_unique<RequestManager>(5000);

    _messageManager->ConnectToComunicator(*_comunicator, 5000, 1);
    _requestManager->ConnectToMessageManager(*_messageManager);
    _requestManager->RegisterProcessingFunction([this] (std::shared_ptr<IncomingRequest> incomingRequest)
        {
            AddIncomingRequest(incomingRequest);
        });

    _running.store(true, std::memory_order_release);
    _mainWorker = std::thread(&GameLobby::MainLoop, this);
    CommandWorker = std::thread(&GameLobby::CommandLoop, this);
    _pingWorker = std::thread(&GameLobby::PingLoop, this);

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

    if (_pingWorker.joinable())
    {
        _pingWorker.join();
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

    Logger::LogMessage<GameLobby>("GameLobby byla ukoncena");
}

void GameLobby::PingLoop()
{
    while (_running.load(std::memory_order_acquire))
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        {
            std::lock_guard<std::mutex> lock(_clientsMutex);
            CheckClients();
        }
    }
}

void GameLobby::PingClients()
{

}

void GameLobby::CheckClients()
{
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

    for (auto& [id, client] : _clients)
    {
        auto elapsedTime = now - client.LastEcho;

        if (!client.Online && client.GameRoomId != 0 && elapsedTime > std::chrono::seconds(40))
        {
            uint32_t roomId = client.GameRoomId;
            uint32_t otherPlayerId = _gameRooms.at(roomId)->GetOtherPlayerId(client.Id);

            client.AssignRoom(0);
            _gameRooms.at(roomId)->RemovePlayer(client.Id);

            if (otherPlayerId == 0)
            {
                _gameRooms.erase(roomId);
                continue;
            }

            if (!_gameRooms.at(roomId)->IsInitialPosition())
            {
                _gameRooms.at(roomId)->SetGameEnd(_gameRooms.at(roomId)->GetPlayerColor(otherPlayerId) == PLAYER_COLOR::WHITE ? GAME_STATE::WHITE_WIN : GAME_STATE::BLACK_WIN);
            }

            if (!_clients.at(otherPlayerId).Online)
            {
                continue;
            }

            if (_gameRooms.at(roomId)->IsInitialPosition())
            {
                _requestManager->CreateDemand(
                    _clients.at(otherPlayerId),
                    OPCODE::OPPONENT_ARRIVED,
                    {""},
                    BIND_TIMEOUT,
                    [this] (std::unique_ptr<IncomingRequest> ir)
                    {
                        PlayerGotOpponent(std::move(ir));
                    }, OPCODE::KNOW_ABOUT_HIM
                );
                continue;
            }

            _requestManager->CreateDemand(
                _clients.at(otherPlayerId),
                OPCODE::GAME_FINISHED,
                {std::to_string(static_cast<uint32_t>(_gameRooms.at(roomId)->GetGameState()))},
                BIND_TIMEOUT,
                [this] (std::unique_ptr<IncomingRequest> ir)
                {
                    PlayerKnowResult(std::move(ir));
                }, OPCODE::FINALY_END
            );
            continue;
        }

        if (!client.Pinged && elapsedTime > std::chrono::seconds(3))
        {
            client.Pinged = true;

            _requestManager->CreateDemand(client, OPCODE::PING, {}, BIND_TIMEOUT, [this] (std::unique_ptr<IncomingRequest> ir)
                {
                    ClientPinged(std::move(ir));
                }, OPCODE::I_SEE_YOU
            );
        }
    }
}

void GameLobby::ProcessCommand(std::string& command)
{
    if (command == "clients")
    {
        std::lock_guard<std::mutex> lock(_clientsMutex);

        std::stringstream ss("List klientu:\n");

        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

        for (const auto& [id, client] : _clients)
        {
            char ipBuf[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client.Addr.sin_addr, ipBuf, INET_ADDRSTRLEN);

            ss << "Klient: " << id << "\t Jmeno: " << client.Name << "\t Adresa: " << std::string(ipBuf) << ":" << std::to_string(ntohs(client.Addr.sin_port)) << "\t Online: " << (client.Online ? "Ano" : "Ne") << "\t Posl. echo pred: " 
                << std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(now - client.LastEcho).count()) << " ms" << "\n";
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
        std::lock_guard<std::mutex> lock(_clientsMutex);

        std::stringstream ss("List roomek:\n");

        for (const auto& [id, room] : _gameRooms)
        {
            ss << "Room: " << id << "\t Bily: " << std::to_string(room->GetWhiteId()) << "\t Cerny: " << std::to_string(room->GetBlackId()) << "\n";
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
    _handlers[OPCODE::RECON] = [this] (std::shared_ptr<IncomingRequest>& incomingRequest) { ReconnectPlayer(incomingRequest); };
    _handlers[OPCODE::WHAT_HAPPEND] = [this] (std::shared_ptr<IncomingRequest>& incomingRequest) { SendLastMove(incomingRequest); };
    _handlers[OPCODE::IS_HE_ALIVE] = [this] (std::shared_ptr<IncomingRequest>& incomingRequest) { OtherStatus(incomingRequest); };
    _handlers[OPCODE::AM_I_PLAYING] = [this] (std::shared_ptr<IncomingRequest>& incomingRequest) { IsPlayerInGame(incomingRequest); };
    _handlers[OPCODE::GET_POSITION] = [this] (std::shared_ptr<IncomingRequest>& incomingRequest) { SendBoard(incomingRequest); };

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

            {
                std::lock_guard<std::mutex> lock(_clientsMutex);
                incommingRequest = std::move(_incomingRequests.front());
            }

            _incomingRequests.pop();
        }

        _handlers[incommingRequest->Opcode](incommingRequest);
    }
}

void GameLobby::OnTimeout(uint32_t id, uint32_t connectionId)
{
    {
        std::lock_guard<std::mutex> lock(_clientsMutex);

        auto demandedClient = _clients.find(id);

        if (demandedClient == _clients.end())
        {
            Logger::LogMessage<GameLobby>("Timeout neexistujiciho klienta");
            return;
        }

        if (!_clients.at(id).Online || _clients.at(id).ConnectionID != connectionId)
        {
            return;
        }

        _clients.at(id).Online = false;


        if (_clients.at(id).GameRoomId == 0)
        {
            return;
        }

        uint32_t othreId = _gameRooms.at(_clients.at(id).GameRoomId)->GetOtherPlayerId(id);

        if (othreId == 0)
        {
            return;
        }

        _requestManager->CreateDemand(_clients.at(othreId), OPCODE::HES_MISSING, {std::to_string(static_cast<uint32_t>(GAME_BOOL::TRUE))}, BIND_TIMEOUT, [this] (std::unique_ptr<IncomingRequest> ir)
            {
                PlayerKnowOthreMissing(std::move(ir));
            }, OPCODE::THATS_A_SHAME
        );
    }
}
