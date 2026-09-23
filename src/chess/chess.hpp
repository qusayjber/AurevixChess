#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace chess {

using U64 = uint64_t;
using Square = int;
constexpr Square NO_SQUARE = -1;

enum class Color : uint8_t { White = 0, Black = 1 };
constexpr Color opposite(Color c) { return c == Color::White ? Color::Black : Color::White; }
constexpr int ci(Color c) { return static_cast<int>(c); }

enum class PieceType : uint8_t { None, Pawn, Knight, Bishop, Rook, Queen, King };

inline char piece_char(PieceType t) {
    switch (t) {
        case PieceType::Pawn:   return 'p';
        case PieceType::Knight: return 'n';
        case PieceType::Bishop: return 'b';
        case PieceType::Rook:   return 'r';
        case PieceType::Queen:  return 'q';
        case PieceType::King:   return 'k';
        default:                return '.';
    }
}
inline PieceType char_piece(char c) {
    switch (c) {
        case 'p': case 'P': return PieceType::Pawn;
        case 'n': case 'N': return PieceType::Knight;
        case 'b': case 'B': return PieceType::Bishop;
        case 'r': case 'R': return PieceType::Rook;
        case 'q': case 'Q': return PieceType::Queen;
        case 'k': case 'K': return PieceType::King;
        default:            return PieceType::None;
    }
}
inline int piece_value(PieceType t) {
    switch (t) {
        case PieceType::Pawn:   return 100;
        case PieceType::Knight: return 320;
        case PieceType::Bishop: return 330;
        case PieceType::Rook:   return 500;
        case PieceType::Queen:  return 900;
        case PieceType::King:   return 20000;
        default:                return 0;
    }
}

struct Piece {
    PieceType type = PieceType::None;
    Color     color = Color::White;
    bool empty() const { return type == PieceType::None; }
    bool operator==(const Piece& o) const { return type == o.type && (type == PieceType::None || color == o.color); }
    bool operator!=(const Piece& o) const { return !(*this == o); }
};

inline int file_of(Square s) { return s & 7; }
inline int rank_of(Square s) { return s >> 3; }
inline Square mk_square(int f, int r) { return r * 8 + f; }
inline bool on_board(int f, int r) { return f >= 0 && f < 8 && r >= 0 && r < 8; }
inline bool valid_square(Square s) { return s >= 0 && s < 64; }

struct Move {
    Square from = NO_SQUARE;
    Square to   = NO_SQUARE;
    PieceType promo = PieceType::None;
    bool en_passant = false;
    bool castle     = false;

    bool operator==(const Move& o) const { return from == o.from && to == o.to && promo == o.promo; }
    bool operator!=(const Move& o) const { return !(*this == o); }
    bool is_null() const { return from == NO_SQUARE; }
    static Move null() { return {}; }
};

struct CastlingRights {
    bool wk = false, wq = false, bk = false, bq = false;
};

struct UndoInfo {
    Move move;
    Piece captured;
    CastlingRights castling;
    Square ep;
    int halfmove;
    int fullmove;
    U64 hash;
};

class Board {
public:
    Board();
    void set_startpos();
    bool set_fen(const std::string& fen, std::string* err = nullptr);
    std::string to_fen() const;

    Piece at(Square s) const { return squares_[s]; }
    Color side_to_move() const { return side_; }
    Square ep_square() const { return ep_; }
    CastlingRights castling() const { return castling_; }
    int halfmove_clock() const { return halfmove_; }
    int fullmove_number() const { return fullmove_; }
    U64 hash() const { return hash_; }

    Square king_square(Color c) const;
    bool in_check(Color c) const;
    bool is_square_attacked(Square s, Color by) const;

    std::vector<Move> generate_pseudo_moves() const;
    std::vector<Move> generate_legal_moves() const;
    bool is_legal(const Move& m) const;

    bool make_move(const Move& m);
    void unmake_move();

    bool is_checkmate() const;
    bool is_stalemate() const;
    bool is_insufficient_material() const;
    bool is_fifty_move() const { return halfmove_ >= 100; }
    bool is_threefold() const;
    bool is_fifty_or_repetition() const { return is_fifty_move() || is_threefold(); }
    bool is_game_over() const;

    void make_null_move();
    void unmake_null_move();

    const std::vector<U64>& position_history() const { return pos_history_; }
    const std::vector<UndoInfo>& history() const { return undo_; }
    int move_count() const { return static_cast<int>(undo_.size()); }
    void clear_history() { undo_.clear(); pos_history_.clear(); pos_history_.push_back(hash_); }
    void set_history_hash(U64 h) { pos_history_.push_back(h); }

    // Direct editing (for FEN/board editor)
    void set_piece(Square s, Piece p);
    void set_side_to_move(Color c) { side_ = c; recompute_hash(); }
    void set_castling(CastlingRights c) { castling_ = c; recompute_hash(); }
    void set_ep(Square s) { ep_ = s; recompute_hash(); }
    void set_halfmove(int h) { halfmove_ = h; }
    void set_fullmove(int f) { fullmove_ = f; }
    void rebuild();

private:
    std::array<Piece, 64> squares_{};
    Color side_ = Color::White;
    CastlingRights castling_{};
    Square ep_ = NO_SQUARE;
    int halfmove_ = 0;
    int fullmove_ = 1;
    U64 hash_ = 0;

    std::vector<UndoInfo> undo_;
    std::vector<U64> pos_history_;
    struct NullUndo { Square ep; U64 hash; };
    std::vector<NullUndo> null_undo_;

    void recompute_hash();
    void apply_move(const Move& m, UndoInfo& info);
    void revert_move(const UndoInfo& info);

    void gen_pawn(Color c, std::vector<Move>& out) const;
    void gen_knight(Color c, std::vector<Move>& out) const;
    void gen_bishop(Color c, std::vector<Move>& out) const;
    void gen_rook(Color c, std::vector<Move>& out) const;
    void gen_queen(Color c, std::vector<Move>& out) const;
    void gen_king(Color c, std::vector<Move>& out) const;
};

// Helpers used by UI/engine
std::string square_name(Square s);
Square parse_square(const std::string& s);
std::string move_to_uci(const Move& m);
std::optional<Move> uci_to_move(const Board& b, const std::string& uci);
std::string move_to_san(const Board& b, const Move& m);
bool is_valid_fen(const std::string& fen, std::string* err = nullptr);

} // namespace chess