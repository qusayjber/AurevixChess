#include "chess/chess.hpp"
#include <sstream>
#include <cctype>
#include <stdexcept>

namespace chess {

void Board::set_startpos() {
    set_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    undo_.clear();
    pos_history_.clear();
    pos_history_.push_back(hash_);
    null_undo_.clear();
}

bool Board::set_fen(const std::string& fen, std::string* err) {
    auto fail = [&](const std::string& m) { if (err) *err = m; return false; };

    // Clear board
    for (auto& s : squares_) s = {};
    side_ = Color::White; castling_ = {}; ep_ = NO_SQUARE; halfmove_ = 0; fullmove_ = 1;

    std::istringstream iss(fen);
    std::string placement, active, castlingStr, epStr;
    int hm = 0, fm = 1;
    if (!(iss >> placement)) return fail("Missing board placement field");
    iss >> active >> castlingStr >> epStr;
    if (iss) iss >> hm; else hm = 0;
    if (iss) iss >> fm; else fm = 1;

    // Placement
    int f = 0, r = 7;
    for (char c : placement) {
        if (c == '/') {
            if (f != 8) return fail("Each FEN rank must have 8 squares");
            f = 0; r--;
            if (r < 0) return fail("Too many ranks in FEN");
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            int n = c - '0';
            if (n < 1 || n > 8) return fail("Invalid skip count");
            f += n;
            if (f > 8) return fail("Rank overflow");
        } else {
            PieceType pt = char_piece(c);
            if (pt == PieceType::None) return fail(std::string("Invalid piece char: ") + c);
            Color col = std::isupper(static_cast<unsigned char>(c)) ? Color::White : Color::Black;
            if (f >= 8 || r < 0) return fail("Placement out of bounds");
            Piece p; p.type = pt; p.color = col;
            squares_[mk_square(f, r)] = p;
            f++;
        }
    }
    if (r != 0 || f != 8) return fail("FEN does not describe 8 ranks of 8 squares");

    // Side to move
    if (active == "w") side_ = Color::White;
    else if (active == "b") side_ = Color::Black;
    else return fail("Side-to-move must be 'w' or 'b'");

    // Castling
    if (castlingStr != "-") {
        for (char c : castlingStr) {
            switch (c) {
                case 'K': castling_.wk = true; break;
                case 'Q': castling_.wq = true; break;
                case 'k': castling_.bk = true; break;
                case 'q': castling_.bq = true; break;
                default: return fail(std::string("Invalid castling char: ") + c);
            }
        }
    }

    // En passant
    if (epStr != "-") {
        Square s = parse_square(epStr);
        if (s == NO_SQUARE) return fail("Invalid en passant square");
        ep_ = s;
    }

    halfmove_ = hm;
    fullmove_ = fm;
    recompute_hash();

    // Validate both kings present
    if (king_square(Color::White) == NO_SQUARE) return fail("No white king");
    if (king_square(Color::Black) == NO_SQUARE) return fail("No black king");
    return true;
}

std::string Board::to_fen() const {
    std::string out;
    for (int r = 7; r >= 0; --r) {
        int empty = 0;
        for (int f = 0; f < 8; ++f) {
            Piece p = squares_[mk_square(f, r)];
            if (p.empty()) { empty++; continue; }
            if (empty) { out += std::to_string(empty); empty = 0; }
            char c = piece_char(p.type);
            out += (p.color == Color::White) ? char(std::toupper(c)) : c;
        }
        if (empty) out += std::to_string(empty);
        if (r > 0) out += '/';
    }
    out += ' ';
    out += (side_ == Color::White) ? 'w' : 'b';
    out += ' ';
    std::string cs;
    if (castling_.wk) cs += 'K';
    if (castling_.wq) cs += 'Q';
    if (castling_.bk) cs += 'k';
    if (castling_.bq) cs += 'q';
    out += cs.empty() ? "-" : cs;
    out += ' ';
    out += (ep_ == NO_SQUARE) ? "-" : square_name(ep_);
    out += ' ';
    out += std::to_string(halfmove_);
    out += ' ';
    out += std::to_string(fullmove_);
    return out;
}

bool is_valid_fen(const std::string& fen, std::string* err) {
    Board b;
    return b.set_fen(fen, err);
}

} // namespace chess