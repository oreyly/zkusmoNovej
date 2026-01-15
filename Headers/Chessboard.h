#pragma once

#include <vector>
#include <optional>
#include <iostream>

enum class Player
{
    White, Black
};
enum class PieceType
{
    Man, King
};

struct Piece
{
    Player owner;
    PieceType type;
};

struct Position
{
    int row, col;
    bool operator==(const Position& other) const
    {
        return row == other.row && col == other.col;
    }
};

class CheckersBoard
{
public:
    static const int SIZE = 8;

    CheckersBoard();

    bool movePiece(Position start, Position end);
    void print() const;
    bool isGameOver() const;

    Player getCurrentPlayer() const
    {
        return currentPlayer;
    }

private:
    std::optional<Piece> board[SIZE][SIZE];
    Player currentPlayer;
    std::optional<Position> multiJumpPos;

    void setupInitialBoard();
    bool isWithinBounds(Position pos) const;
    void promoteToKing(Position pos);

    // Logika pohybu a braní
    bool isValidMove(Position start, Position end) const;
    bool canCapture(Position start, Position end) const;
    bool canAnyPieceCapture(Player player) const;
    bool canPieceCapture(Position pos) const;
};