#include "engine/search.hpp"
#include "engine/eval.hpp"
#include <algorithm>
#include <cmath>
#include <optional>
#include <random>
#include <utility>
#include <vector>

namespace engine {
using namespace chess;

namespace {
constexpr int INF  = 1'000'000;
constexpr int MATE = 100'000;

inline int mvv_lva(const Board& b, const Move& m) {
    Piece attacker = b.at(m.from);
    Piece victim   = b.at(m.to);
    int vv = victim.empty() ? 0 : piece_value(victim.type);
    int va = piece_value(attacker.type);
    return vv * 10 - va;
}
} // namespace

// =============================================================
// Construction
// =============================================================
Engine::Engine() {
    for (auto& k : killers_) { k[0] = Move{}; k[1] = Move{}; }
    std::fill(&history_[0][0][0], &history_[0][0][0] + 2 * 64 * 64, 0);
}

void Engine::set_difficulty(Difficulty d) {
    diff_ = d;
    tt_.clear();
}

SearchLimits Engine::limits_for(Difficulty d) const {
    SearchLimits l;
    switch (d) {
        case Difficulty::Beginner: l.depth = 1;  l.time_ms = 150;  l.randomness = 130; break;
        case Difficulty::Easy:     l.depth = 3;  l.time_ms = 400;  l.randomness = 40;  break;
        case Difficulty::Medium:   l.depth = 5;  l.time_ms = 900;  l.randomness = 10;  break;
        case Difficulty::Hard:     l.depth = 7;  l.time_ms = 1800; l.randomness = 0;   break;
        case Difficulty::Expert:   l.depth = 9;  l.time_ms = 3200; l.randomness = 0;   break;
        case Difficulty::Master:   l.depth = 12; l.time_ms = 6000; l.randomness = 0;   break;
    }
    return l;
}

// =============================================================
// Move encoding (16-bit)
// =============================================================
uint16_t Engine::pack_move(const Move& m) {
    if (m.from < 0 || m.to < 0) return 0;
    return static_cast<uint16_t>(m.from | (m.to << 6));
}

Move Engine::unpack_move(uint16_t v) {
    Move m;
    if (v == 0) return m;
    m.from = v & 63;
    m.to   = (v >> 6) & 63;
    return m;
}

// =============================================================
// Time / node limits
// =============================================================
bool Engine::time_up() {
    if (stop_.load()) return true;
    if (node_limit_ && nodes_ >= node_limit_) return true;
    if (time_limit_ms_ > 0) {
        // Check the wall clock only every 1024 nodes.
        if ((nodes_ & 1023) == 0) {
            auto now = std::chrono::steady_clock::now();
            auto el = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - start_).count();
            if (el >= time_limit_ms_) return true;
        }
    }
    return false;
}

// =============================================================
// Move ordering
// =============================================================
void Engine::order_moves(const Board& b, std::vector<Move>& moves, int ply, Move ttMove) {
    std::stable_sort(moves.begin(), moves.end(), [&](const Move& a, const Move& c) {
        int sa = 0, sc = 0;
        if (a == ttMove) sa += 1'000'000;
        if (c == ttMove) sc += 1'000'000;

        if (a.promo != PieceType::None) sa += 900'000;
        if (c.promo != PieceType::None) sc += 900'000;

        if (!b.at(a.to).empty() || a.en_passant) sa += 100'000 + mvv_lva(b, a);
        if (!b.at(c.to).empty() || c.en_passant) sc += 100'000 + mvv_lva(b, c);

        if (ply >= 0 && ply < 64) {
            if (a == killers_[ply][0]) sa += 800'000;
            if (a == killers_[ply][1]) sa += 700'000;
            if (c == killers_[ply][0]) sc += 800'000;
            if (c == killers_[ply][1]) sc += 700'000;
        }

        sa += history_[ci(b.side_to_move())][a.from][a.to];
        sc += history_[ci(b.side_to_move())][c.from][c.to];

        return sa > sc;
    });
}

// =============================================================
// Quiescence search
// =============================================================
int Engine::quiescence(Board& b, int alpha, int beta, int ply) {
    nodes_++;
    if (time_up()) return 0;

    // Stand-pat: score of side to move.
    int stand = evaluate(b);
    if (b.side_to_move() == Color::Black) stand = -stand;

    if (stand >= beta) return beta;
    if (stand > alpha) alpha = stand;
    if (ply > 32) return alpha;

    auto moves = b.generate_legal_moves();

    std::vector<Move> tactical;
    tactical.reserve(moves.size());
    for (const Move& m : moves) {
        if (!b.at(m.to).empty() || m.en_passant || m.promo != PieceType::None)
            tactical.push_back(m);
    }
    order_moves(b, tactical, -1, Move{});

    for (const Move& m : tactical) {
        // NOTE: make_move / unmake_move manage their own UndoInfo internally.
        // This local object is kept only if the caller needs to inspect it.
        // The fix here: UndoInfo is defined in namespace chess, not inside Board.
        chess::UndoInfo info;      // ← الإصلاح المطلوب
        (void)info;                // silence unused-variable warning

        if (!b.make_move(m)) continue;
        int score = -quiescence(b, -beta, -alpha, ply + 1);
        b.unmake_move();

        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    return alpha;
}

// =============================================================
// Principal search (negamax + alpha-beta)
// =============================================================
int Engine::negamax(Board& b, int depth, int alpha, int beta, int ply) {
    if (time_up()) return 0;
    nodes_++;

    if (ply > 0 && (b.is_fifty_move() || b.is_threefold())) return 0;

    // Mate-distance pruning
    int mate_alpha = std::max(alpha, -MATE + ply);
    int mate_beta  = std::min(beta,   MATE - ply - 1);
    if (mate_alpha >= mate_beta) return mate_alpha;

    // Transposition table probe
    uint64_t h = b.hash();
    TTEntry* tte = tt_.probe(h);
    Move ttMove;
    if (tte) {
        ttMove = unpack_move(tte->best_move);
        if (tte->depth >= depth) {
            int sc = tte->score;
            if (sc >  MATE - 1000) sc -= ply;
            if (sc < -MATE + 1000) sc += ply;
            if (tte->flag == 1) return sc;                    // exact
            if (tte->flag == 2 && sc >= beta)  return sc;     // lower bound
            if (tte->flag == 3 && sc <= alpha) return sc;     // upper bound
        }
    }

    bool inCheck = b.in_check(b.side_to_move());
    if (inCheck) depth++;   // check extension

    if (depth <= 0) return quiescence(b, alpha, beta, ply);

    auto moves = b.generate_legal_moves();
    if (moves.empty()) {
        if (inCheck) return -MATE + ply;
        return 0;   // stalemate
    }
    order_moves(b, moves, ply, ttMove);

    int best = -INF;
    Move bestMove;
    int origAlpha = alpha;

    for (size_t i = 0; i < moves.size(); ++i) {
        const Move& m = moves[i];
        if (!b.make_move(m)) continue;

        int score;
        if (i == 0) {
            score = -negamax(b, depth - 1, -beta, -alpha, ply + 1);
        } else {
            // Late move reduction
            int r = 0;
            if (depth >= 3 && i >= 4 && !inCheck &&
                b.at(m.to).empty() && m.promo == PieceType::None) {
                r = 1 + static_cast<int>(i / 8);
            }
            score = -negamax(b, depth - 1 - r, -alpha - 1, -alpha, ply + 1);
            if (score > alpha && score < beta) {
                score = -negamax(b, depth - 1, -beta, -alpha, ply + 1);
            }
        }
        b.unmake_move();

        if (score > best) { best = score; bestMove = m; }
        if (score > alpha) alpha = score;
        if (alpha >= beta) {
            if (b.at(m.to).empty() && m.promo == PieceType::None && ply < 64) {
                killers_[ply][1] = killers_[ply][0];
                killers_[ply][0] = m;
                history_[ci(b.side_to_move())][m.from][m.to] += depth * depth;
            }
            break;
        }
    }

    uint8_t flag = 1;
    if (best <= origAlpha) flag = 3;
    else if (best >= beta) flag = 2;

    int store_score = best;
    if (store_score >  MATE - 1000) store_score += ply;
    if (store_score < -MATE + 1000) store_score -= ply;
    tt_.store(h, depth, store_score, flag, bestMove);

    return best;
}

// =============================================================
// Iterative deepening driver
// =============================================================
SearchResult Engine::search(Board& b, const SearchLimits& limits, InfoCallback cb) {
    stop_.store(false);
    nodes_ = 0;
    start_ = std::chrono::steady_clock::now();
    time_limit_ms_ = limits.time_ms;
    node_limit_    = limits.max_nodes;

    for (auto& k : killers_) { k[0] = Move{}; k[1] = Move{}; }
    std::fill(&history_[0][0][0], &history_[0][0][0] + 2 * 64 * 64, 0);

    SearchResult result;
    auto rootMoves = b.generate_legal_moves();
    if (rootMoves.empty()) return result;

    std::mt19937 rng(0x5EED);
    std::uniform_int_distribution<int> noise(-limits.randomness, limits.randomness);

    std::vector<Move> pv;
    Move bestMove  = rootMoves.front();
    int  bestScore = 0;
    int  reached_depth = 0;      // ← جديد: تتبّع العمق الفعلي

    const int maxDepth = std::max(1, limits.depth);

    for (int d = 1; d <= maxDepth; ++d) {
        if (time_up()) break;

        order_moves(b, rootMoves, 0, bestMove);

        int alpha = -INF, beta = INF;
        Move localBest  = rootMoves.front();
        int  localScore = -INF;
        std::vector<Move> localPV;

        for (size_t i = 0; i < rootMoves.size(); ++i) {
            const Move& m = rootMoves[i];
            if (!b.make_move(m)) continue;
            int score = -negamax(b, d - 1, -beta, -alpha, 1);
            b.unmake_move();

            int adj = score + (limits.randomness ? noise(rng) : 0);
            if (adj > localScore) {
                localScore = adj;
                localBest  = m;
            }
            if (score > alpha) alpha = score;
        }

        if (localScore == -INF) break;

        bestMove    = localBest;
        bestScore   = localScore;
        reached_depth = d;         // ← سجّل العمق الذي اكتمل فعلاً

        // (PV الحالي مجرد الحركة الأفضل؛ استخراج PV كامل عبر TT ممكن لكن غير مطلوب)
        localPV.clear();
        localPV.push_back(bestMove);
        pv = localPV;

        if (cb) {
            EngineInfo info;
            info.depth = d;
            info.score_cp = bestScore;
            info.nodes = nodes_;
            info.time_ms = static_cast<int>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start_).count());
            info.pv = pv;
            info.best_move_uci = move_to_uci(bestMove);
            cb(info);
        }

        if (std::abs(bestScore) > MATE - 1000) break;   // mate found
    }

    result.best      = bestMove;
    result.score_cp  = bestScore;
    result.depth     = reached_depth;   // ← الإصلاح: العمق الذي وصلناه فعلاً
    result.nodes     = nodes_;
    result.time_ms   = static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_).count());
    result.pv        = pv;
    result.best_san  = move_to_san(b, bestMove);
    return result;
}

// =============================================================
// Convenience API
// =============================================================
std::optional<Move> Engine::best_move(Board& b, const SearchLimits& limits) {
    auto r = search(b, limits, nullptr);
    if (r.best.is_null()) return std::nullopt;
    return r.best;
}

// =============================================================
// Hint explanation generator
// =============================================================
std::string Engine::explain(const Board& before, const Move& m, int score_cp) {
    Piece moving = before.at(m.from);
    Piece victim = before.at(m.to);

    if (!victim.empty() &&
        piece_value(victim.type) >= piece_value(moving.type) + 50) {
        return "This move wins material.";
    }
    if (!victim.empty()) {
        return "Capture the piece to improve your position.";
    }
    if (m.promo != PieceType::None) {
        return "Promote your pawn to a powerful piece.";
    }

    // Does this move check (or mate) the opponent?
    Board after = before;
    after.make_move(m);
    if (after.in_check(after.side_to_move())) {
        if (after.generate_legal_moves().empty())
            return "This delivers checkmate.";
        return "This puts the opponent's king in check.";
    }

    if (moving.type == PieceType::Knight || moving.type == PieceType::Bishop) {
        if (rank_of(m.from) == (moving.color == Color::White ? 0 : 7))
            return "Develop your pieces toward the center.";
    }
    if (moving.type == PieceType::Pawn) {
        int df = std::abs(file_of(m.to) - 3);
        if (df <= 1 || std::abs(file_of(m.to) - 4) <= 1)
            return "Improve your control of the center.";
    }

    if (score_cp >  150) return "This move builds a strong advantage.";
    if (score_cp < -150) return "This is the best try in a difficult position.";
    return "This is the strongest continuation in this position.";
}

// =============================================================
// Multi-hint generator
// =============================================================
std::vector<Engine::Hint> Engine::hints(Board& b, int count) {
    SearchLimits l = limits_for(diff_);
    l.time_ms    = std::max(400, l.time_ms / 2);
    l.randomness = 0;

    stop_.store(false);
    nodes_ = 0;
    start_ = std::chrono::steady_clock::now();
    time_limit_ms_ = l.time_ms;

    auto rootMoves = b.generate_legal_moves();
    std::vector<Hint> hintsOut;
    if (rootMoves.empty()) return hintsOut;

    order_moves(b, rootMoves, 0, Move{});

    std::vector<std::pair<int, Move>> scored;
    scored.reserve(rootMoves.size());

    const int depth = std::max(2, l.depth - 1);
    int alpha = -INF, beta = INF;

    for (const Move& m : rootMoves) {
        if (!b.make_move(m)) continue;
        int score = -negamax(b, depth - 1, -beta, -alpha, 1);
        b.unmake_move();
        scored.push_back({score, m});
        if (score > alpha) alpha = score;
    }

    std::stable_sort(scored.begin(), scored.end(),
                     [](const auto& a, const auto& c){ return a.first > c.first; });

    const int take = std::min<int>(count, static_cast<int>(scored.size()));
    for (int i = 0; i < take; ++i) {
        Hint h;
        h.move = scored[i].second;
        h.score_cp = scored[i].first;
        h.explanation = explain(b, h.move, h.score_cp);
        hintsOut.push_back(h);
    }
    return hintsOut;
}

} // namespace engine