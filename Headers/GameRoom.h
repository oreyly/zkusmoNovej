#pragma once

#include "PlayerColor.h"
#include "Chessboard.h"
#include "GameBool.h"
#include "GameState.h"

#include <cstdint>
#include <unordered_map>


class GameRoom
{
public:
	GameRoom();

	const uint32_t Id;

	std::pair<Position, Position> LastMove;

	uint32_t GetWhiteId();
	uint32_t GetBlackId();
	uint32_t GetPlayerId(PLAYER_COLOR playerColor);

	uint32_t GetOtherPlayerId(uint32_t id);

	void SetWhiteId(uint32_t id);
	void SetBlackId(uint32_t id);
	void SetPlayerId(PLAYER_COLOR playerColor, uint32_t id);

	void RemovePlayer(uint32_t id);

	bool IsInitialPosition();

	PLAYER_COLOR GetPlayerColor(uint32_t id);

	bool PlayMove(uint32_t startX, uint32_t startY, uint32_t endX, uint32_t endY);
	GAME_STATE GetGameState();
	void SetGameEnd(GAME_STATE gameState);

	std::string GetPositionString() const;
	PLAYER_COLOR GetActualPlayer() const;

private:
	std::unordered_map <PLAYER_COLOR, uint32_t> _players;
	Chessboard _chessboard;

	bool _manualyEnd;
	GAME_STATE _gameState;

	static inline uint32_t _maxId = 0;
	static uint32_t NextId();
};
