#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "chess/chess.hpp"
#include "engine/search.hpp"
#include "io/opening_book.hpp"
#include "io/persistence.hpp"
#include "ui/theme.hpp"
#include "ui/render.hpp"
#include "ui/sound.hpp"
#include "ui/menu.hpp"          // ← مُضاف: لتعريف Button
#include <atomic>
#include <future>
#include <optional>
#include <vector>
#include <string>               // ← مُضاف صراحةً للوضوح
#include <chrono>               // ← مُضاف: Clock يستخدم steady_clock
#include <utility>              // ← مُضاف: std::pair

namespace ui {

enum class Screen {
    MainMenu, PlayMode, Difficulty, TimeControl, Game, Settings, Statistics,
    Analysis, About, Puzzle, FenDialog, PromotionDialog, GameOver, Pause,
    SaveLoad, OpeningInfo, ColorChoice
};

enum class GameMode { PvP, PvC, CvC, Practice, Puzzle, Analysis };

struct PieceAnimation {
    chess::Move move;
    float t = 0.f;              // 0..1
    float duration = 0.18f;
    chess::Piece piece;
    chess::Piece captured;
    chess::Square capSq = chess::NO_SQUARE;
    bool from_has_king_after = false;
};

struct Clock {
    int time_ms = 600000;
    int increment_ms = 0;
    int remaining_ms[2] = {600000, 600000};
    bool running = false;
    bool running_for_white = true;
    std::chrono::steady_clock::time_point last;

    void reset(int base_ms, int inc_ms) {
        time_ms = base_ms; increment_ms = inc_ms;
        remaining_ms[0] = remaining_ms[1] = base_ms;
        running = false;
    }
    void start(chess::Color c) {
        running_for_white = (c == chess::Color::White);
        running = true;
        last = std::chrono::steady_clock::now();
    }
    void tick() {
        if (!running) return;
        auto now = std::chrono::steady_clock::now();
        int dt = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count());
        last = now;
        int& r = remaining_ms[running_for_white ? 0 : 1];
        r -= dt;
        if (r < 0) r = 0;
    }
    void switch_turn(chess::Color next, bool add_inc) {
        if (add_inc) {
            int& prev = remaining_ms[running_for_white ? 0 : 1];
            prev += increment_ms;
        }
        running_for_white = (next == chess::Color::White);
        last = std::chrono::steady_clock::now();
    }
    void stop() { running = false; }
};

class App {
public:
    App();
    int run();

private:
    // ---------- Window / assets ----------
    sf::RenderWindow window_;
    sf::Font font_;
    sf::Font font_bold_;
    bool fonts_ok_ = false;
    AppTheme theme_;
    Palette palette_;
    BoardTheme board_theme_ = BoardTheme::Classic;

    // ---------- Screens ----------
    Screen screen_ = Screen::MainMenu;
    Screen prev_screen_ = Screen::MainMenu;
    GameMode mode_ = GameMode::PvP;
    engine::Difficulty difficulty_ = engine::Difficulty::Medium;
    bool human_plays_white_ = true;
    bool board_flipped_ = false;

    // ---------- Chess state ----------
    chess::Board board_;
    BoardView view_;
    ui::SoundManager sound_;
    io::Settings settings_;
    io::Statistics stats_;

    // ---------- Clocks ----------
    Clock clock_;
    bool use_clock_ = false;

    // ---------- Interaction ----------
    chess::Square selected_ = chess::NO_SQUARE;
    std::vector<chess::Move> legal_from_selected_;
    std::optional<chess::Move> drag_move_;
    sf::Vector2f drag_pos_;
    chess::Square last_from_ = chess::NO_SQUARE;
    chess::Square last_to_   = chess::NO_SQUARE;

    // ---------- Animation ----------
    std::optional<PieceAnimation> animation_;
    float pulse_ = 0.f;

    // ---------- Hint state ----------
    bool hint_active_ = false;
    chess::Square hint_from_ = chess::NO_SQUARE;
    chess::Square hint_to_   = chess::NO_SQUARE;
    std::string hint_text_;

    // ---------- Engine state ----------
    std::future<engine::SearchResult> engine_future_;
    std::atomic<bool> engine_busy_{false};
    bool engine_thinking_ = false;
    engine::SearchResult last_result_;
    bool show_engine_panel_ = false;

    // ---------- Move history ----------
    struct HistMove {
        chess::Move move;
        std::string san;
        std::string uci;
        chess::Piece captured;
        std::string fen_after;
    };
    std::vector<HistMove> history_;
    int view_index_ = 0; // index into history_ (-1 = start position)

    // ---------- Redo stack ----------
    struct RedoItem { chess::Move move; };
    std::vector<RedoItem> redo_;

    // ---------- Promotion ----------
    std::optional<chess::Move> pending_promo_;
    chess::Square promo_from_ = chess::NO_SQUARE;
    chess::Square promo_to_   = chess::NO_SQUARE;

    // ---------- FEN dialog ----------
    std::string fen_buffer_;
    bool fen_focus_ = false;
    std::string fen_error_;

    // ❌ REMOVED: current_buttons_ (كان نوعه خاطئاً وغير مستخدم)

    // ---------- Puzzle ----------
    struct Puzzle { std::string fen; std::string best_move_uci; std::string theme; };
    std::vector<Puzzle> puzzles_;
    int puzzle_index_ = 0;
    int puzzle_attempts_ = 0;

    // ---------- Persistent ----------
    std::string current_opening_;
    std::string last_san_list_;

    // ============================================================
    // Helpers
    // ============================================================
    void init_fonts();
    void build_theme();
    void reset_game();
    void new_game(GameMode mode, engine::Difficulty diff, bool human_white);

    // ---------- Screens ----------
    void draw_main_menu();
    void draw_play_mode_menu();
    void draw_difficulty_menu();
    void draw_time_control_menu();
    void draw_color_choice_menu();
    void draw_settings();
    void draw_statistics();
    void draw_about();
    void draw_game_screen();
    void draw_analysis_screen();
    void draw_promotion_dialog();
    void draw_fen_dialog();
    void draw_game_over();
    void draw_pause();

    // ---------- Events ----------
    void handle_event(const sf::Event& e);
    void handle_mouse_press(sf::Vector2f p);
    void handle_mouse_release(sf::Vector2f p);
    void handle_key_press(sf::Keyboard::Key k, bool ctrl);

    // ---------- Input to chess ----------
    void try_select(chess::Square s);
    void try_move(chess::Square to);
    void commit_move(const chess::Move& m);
    void maybe_start_engine_move();
    void do_hint();
    void do_show_best();
    void undo();
    void redo();
    void goto_move(int index);

    // ---------- Drawing ----------
    void draw_buttons(const std::vector<Button>& buttons, sf::Vector2f mouse);
    void draw_panel(sf::FloatRect r, sf::Color c, bool outlined = true);
    void draw_text(const std::string& s, sf::Vector2f p, unsigned size, sf::Color c,
                   bool bold = false, int align = 0 /*0=left, 1=center, 2=right*/);
    void draw_text_wrapped(const std::string& s, sf::FloatRect box, unsigned size, sf::Color c);

    // ---------- Layout ----------
    void layout();
    sf::FloatRect board_rect_;
    sf::FloatRect side_panel_rect_;
    sf::FloatRect top_bar_rect_;

    // ---------- Opening / move helpers ----------
    std::vector<std::string> san_move_list() const;
    void refresh_opening();

    // ---------- Puzzle ----------
    void load_puzzle(int idx);
    void check_puzzle_result();

    // ---------- PGN ----------
    std::string export_pgn() const;

    // ---------- Misc ----------
    std::string clipboard_;    // in-memory clipboard for FEN copy/paste

    bool is_human_turn() const;
    std::string result_string_;
    bool game_over_flag_ = false;
};

} // namespace ui