#pragma once

#include <cstdlib>

enum class GAME_STATE : uint32_t
{
	IN_PROGRESS,
	WHITE_WIN,
	BLACK_WIN,
	DRAW,
	PRE_GAME
};