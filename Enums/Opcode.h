#pragma once

#include <cstdint>

enum class OPCODE : uint32_t
{
    TEST,

    ACK = 2,

    CON = 4,
    CON_AS,

    WANA_NAME,
    VALID_NAME,

    WANA_ROOMS,
    ROOM_LIST,

    CREATE_ROOM,
    ROOM_CREATED,

    GET_TO_ROOM,
    GOT_TO_ROOM,

    OPPONENT_ARRIVED,
    KNOW_ABOUT_HIM,

    I_MOVED,
    YOU_MOVED,

    HE_MOVED,
    I_KNOW_HE_MOVED,

    GAME_FINISHED,
    FINALY_END,

    AM_LEAVING,
    YOU_LEFT,

    GAME_QUIT,
    GAME_QUITED,

    PING,
    I_SEE_YOU,
};
