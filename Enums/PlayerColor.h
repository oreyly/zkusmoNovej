#pragma once

#include <cstdint>

enum class PLAYER_COLOR : uint32_t
{
    BLACK,
    WHITE
};

constexpr PLAYER_COLOR operator!(PLAYER_COLOR color)
{
    return (color == PLAYER_COLOR::BLACK) ? PLAYER_COLOR::WHITE : PLAYER_COLOR::BLACK;
}