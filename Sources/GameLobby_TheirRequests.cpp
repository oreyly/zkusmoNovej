#include <GameLobby.h>

#include <Logger.h>

#include "magic_enum.hpp"



void GameLobby::RegisterClient(std::shared_ptr<IncomingRequest>& incomingRequest)
{
    Client newClient(incomingRequest->SourceAddress);

    if (_clients.size() >= _maxPlayers)
    {
        _requestManager->SendResponse(newClient, OPCODE::CON_AS, {std::to_string(0)}, BIND_TIMEOUT);
        return;
    }

    _requestManager->SendResponse(newClient, OPCODE::CON_AS, {std::to_string(newClient.Id)}, BIND_TIMEOUT);

    _clients.try_emplace(newClient.Id, std::move(newClient));
}

void GameLobby::AssignName(std::shared_ptr<IncomingRequest>& incomingRequest)
{
    if (incomingRequest->Parameters.size() != 1 || incomingRequest->Parameters[0].length() < 1 || incomingRequest->Parameters[0].length() > 10 || !std::all_of(incomingRequest->Parameters[0].begin(), incomingRequest->Parameters[0].end(), [] (unsigned char c)
        {
            return std::isalnum(c);
        }))
    {
        Logger::LogError<GameLobby>(ERROR_CODES::BAD_PARAMETERS);
        return;
    }

    auto demandedClient = _clients.find(incomingRequest->ClientId);

    if (demandedClient == _clients.end())
    {
        Logger::LogError<GameLobby>(ERROR_CODES::UNKNOWN_USER);
        return;
    }

    for (auto& [id, client] : _clients)
    {
        if (client.Name == incomingRequest->Parameters[0])
        {
            if (!client.Online)
            {
                Logger::LogMessage<GameLobby>("Oflajn");
                _requestManager->SendResponse(_clients.at(incomingRequest->ClientId), OPCODE::VALID_NAME, {std::to_string(static_cast<uint32_t>(GAME_BOOL::TRUE)), std::to_string(static_cast<uint32_t>(client.Id)), std::to_string(++client.ConnectionID)}, BIND_TIMEOUT);
                _clients.erase(incomingRequest->ClientId);
                client.LastEcho = std::chrono::steady_clock::now();
                client.Online = true;
                client.Addr = incomingRequest->SourceAddress;
                client.Pinged = false;
                return;

            }

            Logger::LogMessage<GameLobby>("Onlajn");
            _requestManager->SendResponse(_clients.at(incomingRequest->ClientId), OPCODE::VALID_NAME, {std::to_string(static_cast<uint32_t>(GAME_BOOL::FALSE))}, BIND_TIMEOUT);
            return;
        }
    }

    _clients.at(incomingRequest->ClientId).Name = incomingRequest->Parameters[0];
    _requestManager->SendResponse(_clients.at(incomingRequest->ClientId), OPCODE::VALID_NAME, {std::to_string(static_cast<uint32_t>(GAME_BOOL::TRUE))}, BIND_TIMEOUT);
}

void GameLobby::ListRooms(std::shared_ptr<IncomingRequest>& incomingRequest)
{
    auto demandedClient = _clients.find(incomingRequest->ClientId);

    if (demandedClient == _clients.end())
    {
        Logger::LogError<GameLobby>(ERROR_CODES::UNKNOWN_USER);
        return;
    }

    if (!demandedClient->second.Online)
    {
        Logger::LogError<GameLobby>(ERROR_CODES::BAD_OPERATION);
        return;
    }

    std::vector<std::string> roomList;
    roomList.reserve(_gameRooms.size() * 3);

    for (const auto& [_, gr] : _gameRooms)
    {
        if (gr->GetGameState() != GAME_STATE::PRE_GAME)
        {
            continue;
        }

        roomList.push_back(std::to_string(gr->Id));
        roomList.push_back(std::to_string(static_cast<uint32_t>(gr->GetWhiteId() > 0 ? GAME_BOOL::TRUE : GAME_BOOL::FALSE)));
        roomList.push_back(std::to_string(static_cast<uint32_t>(gr->GetBlackId() > 0 ? GAME_BOOL::TRUE : GAME_BOOL::FALSE)));
    }

    _requestManager->SendResponse(_clients.at(incomingRequest->ClientId), OPCODE::ROOM_LIST, roomList, BIND_TIMEOUT);
}

void GameLobby::CreateRoom(std::shared_ptr<IncomingRequest>& incomingRequest)
{
    auto demandedClient = _clients.find(incomingRequest->ClientId);

    if (demandedClient == _clients.end())
    {
        Logger::LogError<GameLobby>(ERROR_CODES::UNKNOWN_USER);
        return;
    }

    if (!demandedClient->second.Online)
    {
        Logger::LogError<GameLobby>(ERROR_CODES::BAD_OPERATION);
        return;
    }

    if (demandedClient->second.GameRoomId != 0)
    {
        Logger::LogError<GameLobby>(ERROR_CODES::BAD_OPERATION);
        return;
    }

    if (incomingRequest->Parameters.size() != 1 || !Utils::IsUint(incomingRequest->Parameters[0]))
    {
        Logger::LogError<GameLobby>(ERROR_CODES::BAD_PARAMETERS);
        return;
    }

    auto originCast = magic_enum::enum_cast<PLAYER_COLOR>(Utils::Str2Uint(incomingRequest->Parameters[0]));

    if (!originCast.has_value())
    {
        Logger::LogError<GameLobby>(ERROR_CODES::BAD_PARAMETERS);
        return;
    }

    if (_gameRooms.size() >= _maxRooms)
    {
        _requestManager->SendResponse(_clients.at(incomingRequest.get()->ClientId), OPCODE::ROOM_CREATED, {std::to_string(static_cast<uint32_t>(GAME_BOOL::FALSE))}, BIND_TIMEOUT);
        return;
    }

    std::unique_ptr<GameRoom> room = std::make_unique<GameRoom>();

    switch (originCast.value())
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
    auto demandedClient = _clients.find(incomingRequest->ClientId);

    if (demandedClient == _clients.end())
    {
        Logger::LogError<GameLobby>(ERROR_CODES::UNKNOWN_USER);
        return;
    }

    if (!demandedClient->second.Online)
    {
        Logger::LogError<GameLobby>(ERROR_CODES::BAD_OPERATION);
        return;
    }

    if (demandedClient->second.GameRoomId != 0)
    {
        Logger::LogError<GameLobby>(ERROR_CODES::BAD_OPERATION);
        return;
    }

    if (incomingRequest->Parameters.size() != 2 || !Utils::IsUint(incomingRequest->Parameters[0]) || !Utils::IsUint(incomingRequest->Parameters[1]))
    {
        Logger::LogError<GameLobby>(ERROR_CODES::BAD_PARAMETERS);
        return;
    }

    auto originCast = magic_enum::enum_cast<PLAYER_COLOR>(Utils::Str2Uint(incomingRequest->Parameters[1]));

    if (!originCast.has_value())
    {
        Logger::LogError<GameLobby>(ERROR_CODES::BAD_PARAMETERS);
        return;
    }

    uint32_t roomId = Utils::Str2Uint(incomingRequest->Parameters[0]);
    PLAYER_COLOR playerColor = originCast.value();

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
    _requestManager->CreateDemand(_clients.at(demandedRoom->second.get()->GetPlayerId(!playerColor)), OPCODE::OPPONENT_ARRIVED, {_clients.at(incomingRequest->ClientId).Name}, BIND_TIMEOUT, [this] (std::unique_ptr<IncomingRequest> ir)
        {
            PlayerGotOpponent(std::move(ir));
        }, OPCODE::KNOW_ABOUT_HIM);
}

void GameLobby::PlayedMove(std::shared_ptr<IncomingRequest>& incomingRequest)
{

    auto demandedClient = _clients.find(incomingRequest->ClientId);

    if (demandedClient == _clients.end())
    {
        Logger::LogError<GameLobby>(ERROR_CODES::UNKNOWN_USER);
        return;
    }

    if (!demandedClient->second.Online)
    {
        Logger::LogError<GameLobby>(ERROR_CODES::BAD_OPERATION);
        return;
    }

    uint32_t roomId = _clients.at(incomingRequest->ClientId).GameRoomId;

    auto demandedRoom = _gameRooms.find(roomId);

    if (demandedRoom == _gameRooms.end())
    {
        Logger::LogError<GameLobby>(ERROR_CODES::BAD_PARAMETERS);
        return;
    }

    if (demandedClient->second.GameRoomId != roomId)
    {
        Logger::LogError<GameLobby>(ERROR_CODES::BAD_PARAMETERS);
        return;
    }

    if (incomingRequest->Parameters.size() != 4 || !Utils::IsUint(incomingRequest->Parameters[0]) || !Utils::IsUint(incomingRequest->Parameters[1]) || !Utils::IsUint(incomingRequest->Parameters[2]) || !Utils::IsUint(incomingRequest->Parameters[3]))
    {
        Logger::LogError<GameLobby>(ERROR_CODES::BAD_PARAMETERS);
        return;
    }

    uint32_t sX = Utils::Str2Uint(incomingRequest->Parameters[0]);
    uint32_t sY = Utils::Str2Uint(incomingRequest->Parameters[1]);
    uint32_t eX = Utils::Str2Uint(incomingRequest->Parameters[2]);
    uint32_t eY = Utils::Str2Uint(incomingRequest->Parameters[3]);

    if (sX < 0 || sY < 0 || eX < 0 || eY < 0 || sX > 7 || sY > 7 || eX > 7 || eY > 7)
    {
        Logger::LogError<GameLobby>(ERROR_CODES::BAD_PARAMETERS);
        return;
    }

    if (!demandedRoom->second->PlayMove(sX, sY, eX, eY))
    {
        _requestManager->SendResponse(_clients.at(incomingRequest.get()->ClientId), OPCODE::YOU_MOVED, {std::to_string(static_cast<uint32_t>(GAME_BOOL::FALSE))}, BIND_TIMEOUT);
        return;
    }

    _requestManager->SendResponse(_clients.at(incomingRequest->ClientId), OPCODE::YOU_MOVED, {std::to_string(static_cast<uint32_t>(GAME_BOOL::TRUE))}, BIND_TIMEOUT);

    _requestManager->CreateDemand(_clients.at(demandedRoom->second->GetOtherPlayerId(incomingRequest->ClientId)), OPCODE::HE_MOVED, incomingRequest->Parameters, BIND_TIMEOUT, [this] (std::unique_ptr<IncomingRequest> ir)
        {
            PlayerGotMove(std::move(ir));
        }, OPCODE::I_KNOW_HE_MOVED);


    GAME_STATE gameState = demandedRoom->second->GetGameState();
    if (gameState == GAME_STATE::IN_PROGRESS)
    {
        return;
    }

    _requestManager->CreateDemand(_clients.at(demandedRoom->second->GetWhiteId()), OPCODE::GAME_FINISHED, {std::to_string(static_cast<uint32_t>(gameState))}, BIND_TIMEOUT, [this] (std::unique_ptr<IncomingRequest> ir)
        {
            PlayerKnowResult(std::move(ir));
        }, OPCODE::FINALY_END);

    _requestManager->CreateDemand(_clients.at(demandedRoom->second->GetBlackId()), OPCODE::GAME_FINISHED, {std::to_string(static_cast<uint32_t>(gameState))}, BIND_TIMEOUT, [this] (std::unique_ptr<IncomingRequest> ir)
        {
            PlayerKnowResult(std::move(ir));
        }, OPCODE::FINALY_END);
}

void GameLobby::PlayerLeave(std::shared_ptr<IncomingRequest>& incomingRequest)
{
    auto demandedClient = _clients.find(incomingRequest->ClientId);

    if (demandedClient == _clients.end())
    {
        Logger::LogError<GameLobby>(ERROR_CODES::UNKNOWN_USER);
        return;
    }

    if (!demandedClient->second.Online)
    {
        Logger::LogError<GameLobby>(ERROR_CODES::BAD_OPERATION);
        return;
    }

    _requestManager->SendResponse(_clients.at(incomingRequest->ClientId), OPCODE::YOU_LEFT, {}, BIND_TIMEOUT);
    _clients.at(incomingRequest->ClientId).Online = false;
}

void GameLobby::PlayerQuit(std::shared_ptr<IncomingRequest>& incomingRequest)
{
    auto demandedClient = _clients.find(incomingRequest->ClientId);

    if (demandedClient == _clients.end())
    {
        Logger::LogError<GameLobby>(ERROR_CODES::UNKNOWN_USER);
        return;
    }

    if (!demandedClient->second.Online)
    {
        Logger::LogError<GameLobby>(ERROR_CODES::BAD_OPERATION);
        return;
    }

    if (demandedClient->second.GameRoomId == 0)
    {
        Logger::LogError<GameLobby>(ERROR_CODES::BAD_OPERATION);
        return;
    }

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

    if (!_gameRooms.at(roomId)->IsInitialPosition())
    {
        _gameRooms.at(roomId)->SetGameEnd(_gameRooms.at(roomId)->GetPlayerColor(otherPlayerId) == PLAYER_COLOR::WHITE ? GAME_STATE::WHITE_WIN : GAME_STATE::BLACK_WIN);
    }

    if (!_clients.at(otherPlayerId).Online)
    {
        return;
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
        return;
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
}

void GameLobby::ReconnectPlayer(std::shared_ptr<IncomingRequest>& incomingRequest)
{
    auto demandedClient = _clients.find(incomingRequest->ClientId);

    if (demandedClient == _clients.end())
    {
        Logger::LogError<GameLobby>(ERROR_CODES::UNKNOWN_USER);
        return;
    }

    if (demandedClient->second.Online)
    {
        Logger::LogError<GameLobby>(ERROR_CODES::BAD_OPERATION);
        return;
    }


    _clients.at(incomingRequest->ClientId).Online = true;
    _clients.at(incomingRequest->ClientId).LastEcho = std::chrono::steady_clock::now();
    _clients.at(incomingRequest->ClientId).Pinged = false;

    _requestManager->SendResponse(_clients.at(incomingRequest->ClientId), OPCODE::RECON_AS, {std::to_string(++_clients.at(incomingRequest->ClientId).ConnectionID)}, BIND_TIMEOUT);

    if (_clients.at(incomingRequest->ClientId).GameRoomId == 0)
    {
        return;
    }

    uint32_t othreId = _gameRooms.at(_clients.at(incomingRequest->ClientId).GameRoomId)->GetOtherPlayerId(incomingRequest->ClientId);

    if (othreId == 0)
    {
        return;
    }

    _requestManager->CreateDemand(_clients.at(othreId), OPCODE::HES_MISSING, {std::to_string(static_cast<uint32_t>(GAME_BOOL::FALSE))}, BIND_TIMEOUT, [this] (std::unique_ptr<IncomingRequest> ir)
        {
            PlayerKnowOthreMissing(std::move(ir));
        }, OPCODE::THATS_A_SHAME
    );
}

void GameLobby::SendLastMove(std::shared_ptr<IncomingRequest>& incomingRequest)
{
    auto demandedClient = _clients.find(incomingRequest->ClientId);

    if (demandedClient == _clients.end())
    {
        Logger::LogError<GameLobby>(ERROR_CODES::UNKNOWN_USER);
        return;
    }

    if (!demandedClient->second.Online)
    {
        Logger::LogError<GameLobby>(ERROR_CODES::BAD_OPERATION);
        return;
    }

    if (demandedClient->second.GameRoomId == 0)
    {
        Logger::LogError<GameLobby>(ERROR_CODES::BAD_OPERATION);
        return;
    }

    Client& c = _clients.at(incomingRequest->ClientId);

    _requestManager->SendResponse(c, OPCODE::THIS_HAPPEND, {
        std::to_string(_gameRooms.at(c.GameRoomId)->LastMove.first.row),
        std::to_string(_gameRooms.at(c.GameRoomId)->LastMove.first.col),
        std::to_string(_gameRooms.at(c.GameRoomId)->LastMove.second.row),
        std::to_string(_gameRooms.at(c.GameRoomId)->LastMove.second.col)
        }, BIND_TIMEOUT);
    
    GAME_STATE gameState = _gameRooms.at(c.GameRoomId)->GetGameState();

    auto demandedPlayer = _clients.find(_gameRooms.at(c.GameRoomId)->GetOtherPlayerId(c.Id));

    std::string otherPlayer;

    if (demandedPlayer == _clients.end())
    {
        otherPlayer = "";
    }
    else
    {
        otherPlayer = demandedPlayer->second.Name;
    }


    switch (gameState)
    {
        case GAME_STATE::IN_PROGRESS:
            break;

        case GAME_STATE::WHITE_WIN:
        case GAME_STATE::BLACK_WIN:
        case GAME_STATE::DRAW:
            _requestManager->CreateDemand(c, OPCODE::GAME_FINISHED, {std::to_string(static_cast<uint32_t>(gameState))}, BIND_TIMEOUT, [this] (std::unique_ptr<IncomingRequest> ir)
                {
                    PlayerKnowResult(std::move(ir));
                }, OPCODE::FINALY_END);
            break;

        case GAME_STATE::PRE_GAME:
            _requestManager->CreateDemand(
                c,
                OPCODE::OPPONENT_ARRIVED,
                {otherPlayer},
                BIND_TIMEOUT,
                [this] (std::unique_ptr<IncomingRequest> ir)
                {
                    PlayerGotOpponent(std::move(ir));
                }, OPCODE::KNOW_ABOUT_HIM
            );
            break;
        default:
            Logger::LogError<GameLobby>(ERROR_CODES::UNKNOWN_GAME_STATE);
            break;
    }
}

void GameLobby::OtherStatus(std::shared_ptr<IncomingRequest>& incomingRequest)
{
    auto demandedClient = _clients.find(incomingRequest->ClientId);

    if (demandedClient == _clients.end())
    {
        Logger::LogError<GameLobby>(ERROR_CODES::UNKNOWN_USER);
        return;
    }

    if (!demandedClient->second.Online)
    {
        Logger::LogError<GameLobby>(ERROR_CODES::BAD_OPERATION);
        return;
    }

    if (demandedClient->second.GameRoomId == 0)
    {
        Logger::LogError<GameLobby>(ERROR_CODES::BAD_OPERATION);
        return;
    }

    uint32_t roomId = _clients.at(incomingRequest->ClientId).GameRoomId;

    if (_gameRooms.at(roomId)->GetOtherPlayerId(demandedClient->second.GameRoomId) == 0)
    {
        _requestManager->SendResponse(_clients.at(incomingRequest->ClientId), OPCODE::HE_IS_ALIVE, {std::to_string(static_cast<uint32_t>(GAME_BOOL::FALSE))}, BIND_TIMEOUT);
    }

    _requestManager->SendResponse(_clients.at(incomingRequest->ClientId), OPCODE::HE_IS_ALIVE, {std::to_string(static_cast<uint32_t>((_clients.at(_gameRooms.at(_clients.at(incomingRequest->ClientId).GameRoomId)->GetOtherPlayerId(incomingRequest->ClientId)).Online ? GAME_BOOL::TRUE : GAME_BOOL::FALSE)))}, BIND_TIMEOUT);
}

void GameLobby::IsPlayerInGame(std::shared_ptr<IncomingRequest>& incomingRequest)
{
    uint32_t roomId = _clients.at(incomingRequest->ClientId).GameRoomId;

    if (roomId == 0)
    {
        _requestManager->SendResponse(_clients.at(incomingRequest->ClientId), OPCODE::YOU_PLAY, {
            std::to_string(static_cast<uint32_t>(GAME_BOOL::FALSE))
            }, BIND_TIMEOUT);
        return;
    }

    uint32_t otherId = _gameRooms.at(roomId)->GetOtherPlayerId(incomingRequest->ClientId);

    if (otherId == 0)
    {
        _requestManager->SendResponse(_clients.at(incomingRequest->ClientId), OPCODE::YOU_PLAY, {
            std::to_string(static_cast<uint32_t>(GAME_BOOL::TRUE)),
            std::to_string(static_cast<uint32_t>(_gameRooms.at(roomId)->GetPlayerColor(incomingRequest->ClientId))),
            ""
                }, BIND_TIMEOUT);

        return;
    }

    _requestManager->SendResponse(_clients.at(incomingRequest->ClientId), OPCODE::YOU_PLAY, {
        std::to_string(static_cast<uint32_t>(GAME_BOOL::TRUE)),
        std::to_string(static_cast<uint32_t>(_gameRooms.at(roomId)->GetPlayerColor(incomingRequest->ClientId))),
        _clients.at(otherId).Name
        }, BIND_TIMEOUT);

    _requestManager->CreateDemand(_clients.at(otherId), OPCODE::HES_MISSING, {std::to_string(static_cast<uint32_t>(GAME_BOOL::FALSE))}, BIND_TIMEOUT, [this] (std::unique_ptr<IncomingRequest> ir)
        {
            PlayerKnowOthreMissing(std::move(ir));
        }, OPCODE::THATS_A_SHAME
    );
}

void GameLobby::SendBoard(std::shared_ptr<IncomingRequest>& incomingRequest)
{
    uint32_t roomId = _clients.at(incomingRequest->ClientId).GameRoomId;

    _requestManager->SendResponse(_clients.at(incomingRequest->ClientId), OPCODE::HERE_POSITION, {_gameRooms.at(roomId)->GetPositionString(), std::to_string(static_cast<uint32_t>(_gameRooms.at(roomId)->GetActualPlayer()))}, BIND_TIMEOUT);
}