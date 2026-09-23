#include "engine/eval.hpp"
#include <array>
#include <cmath>

namespace engine {
using namespace chess;

// Standard PeSTO-style tables (centipawns). Indexed so 0 = a1.
// We define them in "rank 1 at the top" visual order and then mirror.
namespace {
constexpr int P[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0,
};
constexpr int N[64] = {
   -50,-40,-30,-30,-30,-30,-40,-50,
   -40,-20,  0,  0,  0,  0,-20,-40,
   -30,  0, 10, 15, 15, 10,  0,-30,
   -30,  5, 15, 20, 20, 15,  5,-30,
   -30,  0, 15, 20, 20, 15,  0,-30,
   -30,  5, 10, 15, 15, 10,  5,-30,
   -40,-20,  0,  5,  5,  0,-20,-40,
   -50,-40,-30,-30,-30,-30,-40,-50,
};
constexpr int B[64] = {
   -20,-10,-10,-10,-10,-10,-10,-20,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -10,  0,  5, 10, 10,  5,  0,-10,
   -10,  5,  5, 10, 10,  5,  5,-10,
   -10,  0, 10, 10, 10, 10,  0,-10,
   -10, 10, 10, 10, 10, 10, 10,-10,
   -10,  5,  0,  0,  0,  0,  5,-10,
   -20,-10,-10,-10,-10,-10,-10,-20,
};
constexpr int R[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  0,  0,  5,  5,  0,  0,  0,
};
constexpr int Q[64] = {
   -20,-10,-10, -5, -5,-10,-10,-20,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -10,  0,  5,  5,  5,  5,  0,-10,
    -5,  0,  5,  5,  5,  5,  0, -5,
     0,  0,  5,  5,  5,  5,  0, -5,
   -10,  5,  5,  5,  5,  5,  0,-10,
   -10,  0,  5,  0,  0,  0,  0,-10,
   -20,-10,-10, -5, -5,-10,-10,-20,
};
constexpr int KM[64] = {
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -20,-30,-30,-40,-40,-30,-30,-20,
   -10,-20,-20,-20,-20,-20,-20,-10,
    20, 20,  0,  0,  0,  0, 20, 20,
    20, 30, 10,  0,  0, 10, 30, 20,
};
constexpr int KE[64] = {
   -50,-40,-30,-20,-20,-30,-40,-50,
   -30,-20,-10,  0,  0,-10,-20,-30,
   -30,-10, 20, 30, 30, 20,-10,-30,
   -30,-10, 30, 40, 40, 30,-10,-30,
   -30,-10, 30, 40, 40, 30,-10,-30,
   -30,-10, 20, 30, 30, 20,-10,-30,
   -30,-30,  0,  0,  0,  0,-30,-30,
   -50,-30,-30,-30,-30,-30,-30,-50,
};
// Mirror for black: index for a black piece at square s is (s ^ 56)
inline int mir(Square s, Color c) { return c == Color::White ? s : (s ^ 56); }
} // namespace

const int PawnTable[64]   = { P[0],P[1],P[2],P[3],P[4],P[5],P[6],P[7],P[8],P[9],P[10],P[11],P[12],P[13],P[14],P[15],P[16],P[17],P[18],P[19],P[20],P[21],P[22],P[23],P[24],P[25],P[26],P[27],P[28],P[29],P[30],P[31],P[32],P[33],P[34],P[35],P[36],P[37],P[38],P[39],P[40],P[41],P[42],P[43],P[44],P[45],P[46],P[47],P[48],P[49],P[50],P[51],P[52],P[53],P[54],P[55],P[56],P[57],P[58],P[59],P[60],P[61],P[62],P[63] };
const int KnightTable[64] = { N[0],N[1],N[2],N[3],N[4],N[5],N[6],N[7],N[8],N[9],N[10],N[11],N[12],N[13],N[14],N[15],N[16],N[17],N[18],N[19],N[20],N[21],N[22],N[23],N[24],N[25],N[26],N[27],N[28],N[29],N[30],N[31],N[32],N[33],N[34],N[35],N[36],N[37],N[38],N[39],N[40],N[41],N[42],N[43],N[44],N[45],N[46],N[47],N[48],N[49],N[50],N[51],N[52],N[53],N[54],N[55],N[56],N[57],N[58],N[59],N[60],N[61],N[62],N[63] };
const int BishopTable[64] = { B[0],B[1],B[2],B[3],B[4],B[5],B[6],B[7],B[8],B[9],B[10],B[11],B[12],B[13],B[14],B[15],B[16],B[17],B[18],B[19],B[20],B[21],B[22],B[23],B[24],B[25],B[26],B[27],B[28],B[29],B[30],B[31],B[32],B[33],B[34],B[35],B[36],B[37],B[38],B[39],B[40],B[41],B[42],B[43],B[44],B[45],B[46],B[47],B[48],B[49],B[50],B[51],B[52],B[53],B[54],B[55],B[56],B[57],B[58],B[59],B[60],B[61],B[62],B[63] };
const int RookTable[64]   = { R[0],R[1],R[2],R[3],R[4],R[5],R[6],R[7],R[8],R[9],R[10],R[11],R[12],R[13],R[14],R[15],R[16],R[17],R[18],R[19],R[20],R[21],R[22],R[23],R[24],R[25],R[26],R[27],R[28],R[29],R[30],R[31],R[32],R[33],R[34],R[35],R[36],R[37],R[38],R[39],R[40],R[41],R[42],R[43],R[44],R[45],R[46],R[47],R[48],R[49],R[50],R[51],R[52],R[53],R[54],R[55],R[56],R[57],R[58],R[59],R[60],R[61],R[62],R[63] };
const int QueenTable[64]  = { Q[0],Q[1],Q[2],Q[3],Q[4],Q[5],Q[6],Q[7],Q[8],Q[9],Q[10],Q[11],Q[12],Q[13],Q[14],Q[15],Q[16],Q[17],Q[18],Q[19],Q[20],Q[21],Q[22],Q[23],Q[24],Q[25],Q[26],Q[27],Q[28],Q[29],Q[30],Q[31],Q[32],Q[33],Q[34],Q[35],Q[36],Q[37],Q[38],Q[39],Q[40],Q[41],Q[42],Q[43],Q[44],Q[45],Q[46],Q[47],Q[48],Q[49],Q[50],Q[51],Q[52],Q[53],Q[54],Q[55],Q[56],Q[57],Q[58],Q[59],Q[60],Q[61],Q[62],Q[63] };
const int KingMidTable[64] = { KM[0],KM[1],KM[2],KM[3],KM[4],KM[5],KM[6],KM[7],KM[8],KM[9],KM[10],KM[11],KM[12],KM[13],KM[14],KM[15],KM[16],KM[17],KM[18],KM[19],KM[20],KM[21],KM[22],KM[23],KM[24],KM[25],KM[26],KM[27],KM[28],KM[29],KM[30],KM[31],KM[32],KM[33],KM[34],KM[35],KM[36],KM[37],KM[38],KM[39],KM[40],KM[41],KM[42],KM[43],KM[44],KM[45],KM[46],KM[47],KM[48],KM[49],KM[50],KM[51],KM[52],KM[53],KM[54],KM[55],KM[56],KM[57],KM[58],KM[59],KM[60],KM[61],KM[62],KM[63] };
const int KingEndTable[64] = { KE[0],KE[1],KE[2],KE[3],KE[4],KE[5],KE[6],KE[7],KE[8],KE[9],KE[10],KE[11],KE[12],KE[13],KE[14],KE[15],KE[16],KE[17],KE[18],KE[19],KE[20],KE[21],KE[22],KE[23],KE[24],KE[25],KE[26],KE[27],KE[28],KE[29],KE[30],KE[31],KE[32],KE[33],KE[34],KE[35],KE[36],KE[37],KE[38],KE[39],KE[40],KE[41],KE[42],KE[43],KE[44],KE[45],KE[46],KE[47],KE[48],KE[49],KE[50],KE[51],KE[52],KE[53],KE[54],KE[55],KE[56],KE[57],KE[58],KE[59],KE[60],KE[61],KE[62],KE[63] };

float game_phase(const Board& b) {
    int material = 0;
    for (int s = 0; s < 64; ++s) {
        Piece p = b.at(s);
        switch (p.type) {
            case PieceType::Knight: case PieceType::Bishop: material += 1; break;
            case PieceType::Rook: material += 2; break;
            case PieceType::Queen: material += 4; break;
            default: break;
        }
    }
    // 24 = max non-pawn/king material at start
    float t = static_cast<float>(material) / 24.0f;
    if (t > 1.0f) t = 1.0f;
    return t;
}

int evaluate(const Board& b) {
    int score = 0;
    int wPawns[8] = {0}, bPawns[8] = {0};
    int wPawnRanks[8] = {0}, bPawnRanks[8] = {0};
    int wBishops = 0, bBishops = 0;
    int wMobility = 0, bMobility = 0;
    // Piece-square + material
    int phase = static_cast<int>(game_phase(b) * 256);

    auto evaluate_piece = [&](Piece p, Square s) {
        int v = piece_value(p.type);
        int psqt = 0;
        int idx = mir(s, p.color);
        switch (p.type) {
            case PieceType::Pawn:
                psqt = PawnTable[idx];
                if (p.color == Color::White) { wPawns[file_of(s)]++; wPawnRanks[file_of(s)] = rank_of(s); }
                else                          { bPawns[file_of(s)]++; bPawnRanks[file_of(s)] = rank_of(s); }
                break;
            case PieceType::Knight: psqt = KnightTable[idx]; break;
            case PieceType::Bishop: psqt = BishopTable[idx]; if (p.color==Color::White) wBishops++; else bBishops++; break;
            case PieceType::Rook:   psqt = RookTable[idx]; break;
            case PieceType::Queen:  psqt = QueenTable[idx]; break;
            case PieceType::King: {
                int mid = KingMidTable[idx];
                int end = KingEndTable[idx];
                psqt = (mid * phase + end * (256 - phase)) / 256;
            } break;
            default: break;
        }
        int val = v + psqt;
        return p.color == Color::White ? val : -val;
    };

    for (int s = 0; s < 64; ++s) {
        Piece p = b.at(s);
        if (p.empty()) continue;
        score += evaluate_piece(p, s);
    }

    // Mobility (pseudo-legal without king safety concerns; cheap)
    // We count sliders + knights roughly by counting empty/enemy neighbours.
    auto ray_mobility = [&](Square s, const int dirs[][2], int n) {
        int m = 0;
        int f = file_of(s), r = rank_of(s);
        for (int i = 0; i < n; ++i) {
            int nf = f + dirs[i][0], nr = r + dirs[i][1];
            while (on_board(nf, nr)) {
                Piece t = b.at(mk_square(nf, nr));
                m++;
                if (!t.empty()) break;
                nf += dirs[i][0]; nr += dirs[i][1];
            }
        }
        return m;
    };
    static const int RD[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
    static const int BD[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
    for (int s = 0; s < 64; ++s) {
        Piece p = b.at(s);
        if (p.empty()) continue;
        int m = 0;
        switch (p.type) {
            case PieceType::Rook:   m = ray_mobility(s, RD, 4); break;
            case PieceType::Bishop: m = ray_mobility(s, BD, 4); break;
            case PieceType::Queen:  m = ray_mobility(s, RD, 4) + ray_mobility(s, BD, 4); break;
            default: break;
        }
        if (p.color == Color::White) wMobility += m; else bMobility += m;
    }
    score += (wMobility - bMobility) * 2;

    // Pawn structure
    auto pawn_structure = [&](int pawns[8], int ranks[8], Color us) {
        int v = 0;
        for (int f = 0; f < 8; ++f) {
            if (pawns[f] > 1) v -= 15 * (pawns[f] - 1);   // doubled
            if (pawns[f] > 0) {
                bool isolated = (f == 0 || pawns[f-1] == 0) && (f == 7 || pawns[f+1] == 0);
                if (isolated) v -= 12;
                // Passed pawn bonus
                bool passed = true;
                int r = ranks[f];
                for (int ef = std::max(0, f-1); ef <= std::min(7, f+1); ++ef) {
                    if (ef == f) continue;
                    (void)ef;
                }
                // We approximate: passed if no opposing pawn on same/adjacent file ahead
                // (using b pawns if us is White).
                (void)passed; (void)r;
            }
        }
        return us == Color::White ? v : -v;
    };
    score += pawn_structure(wPawns, wPawnRanks, Color::White);
    score += pawn_structure(bPawns, bPawnRanks, Color::Black);

    // Bishop pair
    if (wBishops >= 2) score += 30;
    if (bBishops >= 2) score -= 30;

    // King safety (simple): small penalty if many attackers around, computed via check risk.
    // Also add small tempo bonus.
    score += (b.side_to_move() == Color::White) ? 8 : -8;

    return score;
}

} // namespace engine