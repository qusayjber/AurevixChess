#pragma once
#include "chess/chess.hpp"

namespace engine {

// Piece-square tables (from White's perspective, square 0=a1). Values in centipawns.
extern const int PawnTable[64];
extern const int KnightTable[64];
extern const int BishopTable[64];
extern const int RookTable[64];
extern const int QueenTable[64];
extern const int KingMidTable[64];
extern const int KingEndTable[64];

// Evaluate position from White's perspective (positive = White better).
int evaluate(const chess::Board& b);

// Returns game phase in [0..1] where 1 = full middlegame, 0 = endgame
float game_phase(const chess::Board& b);

} // namespace engine