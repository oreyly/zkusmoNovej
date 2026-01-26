#pragma once

#include <cstdint>

enum class GAME_BOOL : uint32_t
{
    FALSE,
    TRUE
};

constexpr GAME_BOOL operator!(GAME_BOOL gameBool)
{
    return (gameBool == GAME_BOOL::TRUE) ? GAME_BOOL::FALSE : GAME_BOOL::TRUE;
}