#include "Chessboard.h"
#include <cmath>

Chessboard::Chessboard()
{
    setupInitialBoard();
}

void Chessboard::setupInitialBoard()
{
    InitialPosition = true;
    currentPlayer = PLAYER_COLOR::WHITE;
    multiJumpPos = std::nullopt;
    for (int r = 0; r < SIZE; ++r)
    {
        for (int c = 0; c < SIZE; ++c)
        {
            board[r][c] = std::nullopt;
            if ((r + c) % 2 != 0)
            {
                if (r < 3) board[r][c] = Piece {PLAYER_COLOR::BLACK, PIECE_TYPE::MAN};
                else if (r > 4) board[r][c] = Piece {PLAYER_COLOR::WHITE, PIECE_TYPE::MAN};
            }
        }
    }
}

bool Chessboard::isWithinBounds(Position pos) const
{
    return pos.row >= 0 && pos.row < SIZE && pos.col >= 0 && pos.col < SIZE;
}

bool Chessboard::canCapture(Position start, Position end) const
{
    if (!isWithinBounds(start) || !isWithinBounds(end) || board[end.row][end.col]) return false;

    auto p = board[start.row][start.col];
    int dr = end.row - start.row;
    int dc = end.col - start.col;

    if (std::abs(dr) != std::abs(dc) || std::abs(dr) < 2) return false;

    int stepR = (dr > 0) ? 1 : -1;
    int stepC = (dc > 0) ? 1 : -1;

    if (p->type == PIECE_TYPE::MAN)
    {
        if (std::abs(dr) != 2) return false;

        int allowedDir = (p->owner == PLAYER_COLOR::WHITE) ? -2 : 2;
        if (dr != allowedDir) return false;

        Position mid {start.row + stepR, start.col + stepC};
        return board[mid.row][mid.col] && board[mid.row][mid.col]->owner != p->owner;
    }
    else
    { // King
        int enemyCount = 0;
        int r = start.row + stepR, c = start.col + stepC;
        while (r != end.row)
        {
            if (board[r][c])
            {
                if (board[r][c]->owner == p->owner) return false;
                enemyCount++;
            }
            r += stepR; c += stepC;
        }
        return enemyCount == 1;
    }
}

bool Chessboard::canPieceCapture(Position pos) const
{
    auto p = board[pos.row][pos.col];
    if (!p) return false;
    int maxDist = (p->type == PIECE_TYPE::KING) ? SIZE : 2;
    for (int d = 2; d <= maxDist; ++d)
    {
        int dr[] = {d, d, -d, -d}, dc[] = {d, -d, d, -d};
        for (int i = 0; i < 4; ++i)
        {
            if (canCapture(pos, {pos.row + dr[i], pos.col + dc[i]})) return true;
        }
    }
    return false;
}

bool Chessboard::canAnyPieceCapture(PLAYER_COLOR player) const
{
    for (int r = 0; r < SIZE; ++r)
        for (int c = 0; c < SIZE; ++c)
            if (board[r][c] && board[r][c]->owner == player && canPieceCapture({r, c})) return true;
    return false;
}

bool Chessboard::isValidMove(Position start, Position end) const
{
    if (!isWithinBounds(start) || !isWithinBounds(end) || board[end.row][end.col]) return false;
    auto p = board[start.row][start.col];
    int dr = end.row - start.row, dc = std::abs(end.col - start.col);

    if (p->type == PIECE_TYPE::MAN)
    {
        int dir = (p->owner == PLAYER_COLOR::WHITE) ? -1 : 1;
        return (dr == dir && dc == 1);
    }
    else
    { // King - libovolný pohyb po prázdné diagonále
        if (std::abs(dr) != dc) return false;
        int stepR = (dr > 0) ? 1 : -1, stepC = (end.col > start.col) ? 1 : -1;
        int r = start.row + stepR, c = start.col + stepC;
        while (r != end.row)
        {
            if (board[r][c]) return false;
            r += stepR; c += stepC;
        }
        return true;
    }
}

bool Chessboard::movePiece(Position start, Position end)
{
    auto p = board[start.row][start.col];
    if (!p || p->owner != currentPlayer) return false;
    if (multiJumpPos && !(start == *multiJumpPos)) return false;

    bool isCap = canCapture(start, end);
    if (canAnyPieceCapture(currentPlayer) && !isCap) return false;
    if (!isCap && !isValidMove(start, end)) return false;

    // Provedení pohybu
    if (isCap)
    {
        int stepR = (end.row - start.row > 0) ? 1 : -1;
        int stepC = (end.col - start.col > 0) ? 1 : -1;
        int r = start.row + stepR, c = start.col + stepC;
        while (r != end.row)
        {
            board[r][c] = std::nullopt; r += stepR; c += stepC;
        }
    }

    board[end.row][end.col] = board[start.row][start.col];
    board[start.row][start.col] = std::nullopt;

    if (isCap && canPieceCapture(end))
    {
        multiJumpPos = end;
    }
    else
    {
        promoteToKing(end);
        multiJumpPos = std::nullopt;
        currentPlayer = (currentPlayer == PLAYER_COLOR::WHITE) ? PLAYER_COLOR::BLACK : PLAYER_COLOR::WHITE;
    }

    if (InitialPosition)
    {
        InitialPosition = false;
    }

    return true;
}

void Chessboard::promoteToKing(Position pos)
{
    if ((board[pos.row][pos.col]->owner == PLAYER_COLOR::WHITE && pos.row == 0) ||
        (board[pos.row][pos.col]->owner == PLAYER_COLOR::BLACK && pos.row == SIZE - 1))
        board[pos.row][pos.col]->type = PIECE_TYPE::KING;
}

void Chessboard::print() const
{
    std::cout << "  0 1 2 3 4 5 6 7\n";
    for (int r = 0; r < SIZE; ++r)
    {
        std::cout << r << " ";
        for (int c = 0; c < SIZE; ++c)
        {
            if (!board[r][c]) std::cout << ((r + c) % 2 != 0 ? ". " : "  ");
            else std::cout << (board[r][c]->owner == PLAYER_COLOR::WHITE ? (board[r][c]->type == PIECE_TYPE::KING ? "W " : "w ") : (board[r][c]->type == PIECE_TYPE::KING ? "B " : "b "));
        }
        std::cout << r << "\n";
    }
    std::cout << "  0 1 2 3 4 5 6 7\nNa tahu: " << (currentPlayer == PLAYER_COLOR::WHITE ? "Bila" : "Cerna") << "\n";
}

GAME_STATE Chessboard::GetGameState() const
{
    bool whiteHasFigs = false;
    bool blackHasFigs = false;

    for (int r = 0; r < SIZE; ++r)
        for (int c = 0; c < SIZE; ++c)
            if (board[r][c])
            {
                if (!whiteHasFigs && board[r][c]->owner == PLAYER_COLOR::WHITE)
                {
                    whiteHasFigs = true;
                }
                
                if(!blackHasFigs && board[r][c]->owner == PLAYER_COLOR::BLACK)
                {
                    blackHasFigs = true;
                }

                if (board[r][c]->owner == currentPlayer)
                {
                    if (canPieceCapture({r, c})) return GAME_STATE::IN_PROGRESS;
                    for (int dr = -1; dr <= 1; dr += 2)
                        for (int dc = -1; dc <= 1; dc += 2)
                            if (isValidMove({r, c}, {r + dr, c + dc})) return GAME_STATE::IN_PROGRESS;
                }
            }

    if (!whiteHasFigs)
    {
        return GAME_STATE::BLACK_WIN;
    }

    if (!blackHasFigs)
    {
        return GAME_STATE::WHITE_WIN;
    }

    return GAME_STATE::DRAW;
}
