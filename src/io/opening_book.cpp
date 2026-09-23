#include "io/opening_book.hpp"
#include <algorithm>

namespace io {

namespace {
struct Entry { const char* moves; const char* eco; const char* name; };

// Small but useful opening book.  Moves are space-separated SAN.
const Entry kBook[] = {
    {"e4",                 "B00", "King's Pawn Opening"},
    {"e4 e5",              "C20", "King's Pawn Game"},
    {"e4 e5 Nf3",          "C40", "King's Knight Opening"},
    {"e4 e5 Nf3 Nc6",      "C44", "King's Knight Opening"},
    {"e4 e5 Nf3 Nc6 Bb5",  "C60", "Ruy Lopez"},
    {"e4 e5 Nf3 Nc6 Bb5 a6","C68","Ruy Lopez: Morphy Defence"},
    {"e4 e5 Nf3 Nc6 d4",   "C44", "Scotch Game"},
    {"e4 e5 Nf3 Nc6 Bc4",  "C50", "Italian Game"},
    {"e4 e5 Nf3 Nc6 Bc4 Bc5","C50","Italian Game: Giuoco Piano"},
    {"e4 e5 Nf3 Nc6 Bc4 Bc5 b4","C51","Evans Gambit"},
    {"e4 e5 Nf3 Nf6",      "C42", "Petrov Defence"},
    {"e4 e5 Nc3",          "C25", "Vienna Game"},
    {"e4 e5 f4",           "C33", "King's Gambit"},
    {"e4 c5",              "B20", "Sicilian Defence"},
    {"e4 c5 Nf3",          "B27", "Sicilian Defence"},
    {"e4 c5 Nf3 d6",       "B50", "Sicilian: Modern Variations"},
    {"e4 c5 Nf3 Nc6",      "B30", "Sicilian: Old Sicilian"},
    {"e4 c5 Nf3 e6",       "B40", "Sicilian: French Variation"},
    {"e4 c5 Nc3",          "B23", "Sicilian: Closed"},
    {"e4 e6",              "C00", "French Defence"},
    {"e4 e6 d4 d5",        "C01", "French Defence: Main line"},
    {"e4 e6 d4 d5 Nc3",    "C10", "French: Paulsen Variation"},
    {"e4 e6 d4 d5 e5",     "C02", "French: Advance Variation"},
    {"e4 c6",              "B10", "Caro-Kann Defence"},
    {"e4 c6 d4 d5 Nc3",    "B15", "Caro-Kann: Main line"},
    {"e4 c6 d4 d5 e5",     "B12", "Caro-Kann: Advance"},
    {"e4 d5",              "B01", "Scandinavian Defence"},
    {"e4 Nf6",             "B02", "Alekhine Defence"},
    {"e4 d6",              "B07", "Pirc Defence"},
    {"e4 g6",              "B06", "Modern Defence"},
    {"d4",                 "A40", "Queen's Pawn Opening"},
    {"d4 d5",              "D00", "Queen's Pawn Game"},
    {"d4 d5 c4",           "D06", "Queen's Gambit"},
    {"d4 d5 c4 e6",        "D30", "Queen's Gambit Declined"},
    {"d4 d5 c4 c6",        "D10", "Slav Defence"},
    {"d4 d5 c4 dxc4",      "D20", "Queen's Gambit Accepted"},
    {"d4 Nf6",             "A45", "Indian Game"},
    {"d4 Nf6 c4 g6",       "E60", "King's Indian Defence"},
    {"d4 Nf6 c4 g6 Nc3 Bg7","E60","King's Indian: Normal Variation"},
    {"d4 Nf6 c4 e6",       "E00", "Indian: East Indian Defence"},
    {"d4 Nf6 c4 e6 Nc3 Bb4","E20","Nimzo-Indian Defence"},
    {"d4 Nf6 c4 e6 Nf3 b6","E12","Queen's Indian Defence"},
    {"d4 f5",              "A80", "Dutch Defence"},
    {"Nf3",                "A04", "Zukertort Opening"},
    {"Nf3 d5",             "A06", "Réti Opening"},
    {"c4",                 "A10", "English Opening"},
    {"c4 e5",              "A20", "English: Reversed Sicilian"},
    {"c4 c5",              "A30", "English: Symmetrical"},
    {"c4 Nf6",             "A15", "English: Anglo-Indian"},
    {"g3",                 "A00", "Benko Opening"},
    {"b3",                 "A01", "Nimzo-Larsen Attack"},
    {"f4",                 "A02", "Bird's Opening"},
};

std::string join(const std::vector<std::string>& v, size_t n) {
    std::string s;
    for (size_t i = 0; i < n && i < v.size(); ++i) {
        if (i) s += ' ';
        s += v[i];
    }
    return s;
}

} // namespace

OpeningInfo detect_opening(const std::vector<std::string>& san_moves) {
    OpeningInfo best;
    int bestDepth = -1;

    for (const Entry& e : kBook) {
        // Tokenize
        std::vector<std::string> tokens;
        std::string cur;
        for (char c : std::string(e.moves)) {
            if (c == ' ') { if (!cur.empty()) tokens.push_back(cur); cur.clear(); }
            else cur.push_back(c);
        }
        if (!cur.empty()) tokens.push_back(cur);
        if (tokens.empty()) continue;

        if (tokens.size() > san_moves.size()) continue;
        bool match = true;
        for (size_t i = 0; i < tokens.size(); ++i) {
            if (san_moves[i] != tokens[i]) { match = false; break; }
        }
        if (!match) continue;
        if (static_cast<int>(tokens.size()) > bestDepth) {
            bestDepth = static_cast<int>(tokens.size());
            best.eco = e.eco;
            best.name = e.name;
            best.moves = e.moves;
        }
    }

    if (bestDepth < 0) {
        best.name = "Unknown Opening";
        best.eco = "A00";
    }
    // Split name on ":" to get variation
    auto pos = best.name.find(':');
    if (pos != std::string::npos) {
        best.variation = best.name.substr(pos + 1);
        while (!best.variation.empty() && best.variation.front() == ' ') best.variation.erase(best.variation.begin());
        best.name = best.name.substr(0, pos);
    }
    return best;
}

} // namespace io