#include "Chessboard.h"
#include <cmath>

CheckersBoard::CheckersBoard()
{
    setupInitialBoard();
}

void CheckersBoard::setupInitialBoard()
{
    currentPlayer = Player::White;
    multiJumpPos = std::nullopt;
    for (int r = 0; r < SIZE; ++r)
    {
        for (int c = 0; c < SIZE; ++c)
        {
            board[r][c] = std::nullopt;
            if ((r + c) % 2 != 0)
            {
                if (r < 3) board[r][c] = Piece {Player::Black, PieceType::Man};
                else if (r > 4) board[r][c] = Piece {Player::White, PieceType::Man};
            }
        }
    }
}

bool CheckersBoard::isWithinBounds(Position pos) const
{
    return pos.row >= 0 && pos.row < SIZE && pos.col >= 0 && pos.col < SIZE;
}

bool CheckersBoard::canCapture(Position start, Position end) const
{
    if (!isWithinBounds(start) || !isWithinBounds(end) || board[end.row][end.col]) return false;

    auto p = board[start.row][start.col];
    int dr = end.row - start.row;
    int dc = end.col - start.col;

    if (std::abs(dr) != std::abs(dc) || std::abs(dr) < 2) return false;

    int stepR = (dr > 0) ? 1 : -1;
    int stepC = (dc > 0) ? 1 : -1;

    if (p->type == PieceType::Man)
    {
        if (std::abs(dr) != 2) return false;
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

bool CheckersBoard::canPieceCapture(Position pos) const
{
    auto p = board[pos.row][pos.col];
    if (!p) return false;
    int maxDist = (p->type == PieceType::King) ? SIZE : 2;
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

bool CheckersBoard::canAnyPieceCapture(Player player) const
{
    for (int r = 0; r < SIZE; ++r)
        for (int c = 0; c < SIZE; ++c)
            if (board[r][c] && board[r][c]->owner == player && canPieceCapture({r, c})) return true;
    return false;
}

bool CheckersBoard::isValidMove(Position start, Position end) const
{
    if (!isWithinBounds(start) || !isWithinBounds(end) || board[end.row][end.col]) return false;
    auto p = board[start.row][start.col];
    int dr = end.row - start.row, dc = std::abs(end.col - start.col);

    if (p->type == PieceType::Man)
    {
        int dir = (p->owner == Player::White) ? -1 : 1;
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

bool CheckersBoard::movePiece(Position start, Position end)
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
        currentPlayer = (currentPlayer == Player::White) ? Player::Black : Player::White;
    }
    return true;
}

void CheckersBoard::promoteToKing(Position pos)
{
    if ((board[pos.row][pos.col]->owner == Player::White && pos.row == 0) ||
        (board[pos.row][pos.col]->owner == Player::Black && pos.row == SIZE - 1))
        board[pos.row][pos.col]->type = PieceType::King;
}

void CheckersBoard::print() const
{
    std::cout << "  0 1 2 3 4 5 6 7\n";
    for (int r = 0; r < SIZE; ++r)
    {
        std::cout << r << " ";
        for (int c = 0; c < SIZE; ++c)
        {
            if (!board[r][c]) std::cout << ((r + c) % 2 != 0 ? ". " : "  ");
            else std::cout << (board[r][c]->owner == Player::White ? (board[r][c]->type == PieceType::King ? "W " : "w ") : (board[r][c]->type == PieceType::King ? "B " : "b "));
        }
        std::cout << r << "\n";
    }
    std::cout << "  0 1 2 3 4 5 6 7\nNa tahu: " << (currentPlayer == Player::White ? "Bila" : "Cerna") << "\n";
}

bool CheckersBoard::isGameOver() const
{
    for (int r = 0; r < SIZE; ++r)
        for (int c = 0; c < SIZE; ++c)
            if (board[r][c] && board[r][c]->owner == currentPlayer)
            {
                if (canPieceCapture({r, c})) return false;
                for (int dr = -1; dr <= 1; dr += 2)
                    for (int dc = -1; dc <= 1; dc += 2)
                        if (isValidMove({r, c}, {r + dr, c + dc})) return false;
            }
    return true;
}