#include "chess/chess.hpp"
#include <algorithm>
#include <cctype>
#include <random>
#include <sstream>
#include <stdexcept>

namespace chess {

namespace {
struct ZobristTables {
    U64 pieces[2][7][64]{};
    U64 castling[16]{};
    U64 ep[8]{};
    U64 side = 0;
    ZobristTables() {
        std::mt19937_64 rng(0xC0FFEE1234567890ULL);
        for (int c = 0; c < 2; ++c)
            for (int p = 0; p < 7; ++p)
                for (int s = 0; s < 64; ++s)
                    pieces[c][p][s] = rng();
        for (auto& x : castling) x = rng();
        for (auto& x : ep)       x = rng();
        side = rng();
    }
};
const ZobristTables ZT;
inline int castle_bits(const CastlingRights& c) {
    return (c.wk?1:0) | (c.wq?2:0) | (c.bk?4:0) | (c.bq?8:0);
}
} // namespace

Board::Board() { set_startpos(); }

void Board::recompute_hash() {
    hash_ = 0;
    for (int s = 0; s < 64; ++s)
        if (!squares_[s].empty())
            hash_ ^= ZT.pieces[ci(squares_[s].color)][int(squares_[s].type)][s];
    hash_ ^= ZT.castling[castle_bits(castling_)];
    if (ep_ != NO_SQUARE) hash_ ^= ZT.ep[file_of(ep_)];
    if (side_ == Color::Black) hash_ ^= ZT.side;
}
void Board::rebuild() { recompute_hash(); }

void Board::set_piece(Square s, Piece p) { squares_[s] = p; }

Square Board::king_square(Color c) const {
    for (int s = 0; s < 64; ++s)
        if (squares_[s].type == PieceType::King && squares_[s].color == c) return s;
    return NO_SQUARE;
}

bool Board::is_square_attacked(Square s, Color by) const {
    const int f = file_of(s), r = rank_of(s);

    // Pawn attacks
    if (by == Color::White) {
        if (r > 0) {
            if (f > 0) { Piece p = squares_[mk_square(f-1, r-1)]; if (p.type==PieceType::Pawn && p.color==by) return true; }
            if (f < 7) { Piece p = squares_[mk_square(f+1, r-1)]; if (p.type==PieceType::Pawn && p.color==by) return true; }
        }
    } else {
        if (r < 7) {
            if (f > 0) { Piece p = squares_[mk_square(f-1, r+1)]; if (p.type==PieceType::Pawn && p.color==by) return true; }
            if (f < 7) { Piece p = squares_[mk_square(f+1, r+1)]; if (p.type==PieceType::Pawn && p.color==by) return true; }
        }
    }

    // Knights
    static const int kn[8][2] = {{1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1},{-2,1},{-1,2}};
    for (auto& d : kn) {
        int nf = f + d[0], nr = r + d[1];
        if (!on_board(nf,nr)) continue;
        Piece p = squares_[mk_square(nf,nr)];
        if (p.type == PieceType::Knight && p.color == by) return true;
    }
    // King
    for (int df=-1; df<=1; ++df) for (int dr=-1; dr<=1; ++dr) {
        if (!df && !dr) continue;
        int nf = f+df, nr = r+dr;
        if (!on_board(nf,nr)) continue;
        Piece p = squares_[mk_square(nf,nr)];
        if (p.type == PieceType::King && p.color == by) return true;
    }
    // Diagonals
    static const int diag[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
    for (auto& d : diag) {
        int nf = f+d[0], nr = r+d[1];
        while (on_board(nf,nr)) {
            Piece p = squares_[mk_square(nf,nr)];
            if (!p.empty()) {
                if (p.color==by && (p.type==PieceType::Bishop || p.type==PieceType::Queen)) return true;
                break;
            }
            nf += d[0]; nr += d[1];
        }
    }
    // Orthogonals
    static const int orth[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
    for (auto& d : orth) {
        int nf = f+d[0], nr = r+d[1];
        while (on_board(nf,nr)) {
            Piece p = squares_[mk_square(nf,nr)];
            if (!p.empty()) {
                if (p.color==by && (p.type==PieceType::Rook || p.type==PieceType::Queen)) return true;
                break;
            }
            nf += d[0]; nr += d[1];
        }
    }
    return false;
}

bool Board::in_check(Color c) const {
    Square k = king_square(c);
    if (k == NO_SQUARE) return false;
    return is_square_attacked(k, opposite(c));
}

void Board::gen_pawn(Color c, std::vector<Move>& out) const {
    const int dir = (c == Color::White) ? 1 : -1;
    const int startRank = (c == Color::White) ? 1 : 6;
    const int promoRank = (c == Color::White) ? 7 : 0;

    for (int s = 0; s < 64; ++s) {
        Piece p = squares_[s];
        if (p.type != PieceType::Pawn || p.color != c) continue;
        int f = file_of(s), r = rank_of(s);

        // forward
        int nr = r + dir;
        if (on_board(f, nr)) {
            Square to = mk_square(f, nr);
            if (squares_[to].empty()) {
                if (nr == promoRank) {
                    for (PieceType pt : {PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight}) {
                        Move m; m.from=s; m.to=to; m.promo=pt; out.push_back(m);
                    }
                } else {
                    Move m; m.from=s; m.to=to; out.push_back(m);
                    if (r == startRank) {
                        int nr2 = r + 2*dir;
                        Square to2 = mk_square(f, nr2);
                        if (squares_[to2].empty()) { Move m2; m2.from=s; m2.to=to2; out.push_back(m2); }
                    }
                }
            }
        }
        // captures
        for (int df : {-1, 1}) {
            int nf = f + df;
            int nnr = r + dir;
            if (!on_board(nf, nnr)) continue;
            Square to = mk_square(nf, nnr);
            Piece target = squares_[to];
            bool enemy = !target.empty() && target.color != c;
            bool epCap = (to == ep_);
            if (enemy || epCap) {
                if (nnr == promoRank) {
                    for (PieceType pt : {PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight}) {
                        Move m; m.from=s; m.to=to; m.promo=pt; m.en_passant=epCap; out.push_back(m);
                    }
                } else {
                    Move m; m.from=s; m.to=to; m.en_passant=epCap; out.push_back(m);
                }
            }
        }
    }
}

void Board::gen_knight(Color c, std::vector<Move>& out) const {
    static const int kn[8][2] = {{1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1},{-2,1},{-1,2}};
    for (int s = 0; s < 64; ++s) {
        Piece p = squares_[s];
        if (p.type != PieceType::Knight || p.color != c) continue;
        int f = file_of(s), r = rank_of(s);
        for (auto& d : kn) {
            int nf = f+d[0], nr = r+d[1];
            if (!on_board(nf,nr)) continue;
            Square to = mk_square(nf,nr);
            Piece t = squares_[to];
            if (!t.empty() && t.color == c) continue;
            Move m; m.from=s; m.to=to; out.push_back(m);
        }
    }
}

static void sliding_moves(const Board& b, Square s, Color c, const int dirs[][2], int nd, std::vector<Move>& out) {
    int f = file_of(s), r = rank_of(s);
    for (int i = 0; i < nd; ++i) {
        int nf = f + dirs[i][0], nr = r + dirs[i][1];
        while (on_board(nf,nr)) {
            Square to = mk_square(nf,nr);
            Piece t = b.at(to);
            if (t.empty()) { Move m; m.from=s; m.to=to; out.push_back(m); }
            else {
                if (t.color != c) { Move m; m.from=s; m.to=to; out.push_back(m); }
                break;
            }
            nf += dirs[i][0]; nr += dirs[i][1];
        }
    }
}

void Board::gen_bishop(Color c, std::vector<Move>& out) const {
    static const int dirs[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
    for (int s = 0; s < 64; ++s) {
        Piece p = squares_[s];
        if (p.type != PieceType::Bishop || p.color != c) continue;
        sliding_moves(*this, s, c, dirs, 4, out);
    }
}
void Board::gen_rook(Color c, std::vector<Move>& out) const {
    static const int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
    for (int s = 0; s < 64; ++s) {
        Piece p = squares_[s];
        if (p.type != PieceType::Rook || p.color != c) continue;
        sliding_moves(*this, s, c, dirs, 4, out);
    }
}
void Board::gen_queen(Color c, std::vector<Move>& out) const {
    static const int dirs[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
    for (int s = 0; s < 64; ++s) {
        Piece p = squares_[s];
        if (p.type != PieceType::Queen || p.color != c) continue;
        sliding_moves(*this, s, c, dirs, 8, out);
    }
}

void Board::gen_king(Color c, std::vector<Move>& out) const {
    for (int s = 0; s < 64; ++s) {
        Piece p = squares_[s];
        if (p.type != PieceType::King || p.color != c) continue;
        int f = file_of(s), r = rank_of(s);
        for (int df=-1; df<=1; ++df) for (int dr=-1; dr<=1; ++dr) {
            if (!df && !dr) continue;
            int nf = f+df, nr = r+dr;
            if (!on_board(nf,nr)) continue;
            Square to = mk_square(nf,nr);
            Piece t = squares_[to];
            if (!t.empty() && t.color == c) continue;
            Move m; m.from=s; m.to=to; out.push_back(m);
        }
        // Castling
        if (c == Color::White && s == 4) {
            if (castling_.wk &&
                squares_[5].empty() && squares_[6].empty() &&
                squares_[7].type == PieceType::Rook && squares_[7].color == Color::White &&
                !is_square_attacked(4, Color::Black) &&
                !is_square_attacked(5, Color::Black) &&
                !is_square_attacked(6, Color::Black)) {
                Move m; m.from=4; m.to=6; m.castle=true; out.push_back(m);
            }
            if (castling_.wq &&
                squares_[3].empty() && squares_[2].empty() && squares_[1].empty() &&
                squares_[0].type == PieceType::Rook && squares_[0].color == Color::White &&
                !is_square_attacked(4, Color::Black) &&
                !is_square_attacked(3, Color::Black) &&
                !is_square_attacked(2, Color::Black)) {
                Move m; m.from=4; m.to=2; m.castle=true; out.push_back(m);
            }
        }
        if (c == Color::Black && s == 60) {
            if (castling_.bk &&
                squares_[61].empty() && squares_[62].empty() &&
                squares_[63].type == PieceType::Rook && squares_[63].color == Color::Black &&
                !is_square_attacked(60, Color::White) &&
                !is_square_attacked(61, Color::White) &&
                !is_square_attacked(62, Color::White)) {
                Move m; m.from=60; m.to=62; m.castle=true; out.push_back(m);
            }
            if (castling_.bq &&
                squares_[59].empty() && squares_[58].empty() && squares_[57].empty() &&
                squares_[56].type == PieceType::Rook && squares_[56].color == Color::Black &&
                !is_square_attacked(60, Color::White) &&
                !is_square_attacked(59, Color::White) &&
                !is_square_attacked(58, Color::White)) {
                Move m; m.from=60; m.to=58; m.castle=true; out.push_back(m);
            }
        }
    }
}

std::vector<Move> Board::generate_pseudo_moves() const {
    std::vector<Move> out;
    out.reserve(64);
    Color c = side_;
    gen_pawn(c, out);
    gen_knight(c, out);
    gen_bishop(c, out);
    gen_rook(c, out);
    gen_queen(c, out);
    gen_king(c, out);
    return out;
}

std::vector<Move> Board::generate_legal_moves() const {
    std::vector<Move> pseudo = generate_pseudo_moves();
    std::vector<Move> legal;
    legal.reserve(pseudo.size());
    for (const Move& m : pseudo) if (is_legal(m)) legal.push_back(m);
    return legal;
}

bool Board::is_legal(const Move& m) const {
    // Make a temporary copy of the parts we need to check legality
    // (no allocation, no state change on `this`)
    Board& self = const_cast<Board&>(*this);
    UndoInfo info;
    self.apply_move(m, info);
    bool illegal = self.in_check(opposite(self.side_)); // side_ has been flipped by apply
    self.revert_move(info);
    return !illegal;
}

void Board::apply_move(const Move& m, UndoInfo& info) {
    info.move = m;
    info.castling = castling_;
    info.ep = ep_;
    info.halfmove = halfmove_;
    info.fullmove = fullmove_;
    info.hash = hash_;
    info.captured = Piece{};

    Piece moving = squares_[m.from];
    Color us = moving.color;
    Color them = opposite(us);

    // Clear old ep from hash
    if (ep_ != NO_SQUARE) hash_ ^= ZT.ep[file_of(ep_)];
    ep_ = NO_SQUARE;

    // Remove captured piece
    if (m.en_passant) {
        Square capSq = mk_square(file_of(m.to), rank_of(m.from));
        info.captured = squares_[capSq];
        hash_ ^= ZT.pieces[ci(them)][int(PieceType::Pawn)][capSq];
        squares_[capSq] = {};
    } else if (!squares_[m.to].empty()) {
        info.captured = squares_[m.to];
        hash_ ^= ZT.pieces[ci(info.captured.color)][int(info.captured.type)][m.to];
    }

    // Move the piece
    hash_ ^= ZT.pieces[ci(us)][int(moving.type)][m.from];
    hash_ ^= ZT.pieces[ci(us)][int(moving.type)][m.to];
    squares_[m.to] = moving;
    squares_[m.from] = {};

    // Promotion
    if (m.promo != PieceType::None) {
        hash_ ^= ZT.pieces[ci(us)][int(PieceType::Pawn)][m.to];
        hash_ ^= ZT.pieces[ci(us)][int(m.promo)][m.to];
        squares_[m.to].type = m.promo;
    }

    // Castling: move rook
    if (m.castle) {
        if (m.to == 6)  { squares_[5] = squares_[7]; squares_[7] = {}; hash_ ^= ZT.pieces[ci(us)][int(PieceType::Rook)][7] ^ ZT.pieces[ci(us)][int(PieceType::Rook)][5]; }
        if (m.to == 2)  { squares_[3] = squares_[0]; squares_[0] = {}; hash_ ^= ZT.pieces[ci(us)][int(PieceType::Rook)][0] ^ ZT.pieces[ci(us)][int(PieceType::Rook)][3]; }
        if (m.to == 62) { squares_[61] = squares_[63]; squares_[63] = {}; hash_ ^= ZT.pieces[ci(us)][int(PieceType::Rook)][63] ^ ZT.pieces[ci(us)][int(PieceType::Rook)][61]; }
        if (m.to == 58) { squares_[59] = squares_[56]; squares_[56] = {}; hash_ ^= ZT.pieces[ci(us)][int(PieceType::Rook)][56] ^ ZT.pieces[ci(us)][int(PieceType::Rook)][59]; }
    }

    // En passant target (only if double push)
    if (moving.type == PieceType::Pawn) {
        int diff = rank_of(m.to) - rank_of(m.from);
        if (diff == 2 || diff == -2) {
            Square epTarget = mk_square(file_of(m.from), (rank_of(m.from) + rank_of(m.to)) / 2);
            ep_ = epTarget;
            hash_ ^= ZT.ep[file_of(ep_)];
        }
    }

    // Update castling rights
    CastlingRights old = castling_;
    if (moving.type == PieceType::King) {
        if (us == Color::White) { castling_.wk = castling_.wq = false; }
        else                    { castling_.bk = castling_.bq = false; }
    }
    if (m.from == 7  || m.to == 7)  castling_.wk = false;
    if (m.from == 0  || m.to == 0)  castling_.wq = false;
    if (m.from == 63 || m.to == 63) castling_.bk = false;
    if (m.from == 56 || m.to == 56) castling_.bq = false;
    if (castle_bits(old) != castle_bits(castling_)) {
        hash_ ^= ZT.castling[castle_bits(old)];
        hash_ ^= ZT.castling[castle_bits(castling_)];
    }

    // Halfmove clock
    if (moving.type == PieceType::Pawn || !info.captured.empty()) halfmove_ = 0;
    else halfmove_++;

    if (us == Color::Black) fullmove_++;

    // Flip side
    side_ = them;
    hash_ ^= ZT.side;
}

void Board::revert_move(const UndoInfo& info) {
    const Move& m = info.move;
    Color us = opposite(side_); // side_ is currently "them"
    Color them = side_;
    side_ = us;
    hash_ ^= ZT.side;

    // Move piece back
    Piece moving = squares_[m.to];
    if (m.promo != PieceType::None) moving.type = PieceType::Pawn;
    squares_[m.from] = moving;
    squares_[m.to] = {};

    // Restore captured
    if (m.en_passant) {
        Square capSq = mk_square(file_of(m.to), rank_of(m.from));
        squares_[capSq] = info.captured;
    } else if (!info.captured.empty()) {
        squares_[m.to] = info.captured;
    }

    // Undo castling rook move
    if (m.castle) {
        if (m.to == 6)  { squares_[7] = squares_[5]; squares_[5] = {}; }
        if (m.to == 2)  { squares_[0] = squares_[3]; squares_[3] = {}; }
        if (m.to == 62) { squares_[63] = squares_[61]; squares_[61] = {}; }
        if (m.to == 58) { squares_[56] = squares_[59]; squares_[59] = {}; }
    }

    castling_ = info.castling;
    ep_ = info.ep;
    halfmove_ = info.halfmove;
    fullmove_ = info.fullmove;
    hash_ = info.hash;
    (void)them;
}

bool Board::make_move(const Move& m) {
    if (!is_legal(m)) return false;
    UndoInfo info;
    apply_move(m, info);
    undo_.push_back(info);
    pos_history_.push_back(hash_);
    return true;
}

void Board::unmake_move() {
    if (undo_.empty()) return;
    UndoInfo info = undo_.back();
    undo_.pop_back();
    if (!pos_history_.empty()) pos_history_.pop_back();
    revert_move(info);
}

void Board::make_null_move() {
    NullUndo nd; nd.ep = ep_; nd.hash = hash_;
    null_undo_.push_back(nd);
    if (ep_ != NO_SQUARE) hash_ ^= ZT.ep[file_of(ep_)];
    ep_ = NO_SQUARE;
    side_ = opposite(side_);
    hash_ ^= ZT.side;
    halfmove_++;
}
void Board::unmake_null_move() {
    if (null_undo_.empty()) return;
    NullUndo nd = null_undo_.back();
    null_undo_.pop_back();
    side_ = opposite(side_);
    ep_ = nd.ep;
    hash_ = nd.hash;
    halfmove_--;
}

bool Board::is_checkmate() const {
    if (!in_check(side_)) return false;
    return generate_legal_moves().empty();
}
bool Board::is_stalemate() const {
    if (in_check(side_)) return false;
    return generate_legal_moves().empty();
}
bool Board::is_insufficient_material() const {
    int wB = 0, wN = 0, bB = 0, bN = 0;
    int others = 0;
    std::vector<Square> bishopSquares;
    for (int s = 0; s < 64; ++s) {
        Piece p = squares_[s];
        if (p.empty() || p.type == PieceType::King) continue;
        switch (p.type) {
            case PieceType::Pawn: case PieceType::Rook: case PieceType::Queen: others++; break;
            case PieceType::Bishop: if (p.color==Color::White) wB++; else bB++; bishopSquares.push_back(s); break;
            case PieceType::Knight: if (p.color==Color::White) wN++; else bN++; break;
            default: break;
        }
    }
    if (others) return false;
    int total = wB + wN + bB + bN;
    if (total == 0) return true;                                    // K vs K
    if (total == 1) return true;                                    // K+minor vs K
    if (wB == 1 && bB == 1 && wN == 0 && bN == 0) {                 // same-color bishops
        auto color_of = [](Square s) { return (file_of(s) + rank_of(s)) & 1; };
        if (bishopSquares.size() == 2 && color_of(bishopSquares[0]) == color_of(bishopSquares[1])) return true;
    }
    return false;
}
bool Board::is_threefold() const {
    U64 h = hash_;
    int count = 0;
    // Only count positions with same side to move and castling/ep (already in hash)
    for (U64 ph : pos_history_) if (ph == h) count++;
    return count >= 3;
}
bool Board::is_game_over() const {
    auto lm = generate_legal_moves();
    if (lm.empty()) return true;
    if (is_fifty_move() || is_threefold() || is_insufficient_material()) return true;
    return false;
}

// ---------- String helpers ----------
std::string square_name(Square s) {
    if (!valid_square(s)) return "-";
    std::string r;
    r += char('a' + file_of(s));
    r += char('1' + rank_of(s));
    return r;
}
Square parse_square(const std::string& s) {
    if (s.size() < 2) return NO_SQUARE;
    int f = s[0] - 'a';
    int r = s[1] - '1';
    if (!on_board(f, r)) return NO_SQUARE;
    return mk_square(f, r);
}
std::string move_to_uci(const Move& m) {
    std::string s = square_name(m.from) + square_name(m.to);
    if (m.promo != PieceType::None) s += piece_char(m.promo);
    return s;
}
std::optional<Move> uci_to_move(const Board& b, const std::string& uci) {
    auto moves = b.generate_legal_moves();
    std::string u = uci;
    std::transform(u.begin(), u.end(), u.begin(), [](unsigned char c){ return std::tolower(c); });
    for (const Move& m : moves) if (move_to_uci(m) == u) return m;
    return std::nullopt;
}
std::string move_to_san(const Board& b, const Move& m) {
    Board copy = b;
    Piece moving = copy.at(m.from);
    if (moving.empty()) return move_to_uci(m);
    bool capture = !copy.at(m.to).empty() || m.en_passant;

    // Disambiguation
    std::string disamb;
    if (moving.type != PieceType::Pawn && moving.type != PieceType::King) {
        auto lm = copy.generate_legal_moves();
        bool needFile = false, needRank = false, ambiguous = false;
        for (const Move& other : lm) {
            if (other.to != m.to) continue;
            if (other.from == m.from) continue;
            Piece op = copy.at(other.from);
            if (op.type != moving.type || op.color != moving.color) continue;
            ambiguous = true;
            if (file_of(other.from) == file_of(m.from)) needRank = true;
            if (rank_of(other.from) == rank_of(m.from)) needFile = true;
        }
        if (ambiguous) {
            if (needFile) disamb += char('a' + file_of(m.from));
            if (needRank) disamb += char('1' + rank_of(m.from));
            if (!needFile && !needRank) disamb += char('a' + file_of(m.from));
        }
    }

    std::string san;
    if (m.castle) {
        san = (m.to == 6 || m.to == 62) ? "O-O" : "O-O-O";
    } else if (moving.type == PieceType::Pawn) {
        if (capture) san += char('a' + file_of(m.from));
        if (capture) san += 'x';
        san += square_name(m.to);
        if (m.promo != PieceType::None) {
            san += '=';
            san += char(std::toupper(piece_char(m.promo)));
        }
    } else {
        san += char(std::toupper(piece_char(moving.type)));
        san += disamb;
        if (capture) san += 'x';
        san += square_name(m.to);
    }

    // Check/checkmate
    Board after = b;
    after.make_move(m);
    if (after.in_check(after.side_to_move())) {
        if (after.generate_legal_moves().empty()) san += '#';
        else san += '+';
    }
    return san;
}

} // namespace chess