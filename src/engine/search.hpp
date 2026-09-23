#pragma once
#include "chess/chess.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace engine {

using chess::Board;
using chess::Move;

enum class Difficulty { Beginner, Easy, Medium, Hard, Expert, Master };

struct SearchLimits {
    int   depth       = 6;
    int   time_ms     = 3000;
    int   randomness  = 0;   // centipawns of random noise added to root scores
    int   max_nodes   = 0;   // 0 = unlimited
};

struct SearchResult {
    Move  best;
    int   score_cp = 0;
    int   depth = 0;
    uint64_t nodes = 0;
    int   time_ms = 0;
    std::vector<Move> pv;
    std::string best_san;
};

struct EngineInfo {
    int depth = 0;
    int score_cp = 0;
    uint64_t nodes = 0;
    int time_ms = 0;
    std::vector<Move> pv;
    std::string best_move_uci;
};

using InfoCallback = std::function<void(const EngineInfo&)>;

class Engine {
public:
    Engine();

    void set_difficulty(Difficulty d);
    Difficulty difficulty() const { return diff_; }
    SearchLimits limits_for(Difficulty d) const;

    // Synchronous search with optional callback.
    SearchResult search(Board& b, const SearchLimits& limits, InfoCallback cb = nullptr);

    // Cancel any running search
    void stop() { stop_.store(true); }
    void reset() { stop_.store(false); tt_.clear(); }

    // Hint: return a top-N list of candidate moves with scores + a short explanation.
    struct Hint { Move move; int score_cp; std::string explanation; };
    std::vector<Hint> hints(Board& b, int count = 3);

    // Best move only (used by "Show best move").
    std::optional<Move> best_move(Board& b, const SearchLimits& limits);

private:
    // --- Transposition table ---
    struct TTEntry {
        uint64_t hash = 0;
        int32_t  score = 0;
        int16_t  depth = -1;
        uint8_t  flag  = 0;      // 0=none, 1=exact, 2=lower, 3=upper
        uint16_t best_move = 0;  // packed as 12 bits (from/to)
    };
    static uint16_t pack_move(const Move& m);
    static Move     unpack_move(uint16_t v);
    struct TT {
        std::vector<TTEntry> table;
        TT() { table.resize(1u << 20); }
        void clear() { std::fill(table.begin(), table.end(), TTEntry{}); }
        TTEntry* probe(uint64_t h) {
            TTEntry* e = &table[h & (table.size() - 1)];
            return (e->hash == h) ? e : nullptr;
        }
        void store(uint64_t h, int depth, int score, uint8_t flag, Move m) {
            TTEntry* e = &table[h & (table.size() - 1)];
            if (e->hash != h || e->depth <= depth) {
                e->hash = h; e->depth = static_cast<int16_t>(depth); e->score = score;
                e->flag = flag; e->best_move = pack_move(m);
            }
        }
    };
    TT tt_;

    Difficulty diff_ = Difficulty::Medium;
    std::atomic<bool> stop_{false};
    uint64_t nodes_ = 0;
    std::chrono::steady_clock::time_point start_;
    int time_limit_ms_ = 0;
    uint64_t node_limit_ = 0;

    // Killer moves & history heuristic
    Move killers_[64][2];
    int  history_[2][64][64];

    int  negamax(Board& b, int depth, int alpha, int beta, int ply);
    int  quiescence(Board& b, int alpha, int beta, int ply);
    void order_moves(const Board& b, std::vector<Move>& moves, int ply, Move ttMove);
    bool time_up();

    std::string explain(const Board& before, const Move& m, int score_cp);
};

} // namespace engine