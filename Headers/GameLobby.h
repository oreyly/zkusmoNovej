#pragma once

#include "GameRoom.h"

#include <vector>
#include <memory>

class Game
{
public:

private:
	std::vector<std::shared_ptr<GameRoom>> GameRooms;
};