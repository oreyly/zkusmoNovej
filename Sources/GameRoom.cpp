#include "GameRoom.h"

GameRoom::GameRoom() : Id(NextId())
{
	_players[PLAYER_COLOR::WHITE] = 0;
	_players[PLAYER_COLOR::BLACK] = 0;
}

uint32_t GameRoom::GetWhiteId()
{
	return _players[PLAYER_COLOR::WHITE];
}

uint32_t GameRoom::GetBlackId()
{
	return _players[PLAYER_COLOR::BLACK];
}

uint32_t GameRoom::GetPlayerId(PLAYER_COLOR playerColor)
{
	return _players[playerColor];
}

uint32_t GameRoom::GetOtherPlayerId(uint32_t id)
{
	return id == _players[PLAYER_COLOR::BLACK] ? _players[PLAYER_COLOR::WHITE] : _players[PLAYER_COLOR::BLACK];
}
void GameRoom::SetWhiteId(uint32_t id)
{
	_players[PLAYER_COLOR::WHITE] = id;
}

void GameRoom::SetBlackId(uint32_t id)
{
	_players[PLAYER_COLOR::BLACK] = id;
}

void GameRoom::SetPlayerId(PLAYER_COLOR playerColor, uint32_t id)
{
	_players[playerColor] = id;
}

bool GameRoom::IsInitialPosition()
{
	return _chessboard.InitialPosition;
}

PLAYER_COLOR GameRoom::GetPlayerColor(uint32_t id)
{
	return _players.at(PLAYER_COLOR::WHITE) == id ? PLAYER_COLOR::WHITE : PLAYER_COLOR::BLACK;
}

void GameRoom::RemovePlayer(uint32_t id)
{
	if (_players.at(PLAYER_COLOR::WHITE) == id)
	{
		_players.at(PLAYER_COLOR::WHITE) = 0;
	}
	else
	{
		_players.at(PLAYER_COLOR::BLACK) = 0;
	}
}

bool GameRoom::PlayMove(uint32_t startX, uint32_t startY, uint32_t endX, uint32_t endY)
{
	return _chessboard.movePiece({static_cast<int>(startX), static_cast<int>(startY)}, {static_cast<int>(endX), static_cast<int>(endY)});
}

GAME_STATE GameRoom::GetGameState()
{
	return _chessboard.GetGameState();
}

uint32_t GameRoom::NextId()
{
	return ++_maxId;
}