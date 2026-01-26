#pragma once

#include "PlayerColor.h"
#include "PieceType.h"
#include "GameState.h"

#include <vector>
#include <optional>
#include <iostream>

struct Piece
{
    PLAYER_COLOR owner;
    PIECE_TYPE type;
};

struct Position
{
    int row, col;
    bool operator==(const Position& other) const
    {
        return row == other.row && col == other.col;
    }
};

class Chessboard
{
public:
    static const int SIZE = 8;
    bool InitialPosition;

    Chessboard();

    bool movePiece(Position start, Position end);
    void print() const;
    GAME_STATE GetGameState() const;

    PLAYER_COLOR getCurrentPlayer() const
    {
        return currentPlayer;
    }

private:
    std::optional<Piece> board[SIZE][SIZE];
    PLAYER_COLOR currentPlayer;
    std::optional<Position> multiJumpPos;

    void setupInitialBoard();
    bool isWithinBounds(Position pos) const;
    void promoteToKing(Position pos);

    // Logika pohybu a braní
    bool isValidMove(Position start, Position end) const;
    bool canCapture(Position start, Position end) const;
    bool canAnyPieceCapture(PLAYER_COLOR player) const;
    bool canPieceCapture(Position pos) const;
};
