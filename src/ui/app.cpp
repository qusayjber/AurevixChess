#include "ui/app.hpp"
#include "ui/pieces.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

namespace ui {
using namespace chess;

namespace {
constexpr float SIDE_PANEL_W_MIN = 340.f;
constexpr float SIDE_PANEL_W_MAX = 420.f;
constexpr float PAD = 16.f;
constexpr float BTN_H = 40.f;

std::string piece_name(PieceType t) {
    switch (t) {
        case PieceType::Pawn:   return "Pawn";
        case PieceType::Knight: return "Knight";
        case PieceType::Bishop: return "Bishop";
        case PieceType::Rook:   return "Rook";
        case PieceType::Queen:  return "Queen";
        case PieceType::King:   return "King";
        default:                return "";
    }
}
int piece_count(const Board& b, Color c, PieceType t) {
    int n = 0;
    for (int s = 0; s < 64; ++s) {
        auto p = b.at(s);
        if (p.color == c && p.type == t && !p.empty()) n++;
    }
    return n;
}
} // namespace

// =============================================================
// Construction
// =============================================================
App::App() : window_(sf::VideoMode(1280, 800), "Chess", sf::Style::Default) {
    window_.setVerticalSyncEnabled(true);
    window_.setKeyRepeatEnabled(false);
    settings_ = io::load_settings();
    stats_    = io::load_stats();
    board_theme_ = BoardTheme::Classic;
    show_engine_panel_ = settings_.show_engine_panel;
    sound_.set_enabled(settings_.sound_enabled);
    sound_.set_volume(settings_.sound_volume);
    init_fonts();
    build_theme();
    layout();
    reset_game();

    // Seed puzzle list
    puzzles_ = {
        {"r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1", "f3g5", "Fork"},
        {"rnbqkbnr/pppp1ppp/8/4p3/6P1/5P2/PPPPP2P/RNBQKBNR b KQkq g3 0 2",   "d8h4", "Back-rank mate"},
        {"r1bqk2r/pppp1ppp/2n2n2/2b1p3/2B1P3/2N2N2/PPPP1PPP/R1BQK2R w KQkq - 0 5", "c4f7", "Sacrifice"},
        {"6k1/5ppp/8/8/8/8/5PPP/R5K1 w - - 0 1", "a1a8", "Back-rank mate"},
        {"6k1/5ppp/8/8/8/8/8/R6K w - - 0 1",    "a1a8", "Back-rank mate"},
    };
}

void App::init_fonts() {
    std::vector<std::string> candidates = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/Library/Fonts/Arial.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "assets/DejaVuSans.ttf",
    };
    for (const auto& p : candidates) {
        if (font_.loadFromFile(p)) { fonts_ok_ = true; break; }
    }
    if (fonts_ok_) {
        std::vector<std::string> boldCandidates = {
            "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
            "/System/Library/Fonts/HelveticaNeue.ttc",
            "C:/Windows/Fonts/segoeuib.ttf",
            "C:/Windows/Fonts/arialbd.ttf",
        };
        for (const auto& p : boldCandidates) {
            if (font_bold_.loadFromFile(p)) break;
        }
    }
}

void App::build_theme() {
    theme_ = app_theme(settings_.dark_mode);
    if      (settings_.board_theme == "Classic")    board_theme_ = BoardTheme::Classic;
    else if (settings_.board_theme == "Midnight")   board_theme_ = BoardTheme::Midnight;
    else if (settings_.board_theme == "Ivory")      board_theme_ = BoardTheme::Ivory;
    else if (settings_.board_theme == "Emerald")    board_theme_ = BoardTheme::Emerald;
    else if (settings_.board_theme == "Graphite")   board_theme_ = BoardTheme::Graphite;
    else if (settings_.board_theme == "Tournament") board_theme_ = BoardTheme::Tournament;
    palette_ = palette_for(board_theme_);
}

// =============================================================
// Game lifecycle
// =============================================================
void App::reset_game() {
    board_.set_startpos();
    board_.clear_history();
    history_.clear();
    redo_.clear();
    view_index_ = 0;
    selected_ = NO_SQUARE;
    legal_from_selected_.clear();
    drag_move_.reset();
    last_from_ = last_to_ = NO_SQUARE;
    hint_active_ = false;
    game_over_flag_ = false;
    result_string_.clear();
    current_opening_.clear();

    // Preserve the time control that was configured earlier.
    clock_.remaining_ms[0] = clock_.remaining_ms[1] = clock_.time_ms;
    clock_.running = false;
    if (use_clock_ && clock_.time_ms > 0) {
        clock_.start(board_.side_to_move());
    }
}

void App::new_game(GameMode mode, engine::Difficulty diff, bool human_white) {
    mode_ = mode;
    difficulty_ = diff;
    human_plays_white_ = human_white;
    board_flipped_ = (mode == GameMode::PvC && !human_white);
    reset_game();
    show_engine_panel_ = settings_.show_engine_panel;
    maybe_start_engine_move();
}

bool App::is_human_turn() const {
    if (mode_ == GameMode::PvP || mode_ == GameMode::Analysis || mode_ == GameMode::Puzzle) return true;
    if (mode_ == GameMode::CvC) return false;
    if (mode_ == GameMode::Practice) return true;
    bool whiteTurn = (board_.side_to_move() == Color::White);
    return whiteTurn == human_plays_white_;
}

void App::maybe_start_engine_move() {
    if (mode_ == GameMode::CvC || (mode_ == GameMode::PvC && !is_human_turn())) {
        if (game_over_flag_ || board_.is_game_over()) return;
        if (engine_busy_.load()) return;

        Board copy = board_;
        auto eng = std::make_unique<engine::Engine>();
        eng->set_difficulty(difficulty_);
        auto limits = eng->limits_for(difficulty_);

        engine_busy_ = true;
        engine_thinking_ = true;
        engine_future_ = std::async(std::launch::async,
            [eng = std::move(eng), copy, limits]() mutable {
                return eng->search(copy, limits, nullptr);
            });
    }
}

// =============================================================
// Layout
// =============================================================
void App::layout() {
    auto size = window_.getSize();
    float W = static_cast<float>(size.x), H = static_cast<float>(size.y);
    float panelW = std::clamp(W * 0.32f, SIDE_PANEL_W_MIN, SIDE_PANEL_W_MAX);
    float barH = 52.f;
    top_bar_rect_ = {0.f, 0.f, W, barH};

    float availW = W - panelW - PAD * 3.f;
    float availH = H - barH - PAD * 2.f;
    float boardSize = std::floor(std::min(availW, availH));
    float bx = PAD + (availW - boardSize) * 0.5f;
    float by = barH + PAD + (availH - boardSize) * 0.5f;
    board_rect_ = {bx, by, boardSize, boardSize};
    view_.rect = board_rect_;
    view_.flipped = board_flipped_;

    side_panel_rect_ = {W - panelW - PAD, barH + PAD, panelW, H - barH - PAD * 2.f};
}

// =============================================================
// Main loop
// =============================================================
int App::run() {
    sf::Clock frameClock;
    while (window_.isOpen()) {
        sf::Event e;
        while (window_.pollEvent(e)) {
            handle_event(e);
        }

        // Tick engine if running
        if (engine_busy_.load() && engine_future_.valid()) {
            if (engine_future_.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                auto r = engine_future_.get();
                engine_busy_ = false;
                engine_thinking_ = false;
                last_result_ = r;
                if (!r.best.is_null() && !game_over_flag_ && !is_human_turn()) {
                    commit_move(r.best);
                }
            }
        }

        // Animations
        float dt = frameClock.restart().asSeconds();
        if (animation_) {
            animation_->t += dt * (1.f / std::max(0.05f, animation_->duration)) * settings_.animation_speed;
            if (animation_->t >= 1.f) animation_.reset();
        }
        pulse_ = 0.5f + 0.5f * std::sin(static_cast<float>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count() % 1000) * 0.00628f);

        // Clocks
        if (use_clock_ && clock_.running && !game_over_flag_ && screen_ == Screen::Game) {
            clock_.tick();
            for (int i = 0; i < 2; ++i) {
                if (clock_.remaining_ms[i] <= 0) {
                    game_over_flag_ = true;
                    result_string_ = (i == 0 ? "Black wins on time" : "White wins on time");
                    clock_.stop();
                    screen_ = Screen::GameOver;
                }
            }
        }

        window_.clear(theme_.bg);

        switch (screen_) {
            case Screen::MainMenu:        draw_main_menu(); break;
            case Screen::PlayMode:        draw_play_mode_menu(); break;
            case Screen::Difficulty:      draw_difficulty_menu(); break;
            case Screen::TimeControl:     draw_time_control_menu(); break;
            case Screen::ColorChoice:     draw_color_choice_menu(); break;
            case Screen::Settings:        draw_settings(); break;
            case Screen::Statistics:      draw_statistics(); break;
            case Screen::About:           draw_about(); break;
            case Screen::Game:            draw_game_screen(); break;
            case Screen::Analysis:        draw_analysis_screen(); break;
            case Screen::PromotionDialog: draw_promotion_dialog(); break;
            case Screen::FenDialog:       draw_fen_dialog(); break;
            case Screen::GameOver:        draw_game_over(); break;
            case Screen::Pause:           draw_pause(); break;
            case Screen::Puzzle:          draw_analysis_screen(); break;
            default:                      draw_main_menu(); break;
        }

        window_.display();
    }
    io::save_settings(settings_);
    io::save_stats(stats_);
    return 0;
}

// =============================================================
// Event handling
// =============================================================
void App::handle_event(const sf::Event& e) {
    if (e.type == sf::Event::Closed) {
        window_.close();
    }
    if (e.type == sf::Event::Resized) {
        sf::FloatRect visibleArea(0.f, 0.f,
            static_cast<float>(e.size.width), static_cast<float>(e.size.height));
        window_.setView(sf::View(visibleArea));
        layout();
    }
    if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left) {
        auto world = window_.mapPixelToCoords({e.mouseButton.x, e.mouseButton.y});
        handle_mouse_press(world);
    }
    if (e.type == sf::Event::MouseButtonReleased && e.mouseButton.button == sf::Mouse::Left) {
        auto world = window_.mapPixelToCoords({e.mouseButton.x, e.mouseButton.y});
        handle_mouse_release(world);
    }
    if (e.type == sf::Event::TextEntered && screen_ == Screen::FenDialog) {
        auto u = e.text.unicode;
        if (u == 8) {
            if (!fen_buffer_.empty()) fen_buffer_.pop_back();
        } else if (u >= 32 && u < 127) {
            fen_buffer_ += static_cast<char>(u);
        }
    }
    if (e.type == sf::Event::KeyPressed) {
        bool ctrl = e.key.control;
        handle_key_press(e.key.code, ctrl);
    }
}

void App::handle_key_press(sf::Keyboard::Key k, bool ctrl) {
    if (screen_ == Screen::FenDialog && k != sf::Keyboard::Escape) return;

    // Global shortcuts
    if (ctrl && k == sf::Keyboard::Z) { undo(); return; }
    if (ctrl && k == sf::Keyboard::Y) { redo(); return; }
    if (ctrl && k == sf::Keyboard::F && screen_ == Screen::Game) {
        fen_buffer_ = board_.to_fen();
        fen_error_.clear();
        screen_ = Screen::FenDialog;
        return;
    }

    switch (k) {
        case sf::Keyboard::Escape:
            if (screen_ == Screen::FenDialog || screen_ == Screen::PromotionDialog) {
                screen_ = Screen::Game;
                pending_promo_.reset();
            } else if (screen_ == Screen::Game) {
                screen_ = Screen::Pause;
            } else if (screen_ == Screen::Pause) {
                screen_ = Screen::Game;
            } else if (screen_ != Screen::MainMenu) {
                screen_ = Screen::MainMenu;
            }
            break;
        case sf::Keyboard::Left:  if (screen_ == Screen::Game) goto_move(view_index_ - 1); break;
        case sf::Keyboard::Right: if (screen_ == Screen::Game) goto_move(view_index_ + 1); break;
        case sf::Keyboard::Home:  if (screen_ == Screen::Game) goto_move(-1); break;
        case sf::Keyboard::End:   if (screen_ == Screen::Game) goto_move((int)history_.size() - 1); break;
        case sf::Keyboard::R:     if (screen_ == Screen::Game) reset_game(); break;
        case sf::Keyboard::H:     if (screen_ == Screen::Game) do_hint(); break;
        case sf::Keyboard::Space:
            if (screen_ == Screen::Game && use_clock_) {
                clock_.running = !clock_.running;
                clock_.last = std::chrono::steady_clock::now();
            }
            break;
        default: break;
    }
}

void App::handle_mouse_press(sf::Vector2f p) {
    if (screen_ == Screen::PromotionDialog) {
        auto size = window_.getSize();
        float bw = 80.f;
        float bx = static_cast<float>(size.x) * 0.5f - 2.f * bw;
        float by = static_cast<float>(size.y) * 0.5f - bw * 0.5f;
        PieceType choices[4] = {PieceType::Queen, PieceType::Rook,
                                PieceType::Bishop, PieceType::Knight};
        for (int i = 0; i < 4; ++i) {
            sf::FloatRect r(bx + i * bw, by, bw, bw);
            if (r.contains(p)) {
                if (pending_promo_) {
                    pending_promo_->promo = choices[i];
                    commit_move(*pending_promo_);
                    pending_promo_.reset();
                }
                screen_ = Screen::Game;
                return;
            }
        }
        return;
    }

    if (screen_ == Screen::Game || screen_ == Screen::Analysis || screen_ == Screen::Puzzle) {
        Square s = view_.square_at(p);
        if (valid_square(s)) {
            Piece pc = board_.at(s);
            if (!pc.empty() && pc.color == board_.side_to_move() && is_human_turn()) {
                // Start drag
                Move placeholder;
                placeholder.from = s;
                placeholder.to   = s;
                drag_move_ = placeholder;
                drag_pos_  = p;
                try_select(s);
            } else {
                try_move(s);
            }
        }
    }
}

void App::handle_mouse_release(sf::Vector2f p) {
    if ((screen_ == Screen::Game || screen_ == Screen::Analysis || screen_ == Screen::Puzzle)
        && drag_move_) {
        Square to = view_.square_at(p);
        if (valid_square(to) && selected_ != NO_SQUARE && to != selected_) {
            try_move(to);
        }
        drag_move_.reset();
    }
}

// =============================================================
// Interaction -> chess
// =============================================================
void App::try_select(Square s) {
    selected_ = s;
    legal_from_selected_.clear();
    for (const Move& m : board_.generate_legal_moves()) {
        if (m.from == s) legal_from_selected_.push_back(m);
    }
    sound_.play(Sfx::UI);
}

void App::try_move(Square to) {
    if (selected_ == NO_SQUARE || !valid_square(to)) return;
    for (const Move& m : legal_from_selected_) {
        if (m.to == to) {
            if (m.promo != PieceType::None) {
                pending_promo_ = m;
                promo_from_ = m.from;
                promo_to_   = m.to;
                screen_ = Screen::PromotionDialog;
                return;
            }
            commit_move(m);
            return;
        }
    }
    sound_.play(Sfx::Illegal);
}

void App::commit_move(const Move& m) {
    Piece capturedSq = board_.at(m.en_passant ? mk_square(file_of(m.to), rank_of(m.from)) : m.to);
    Piece moving     = board_.at(m.from);
    if (moving.empty()) return;

    std::string san = move_to_san(board_, m);
    std::string uci = move_to_uci(m);

    if (!board_.make_move(m)) {
        sound_.play(Sfx::Illegal);
        return;
    }

    // Start animation only after a successful move.
    PieceAnimation anim;
    anim.move     = m;
    anim.piece    = moving;
    anim.captured = capturedSq;
    anim.capSq    = m.en_passant ? mk_square(file_of(m.to), rank_of(m.from)) : m.to;
    anim.duration = settings_.reduced_motion ? 0.0f : 0.15f;
    animation_ = anim;

    history_.push_back({m, san, uci, capturedSq, board_.to_fen()});
    view_index_ = static_cast<int>(history_.size()) - 1;
    redo_.clear();

    refresh_opening();

    last_from_ = m.from;
    last_to_   = m.to;
    selected_ = NO_SQUARE;
    legal_from_selected_.clear();
    hint_active_ = false;

    // Sound
    if (board_.in_check(board_.side_to_move())) {
        if (board_.is_checkmate()) sound_.play(Sfx::Checkmate);
        else                       sound_.play(Sfx::Check);
    } else if (!capturedSq.empty() || m.en_passant) sound_.play(Sfx::Capture);
    else if (m.promo != PieceType::None)            sound_.play(Sfx::Promote);
    else                                            sound_.play(Sfx::Move);

    // Clock
    if (use_clock_ && clock_.running) {
        clock_.switch_turn(board_.side_to_move(), true);
    }

    // Puzzle check
    if (mode_ == GameMode::Puzzle) check_puzzle_result();

    // Game-over conditions
    auto legal = board_.generate_legal_moves();
    if (legal.empty()) {
        game_over_flag_ = true;
        if (board_.in_check(board_.side_to_move())) {
            result_string_ = (board_.side_to_move() == Color::White)
                ? "Checkmate - Black wins" : "Checkmate - White wins";
            if (mode_ == GameMode::PvC) {
                bool humanWon = (board_.side_to_move() == Color::White) == !human_plays_white_;
                if (humanWon) stats_.wins++; else stats_.losses++;
            }
        } else {
            result_string_ = "Stalemate - Draw";
            stats_.draws++;
        }
        stats_.games_played++;
        stats_.total_move_count += static_cast<int>(history_.size());
        io::save_stats(stats_);

        io::GameRecord rec;
        rec.white   = (mode_ == GameMode::PvC && !human_plays_white_) ? "Computer" : "Player";
        rec.black   = (mode_ == GameMode::PvC &&  human_plays_white_) ? "Computer" : "Player";
        rec.result  = result_string_;
        rec.opening = current_opening_;
        rec.moves   = static_cast<int>(history_.size());
        {
            std::time_t t = std::time(nullptr);
            char buf[32];
            std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", std::localtime(&t));
            rec.date = buf;
        }
        io::append_game_record(rec);
        screen_ = Screen::GameOver;
        return;
    }
    if (board_.is_fifty_move()) {
        game_over_flag_ = true;
        result_string_ = "Draw - 50 move rule";
        stats_.games_played++; stats_.draws++;
        io::save_stats(stats_);
        screen_ = Screen::GameOver;
        return;
    }
    if (board_.is_threefold()) {
        game_over_flag_ = true;
        result_string_ = "Draw - Threefold repetition";
        stats_.games_played++; stats_.draws++;
        io::save_stats(stats_);
        screen_ = Screen::GameOver;
        return;
    }
    if (board_.is_insufficient_material()) {
        game_over_flag_ = true;
        result_string_ = "Draw - Insufficient material";
        stats_.games_played++; stats_.draws++;
        io::save_stats(stats_);
        screen_ = Screen::GameOver;
        return;
    }

    maybe_start_engine_move();
}

void App::do_hint() {
    if (game_over_flag_ || board_.generate_legal_moves().empty()) return;
    engine::Engine eng;
    eng.set_difficulty(difficulty_);
    auto hints = eng.hints(board_, 1);
    if (hints.empty()) return;
    hint_from_ = hints[0].move.from;
    hint_to_   = hints[0].move.to;
    hint_text_ = hints[0].explanation;
    hint_active_ = true;
    sound_.play(Sfx::UI);
}

void App::do_show_best() {
    do_hint();
    if (hint_active_) hint_text_ += "  (engine best)";
}

void App::undo() {
    if (history_.empty()) return;
    bool doubleUndo = (mode_ == GameMode::CvC) || (mode_ == GameMode::PvC && !is_human_turn());
    if (doubleUndo && history_.size() >= 2) {
        board_.unmake_move();
        auto m2 = history_.back(); history_.pop_back();
        redo_.push_back({m2.move});
        board_.unmake_move();
        auto m1 = history_.back(); history_.pop_back();
        redo_.push_back({m1.move});
    } else {
        board_.unmake_move();
        auto m = history_.back(); history_.pop_back();
        redo_.push_back({m.move});
    }
    selected_ = NO_SQUARE;
    legal_from_selected_.clear();
    if (!history_.empty()) {
        last_from_ = history_.back().move.from;
        last_to_   = history_.back().move.to;
    } else {
        last_from_ = last_to_ = NO_SQUARE;
    }
    view_index_ = static_cast<int>(history_.size()) - 1;
    hint_active_ = false;
    refresh_opening();
    sound_.play(Sfx::UI);
}

void App::redo() {
    if (redo_.empty()) return;
    Move m = redo_.back().move;
    redo_.pop_back();
    Piece captured = board_.at(m.to);
    std::string san = move_to_san(board_, m);
    if (board_.make_move(m)) {
        history_.push_back({m, san, move_to_uci(m), captured, board_.to_fen()});
        view_index_ = static_cast<int>(history_.size()) - 1;
        last_from_ = m.from;
        last_to_   = m.to;
        refresh_opening();
    }
    sound_.play(Sfx::UI);
}

void App::goto_move(int index) {
    if (index < -1) index = -1;
    if (index >= static_cast<int>(history_.size()))
        index = static_cast<int>(history_.size()) - 1;

    board_.set_startpos();
    board_.clear_history();
    for (int i = 0; i <= index; ++i) {
        board_.make_move(history_[i].move);
    }
    view_index_ = index;
    if (index >= 0) {
        last_from_ = history_[index].move.from;
        last_to_   = history_[index].move.to;
    } else {
        last_from_ = last_to_ = NO_SQUARE;
    }
    selected_ = NO_SQUARE;
    legal_from_selected_.clear();
}

void App::refresh_opening() {
    auto sans = san_move_list();
    auto info = io::detect_opening(sans);
    current_opening_ = info.name;
    if (!info.variation.empty()) current_opening_ += ": " + info.variation;
}

std::vector<std::string> App::san_move_list() const {
    std::vector<std::string> v;
    v.reserve(history_.size());
    for (auto& h : history_) v.push_back(h.san);
    return v;
}

// =============================================================
// Rendering helpers
// =============================================================
void App::draw_panel(sf::FloatRect r, sf::Color c, bool outlined) {
    sf::RectangleShape s({r.width, r.height});
    s.setPosition(r.left, r.top);
    s.setFillColor(c);
    if (outlined) {
        s.setOutlineColor(theme_.border);
        s.setOutlineThickness(1.f);
    }
    window_.draw(s);
}

void App::draw_text(const std::string& s, sf::Vector2f p, unsigned size, sf::Color c,
                    bool bold, int align) {
    if (!fonts_ok_) return;
    const sf::Font& f = (bold && !font_bold_.getInfo().family.empty()) ? font_bold_ : font_;
    sf::Text t(s, f, size);
    t.setFillColor(c);
    auto b = t.getLocalBounds();
    if      (align == 1) t.setPosition(p.x - b.width * 0.5f - b.left, p.y);
    else if (align == 2) t.setPosition(p.x - b.width - b.left, p.y);
    else                 t.setPosition(p.x - b.left, p.y);
    window_.draw(t);
}

void App::draw_text_wrapped(const std::string& s, sf::FloatRect box, unsigned size, sf::Color c) {
    if (!fonts_ok_) return;
    std::string word, line;
    float y = box.top;
    for (size_t i = 0; i < s.size(); ++i) {
        char ch = s[i];
        word += ch;
        if (ch == ' ' || i + 1 == s.size()) {
            sf::Text t(line + word, font_, size);
            if (t.getLocalBounds().width > box.width && !line.empty()) {
                draw_text(line, {box.left, y}, size, c);
                y += t.getCharacterSize() * 1.2f;
                line.clear();
            }
            line += word;
            word.clear();
            if (y > box.top + box.height) break;
        }
    }
    if (!line.empty()) draw_text(line, {box.left, y}, size, c);
}

void App::draw_buttons(const std::vector<Button>& buttons, sf::Vector2f mouse) {
    for (const auto& b : buttons) {
        bool hover = b.rect.contains(mouse) && b.enabled;
        sf::Color fill = b.primary ? theme_.accent : theme_.card;
        if (hover) {
            fill = b.primary
                ? sf::Color(std::min<int>(255, theme_.accent.r + 25),
                            std::min<int>(255, theme_.accent.g + 25),
                            std::min<int>(255, theme_.accent.b + 25))
                : theme_.panel_alt;
        }
        if (!b.enabled) fill = theme_.panel_alt;

        sf::RectangleShape r({b.rect.width, b.rect.height});
        r.setPosition(b.rect.left, b.rect.top);
        r.setFillColor(fill);
        r.setOutlineThickness(1.f);
        r.setOutlineColor(theme_.border);
        window_.draw(r);

        sf::Color textCol = b.primary ? sf::Color::White : theme_.text;
        if (!b.enabled) textCol = theme_.text_dim;
        draw_text(b.label,
                  {b.rect.left + b.rect.width * 0.5f, b.rect.top + b.rect.height * 0.5f - 10.f},
                  16, textCol, b.primary, 1);
    }
}

// =============================================================
// Main menu
// =============================================================
void App::draw_main_menu() {
    auto size = window_.getSize();
    sf::Vector2f mouse = window_.mapPixelToCoords(sf::Mouse::getPosition(window_));
    float W = static_cast<float>(size.x);
    float H = static_cast<float>(size.y);

    draw_text("CHESS", {W * 0.5f, H * 0.14f}, 68, theme_.text, true, 1);
    draw_text("A premium C++ chess application", {W * 0.5f, H * 0.14f + 78.f},
              18, theme_.text_dim, false, 1);

    std::vector<Button> buttons;
    auto add = [&](const std::string& label, std::function<void()> f) {
        Button b; b.label = label; b.on_click = std::move(f); buttons.push_back(b);
    };
    add("Play",                 [&]{ screen_ = Screen::PlayMode; });
    add("Practice",             [&]{ new_game(GameMode::Practice, difficulty_, true); screen_ = Screen::Game; });
    add("Computer vs Computer", [&]{ mode_ = GameMode::CvC; screen_ = Screen::Difficulty; });
    add("Puzzles",              [&]{ load_puzzle(0); screen_ = Screen::Analysis; });
    add("Analysis",             [&]{ new_game(GameMode::Analysis, difficulty_, true); screen_ = Screen::Analysis; });
    add("Statistics",           [&]{ screen_ = Screen::Statistics; });
    add("Settings",             [&]{ screen_ = Screen::Settings; });
    add("About",                [&]{ screen_ = Screen::About; });
    add("Quit",                 [&]{ window_.close(); });

    float bw = 360.f, bh = 48.f, gap = 12.f;
    float totalH = buttons.size() * bh + (buttons.size() - 1) * gap;
    float startY = H * 0.5f - totalH * 0.5f + 30.f;
    for (size_t i = 0; i < buttons.size(); ++i) {
        buttons[i].rect = {W * 0.5f - bw * 0.5f, startY + i * (bh + gap), bw, bh};
        buttons[i].primary = (i == 0);
    }

    draw_buttons(buttons, mouse);

    if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
        for (auto& b : buttons) {
            if (b.rect.contains(mouse) && b.enabled && b.on_click) {
                auto cb = b.on_click;
                b.on_click = nullptr;
                cb();
                break;
            }
        }
    }
}

void App::draw_play_mode_menu() {
    auto size = window_.getSize();
    sf::Vector2f mouse = window_.mapPixelToCoords(sf::Mouse::getPosition(window_));
    float W = static_cast<float>(size.x), H = static_cast<float>(size.y);

    draw_text("Choose Game Mode", {W * 0.5f, H * 0.12f}, 40, theme_.text, true, 1);

    struct Card { std::string title; std::string desc; Screen next; GameMode mode; };
    std::vector<Card> cards = {
        {"Player vs Player",     "Two humans on the same computer.", Screen::TimeControl, GameMode::PvP},
        {"Player vs Computer",   "Play against the built-in engine.", Screen::ColorChoice, GameMode::PvC},
        {"Computer vs Computer", "Watch the engine play both sides.", Screen::TimeControl, GameMode::CvC},
        {"Practice Mode",        "Experiment freely with no clock.",  Screen::Game,        GameMode::Practice},
    };

    float cw = 320.f, ch = 180.f, gap = 20.f;
    float totalW = cards.size() * cw + (cards.size() - 1) * gap;
    float sx = W * 0.5f - totalW * 0.5f;
    float sy = H * 0.35f;
    for (size_t i = 0; i < cards.size(); ++i) {
        sf::FloatRect r(sx + i * (cw + gap), sy, cw, ch);
        bool hover = r.contains(mouse);
        draw_panel(r, hover ? theme_.panel_alt : theme_.card);
        draw_text(cards[i].title, {r.left + 20.f, r.top + 20.f}, 22, theme_.text, true);
        draw_text_wrapped(cards[i].desc,
                          {r.left + 20.f, r.top + 60.f, r.width - 40.f, r.height - 80.f},
                          14, theme_.text_dim);
        if (hover && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            mode_ = cards[i].mode;
            if (cards[i].mode == GameMode::Practice) {
                new_game(GameMode::Practice, difficulty_, true);
                screen_ = Screen::Game;
            } else {
                screen_ = cards[i].next;
            }
            sf::sleep(sf::milliseconds(120));
            return;
        }
    }

    Button back; back.label = "Back"; back.rect = {20.f, H - 60.f, 120.f, 40.f};
    std::vector<Button> btn = {back};
    draw_buttons(btn, mouse);
    if (back.rect.contains(mouse) && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
        screen_ = Screen::MainMenu;
        sf::sleep(sf::milliseconds(120));
    }
}

void App::draw_difficulty_menu() {
    auto size = window_.getSize();
    sf::Vector2f mouse = window_.mapPixelToCoords(sf::Mouse::getPosition(window_));
    float W = static_cast<float>(size.x), H = static_cast<float>(size.y);

    draw_text("Engine Strength", {W * 0.5f, H * 0.12f}, 40, theme_.text, true, 1);

    struct Level { engine::Difficulty d; std::string name; std::string desc; };
    std::vector<Level> levels = {
        {engine::Difficulty::Beginner, "Beginner", "Very fast, intentionally weak."},
        {engine::Difficulty::Easy,     "Easy",     "Basic tactical awareness."},
        {engine::Difficulty::Medium,   "Medium",   "Solid club player."},
        {engine::Difficulty::Hard,     "Hard",     "Strong search, deep lines."},
        {engine::Difficulty::Expert,   "Expert",   "Advanced engine strength."},
        {engine::Difficulty::Master,   "Master",   "Maximum practical strength."},
    };

    float cw = 260.f, ch = 120.f, gap = 16.f;
    int cols = 3, rows = 2;
    float totalW = cols * cw + (cols - 1) * gap;
    float sx = W * 0.5f - totalW * 0.5f;
    float sy = H * 0.3f;
    for (size_t i = 0; i < levels.size(); ++i) {
        int col = static_cast<int>(i) % cols;
        int row = static_cast<int>(i) / cols;
        sf::FloatRect r(sx + col * (cw + gap), sy + row * (ch + gap), cw, ch);
        bool selected = levels[i].d == difficulty_;
        bool hover = r.contains(mouse);
        draw_panel(r, selected ? theme_.accent_dim : (hover ? theme_.panel_alt : theme_.card));
        draw_text(levels[i].name, {r.left + 16.f, r.top + 14.f}, 22, theme_.text, true);
        draw_text_wrapped(levels[i].desc,
                          {r.left + 16.f, r.top + 52.f, r.width - 32.f, r.height - 60.f},
                          13, theme_.text_dim);
        if (hover && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            difficulty_ = levels[i].d;
            stats_.last_difficulty = static_cast<int>(difficulty_);
            sf::sleep(sf::milliseconds(100));
            screen_ = (mode_ == GameMode::CvC) ? Screen::TimeControl : Screen::ColorChoice;
            return;
        }
    }

    Button back; back.label = "Back"; back.rect = {20.f, H - 60.f, 120.f, 40.f};
    std::vector<Button> btn = {back};
    draw_buttons(btn, mouse);
    if (back.rect.contains(mouse) && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
        screen_ = Screen::PlayMode;
        sf::sleep(sf::milliseconds(120));
    }
}

void App::draw_color_choice_menu() {
    auto size = window_.getSize();
    sf::Vector2f mouse = window_.mapPixelToCoords(sf::Mouse::getPosition(window_));
    float W = static_cast<float>(size.x), H = static_cast<float>(size.y);

    draw_text("Choose Your Color", {W * 0.5f, H * 0.2f}, 40, theme_.text, true, 1);

    float cw = 260.f, ch = 200.f, gap = 40.f;
    float sx = W * 0.5f - (cw + gap * 0.5f);
    float sy = H * 0.4f;

    struct Card { std::string title; std::string sub; bool white; };
    std::vector<Card> cards = {
        {"Play as White", "You move first.",          true},
        {"Play as Black", "The engine moves first.",  false},
    };
    for (auto& c : cards) {
        sf::FloatRect r = c.white ? sf::FloatRect(sx, sy, cw, ch)
                                  : sf::FloatRect(sx + cw + gap, sy, cw, ch);
        bool hover = r.contains(mouse);
        draw_panel(r, hover ? theme_.panel_alt : theme_.card);
        draw_text(c.title, {r.left + r.width * 0.5f, r.top + 40.f}, 26, theme_.text, true, 1);
        draw_text(c.sub,   {r.left + r.width * 0.5f, r.top + 90.f}, 14, theme_.text_dim, false, 1);

        Piece p; p.type = PieceType::King; p.color = c.white ? Color::White : Color::Black;
        draw_piece(window_, p, r.left + r.width * 0.5f - 40.f, r.top + 120.f, 80.f);

        if (hover && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            human_plays_white_ = c.white;
            board_flipped_ = !c.white;
            sf::sleep(sf::milliseconds(100));
            screen_ = Screen::TimeControl;
            return;
        }
    }

    Button back; back.label = "Back"; back.rect = {20.f, H - 60.f, 120.f, 40.f};
    std::vector<Button> btn = {back};
    draw_buttons(btn, mouse);
    if (back.rect.contains(mouse) && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
        screen_ = Screen::Difficulty;
        sf::sleep(sf::milliseconds(120));
    }
}

void App::draw_time_control_menu() {
    auto size = window_.getSize();
    sf::Vector2f mouse = window_.mapPixelToCoords(sf::Mouse::getPosition(window_));
    float W = static_cast<float>(size.x), H = static_cast<float>(size.y);

    draw_text("Time Control", {W * 0.5f, H * 0.12f}, 40, theme_.text, true, 1);

    struct Preset { std::string label; int base_min; int inc_s; };
    std::vector<Preset> presets = {
        {"1+0", 1, 0}, {"2+1", 2, 1}, {"3+0", 3, 0}, {"3+2", 3, 2},
        {"5+0", 5, 0}, {"5+3", 5, 3}, {"10+0", 10, 0}, {"10+5", 10, 5},
        {"15+10", 15, 10}, {"30+0", 30, 0},
        {"No Clock", 0, 0},
    };

    float cw = 180.f, ch = 70.f, gap = 14.f;
    int cols = 4;
    float totalW = cols * cw + (cols - 1) * gap;
    float sx = W * 0.5f - totalW * 0.5f;
    float sy = H * 0.3f;
    for (size_t i = 0; i < presets.size(); ++i) {
        int col = static_cast<int>(i) % cols;
        int row = static_cast<int>(i) / cols;
        sf::FloatRect r(sx + col * (cw + gap), sy + row * (ch + gap), cw, ch);
        bool hover = r.contains(mouse);
        draw_panel(r, hover ? theme_.panel_alt : theme_.card);
        draw_text(presets[i].label,
                  {r.left + r.width * 0.5f, r.top + r.height * 0.5f - 14.f},
                  22, theme_.text, true, 1);
        if (hover && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            use_clock_ = (presets[i].base_min > 0);
            clock_.reset(presets[i].base_min * 60000, presets[i].inc_s * 1000);
            sf::sleep(sf::milliseconds(100));
            new_game(mode_, difficulty_, human_plays_white_);
            screen_ = Screen::Game;
            return;
        }
    }

    Button back; back.label = "Back"; back.rect = {20.f, H - 60.f, 120.f, 40.f};
    std::vector<Button> btn = {back};
    draw_buttons(btn, mouse);
    if (back.rect.contains(mouse) && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
        screen_ = Screen::PlayMode;
        sf::sleep(sf::milliseconds(120));
    }
}

// =============================================================
// Settings
// =============================================================
void App::draw_settings() {
    auto size = window_.getSize();
    sf::Vector2f mouse = window_.mapPixelToCoords(sf::Mouse::getPosition(window_));
    float W = static_cast<float>(size.x), H = static_cast<float>(size.y);

    draw_text("Settings", {W * 0.5f, 40.f}, 36, theme_.text, true, 1);

    float pw = 640.f;
    float sx = W * 0.5f - pw * 0.5f;
    float y = 120.f;
    draw_panel({sx, y, pw, H - 200.f}, theme_.panel);

    struct Row { std::string label; std::string value; std::function<void()> toggle; };
    std::vector<Row> rows;

    std::vector<std::string> themes = {"Classic", "Midnight", "Ivory", "Emerald", "Graphite", "Tournament"};
    auto nextTheme = [&]() {
        auto it = std::find(themes.begin(), themes.end(), settings_.board_theme);
        int idx = (it == themes.end()) ? 0 : static_cast<int>(it - themes.begin());
        idx = (idx + 1) % static_cast<int>(themes.size());
        settings_.board_theme = themes[idx];
        build_theme();
    };

    rows.push_back({"Board Theme", settings_.board_theme, nextTheme});
    rows.push_back({"Dark Mode", settings_.dark_mode ? "On" : "Off",
        [&]{ settings_.dark_mode = !settings_.dark_mode; build_theme(); }});
    rows.push_back({"Sound", settings_.sound_enabled ? "On" : "Off",
        [&]{ settings_.sound_enabled = !settings_.sound_enabled; sound_.set_enabled(settings_.sound_enabled); }});
    rows.push_back({"Volume", std::to_string(static_cast<int>(settings_.sound_volume * 100)) + "%",
        [&]{
            settings_.sound_volume = std::fmod(settings_.sound_volume + 0.1f, 1.1f);
            if (settings_.sound_volume > 1.f) settings_.sound_volume = 0.f;
            sound_.set_volume(settings_.sound_volume);
        }});
    rows.push_back({"Show Coordinates", settings_.show_coordinates ? "On" : "Off",
        [&]{ settings_.show_coordinates = !settings_.show_coordinates; }});
    rows.push_back({"Show Legal Moves", settings_.show_legal_moves ? "On" : "Off",
        [&]{ settings_.show_legal_moves = !settings_.show_legal_moves; }});
    rows.push_back({"Engine Panel", settings_.show_engine_panel ? "On" : "Off",
        [&]{
            settings_.show_engine_panel = !settings_.show_engine_panel;
            show_engine_panel_ = settings_.show_engine_panel;
        }});
    rows.push_back({"Reduced Motion", settings_.reduced_motion ? "On" : "Off",
        [&]{ settings_.reduced_motion = !settings_.reduced_motion; }});

    for (size_t i = 0; i < rows.size(); ++i) {
        float ry = y + 20.f + i * 56.f;
        draw_text(rows[i].label, {sx + 24.f, ry + 10.f}, 17, theme_.text, false);
        sf::FloatRect vr(sx + pw - 240.f, ry, 216.f, 40.f);
        bool hover = vr.contains(mouse);
        draw_panel(vr, hover ? theme_.panel_alt : theme_.card);
        draw_text(rows[i].value, {vr.left + vr.width * 0.5f, vr.top + 10.f}, 16, theme_.text, false, 1);
        if (hover && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            rows[i].toggle();
            sf::sleep(sf::milliseconds(120));
        }
    }

    Button back; back.label = "Back"; back.rect = {sx + pw * 0.5f - 80.f, H - 70.f, 160.f, 44.f};
    back.primary = true;
    std::vector<Button> btn = {back};
    draw_buttons(btn, mouse);
    if (back.rect.contains(mouse) && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
        io::save_settings(settings_);
        screen_ = Screen::MainMenu;
        sf::sleep(sf::milliseconds(120));
    }
}

// =============================================================
// Statistics
// =============================================================
void App::draw_statistics() {
    auto size = window_.getSize();
    sf::Vector2f mouse = window_.mapPixelToCoords(sf::Mouse::getPosition(window_));
    float W = static_cast<float>(size.x), H = static_cast<float>(size.y);

    draw_text("Statistics", {W * 0.5f, 40.f}, 36, theme_.text, true, 1);

    float pw = 800.f, ph = H - 200.f;
    float sx = W * 0.5f - pw * 0.5f;
    float sy = 120.f;
    draw_panel({sx, sy, pw, ph}, theme_.panel);

    int total = stats_.games_played;
    float winRate = total ? (100.f * stats_.wins / total) : 0.f;
    float avgLen = total ? static_cast<float>(stats_.total_move_count) / total : 0.f;

    auto row = [&](float y, const std::string& k, const std::string& v) {
        draw_text(k, {sx + 32.f, y}, 16, theme_.text_dim);
        draw_text(v, {sx + pw - 32.f, y}, 18, theme_.text, true, 2);
    };
    float y = sy + 30.f;
    row(y, "Games Played",         std::to_string(stats_.games_played));           y += 34.f;
    row(y, "Wins",                 std::to_string(stats_.wins));                   y += 34.f;
    row(y, "Losses",               std::to_string(stats_.losses));                 y += 34.f;
    row(y, "Draws",                std::to_string(stats_.draws));                  y += 34.f;
    row(y, "Win Rate",             std::to_string(static_cast<int>(winRate)) + "%"); y += 34.f;
    row(y, "Average Game Length",  std::to_string(static_cast<int>(avgLen)) + " moves"); y += 34.f;
    row(y, "Puzzles Attempted",    std::to_string(stats_.puzzles_attempted));      y += 34.f;
    row(y, "Puzzles Solved",       std::to_string(stats_.puzzles_solved));         y += 34.f;

    y += 24.f;
    draw_text("Recent Games", {sx + 32.f, y}, 20, theme_.text, true);
    y += 32.f;
    auto recent = io::load_recent_games();
    int shown = 0;
    for (auto it = recent.rbegin(); it != recent.rend() && shown < 6; ++it, ++shown) {
        std::string line = it->date + "   " + it->white + " vs " + it->black + "   " + it->result;
        draw_text(line, {sx + 32.f, y}, 15, theme_.text_dim);
        y += 26.f;
    }

    Button back; back.label = "Back"; back.rect = {sx + pw * 0.5f - 80.f, H - 70.f, 160.f, 44.f};
    back.primary = true;
    std::vector<Button> btn = {back};
    draw_buttons(btn, mouse);
    if (back.rect.contains(mouse) && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
        screen_ = Screen::MainMenu;
        sf::sleep(sf::milliseconds(120));
    }
}

// =============================================================
// About
// =============================================================
void App::draw_about() {
    auto size = window_.getSize();
    sf::Vector2f mouse = window_.mapPixelToCoords(sf::Mouse::getPosition(window_));
    float W = static_cast<float>(size.x), H = static_cast<float>(size.y);

    draw_text("About", {W * 0.5f, 60.f}, 36, theme_.text, true, 1);

    float pw = 700.f;
    float sx = W * 0.5f - pw * 0.5f;
    float sy = 140.f;
    draw_panel({sx, sy, pw, H - 260.f}, theme_.panel);
    draw_text_wrapped(
        "Chess is a complete, from-scratch chess application written in modern C++20. "
        "It uses SFML for windowing, rendering and audio, but contains no game engine. "
        "The chess engine, UI, rules, animations and theming are all hand-written.\n\n"
        "The engine implements alpha-beta search with iterative deepening, quiescence search, "
        "a transposition table with Zobrist hashing, killer moves, history heuristics and a "
        "piece-square-table-based evaluation function that considers material, king safety, "
        "pawn structure, mobility and bishop pairs.\n\n"
        "All chess rules are fully supported: castling, en passant, promotion, threefold "
        "repetition, the fifty-move rule, insufficient material, check, checkmate and stalemate.",
        {sx + 32.f, sy + 32.f, pw - 64.f, H - 340.f}, 15, theme_.text);

    Button back; back.label = "Back"; back.rect = {sx + pw * 0.5f - 80.f, H - 80.f, 160.f, 44.f};
    back.primary = true;
    std::vector<Button> btn = {back};
    draw_buttons(btn, mouse);
    if (back.rect.contains(mouse) && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
        screen_ = Screen::MainMenu;
        sf::sleep(sf::milliseconds(120));
    }
}

// =============================================================
// Game screen
// =============================================================
void App::draw_game_screen() {
    auto size = window_.getSize();
    sf::Vector2f mouse = window_.mapPixelToCoords(sf::Mouse::getPosition(window_));
    float W = static_cast<float>(size.x), H = static_cast<float>(size.y);
    view_.rect = board_rect_;
    view_.flipped = board_flipped_;

    // Top bar
    draw_panel(top_bar_rect_, theme_.panel);
    draw_text("Game", {PAD + 8.f, 14.f}, 20, theme_.text, true);
    draw_text(current_opening_.empty() ? "" : current_opening_,
              {PAD + 90.f, 18.f}, 14, theme_.text_dim);

    if (engine_thinking_) {
        draw_text("Engine thinking...", {W - 220.f, 18.f}, 14, theme_.accent);
    }

    // Board
    draw_board(window_, view_, palette_, settings_.show_coordinates, settings_.dark_mode);

    if (last_from_ != NO_SQUARE || last_to_ != NO_SQUARE)
        draw_last_move(window_, view_, palette_, last_from_, last_to_);
    if (selected_ != NO_SQUARE)
        draw_selected(window_, view_, palette_, selected_);

    if (board_.in_check(board_.side_to_move())) {
        Square k = board_.king_square(board_.side_to_move());
        draw_check_ring(window_, view_, palette_, k);
    }

    if (settings_.show_legal_moves && selected_ != NO_SQUARE) {
        for (const Move& m : legal_from_selected_) {
            bool cap = !board_.at(m.to).empty() || m.en_passant;
            draw_legal_indicator(window_, view_, palette_, m.to, cap);
        }
    }

    if (hint_active_) {
        draw_hint_squares(window_, view_, palette_, hint_from_, hint_to_, pulse_);
    }

    // Pieces (with animation)
    if (animation_) {
        for (int s = 0; s < 64; ++s) {
            if (s == animation_->move.from) continue;
            if (s == animation_->capSq)     continue;
            if (s == animation_->move.to)   continue;
            Piece p = board_.at(s);
            if (p.empty()) continue;
            sf::Vector2f pp = view_.square_pos(s);
            draw_piece(window_, p, pp.x, pp.y, view_.square_size());
        }
        sf::Vector2f from = view_.square_pos(animation_->move.from);
        sf::Vector2f to   = view_.square_pos(animation_->move.to);
        float t = std::clamp(animation_->t, 0.f, 1.f);
        float x = from.x + (to.x - from.x) * t;
        float y = from.y + (to.y - from.y) * t;
        draw_piece(window_, animation_->piece, x, y, view_.square_size());
    } else {
        for (int s = 0; s < 64; ++s) {
            Piece p = board_.at(s);
            if (p.empty()) continue;
            sf::Vector2f pp = view_.square_pos(s);
            draw_piece(window_, p, pp.x, pp.y, view_.square_size());
        }
    }

    // ------------- Side panel -------------
    draw_panel(side_panel_rect_, theme_.panel);
    float px = side_panel_rect_.left + 16.f;
    float pw = side_panel_rect_.width - 32.f;
    float y  = side_panel_rect_.top + 16.f;

    Color topColor    = board_flipped_ ? Color::White : Color::Black;
    Color bottomColor = board_flipped_ ? Color::Black : Color::White;

    auto playerName = [&](Color c) -> std::string {
        if (mode_ == GameMode::PvP) return c == Color::White ? "White" : "Black";
        if (mode_ == GameMode::CvC) return c == Color::White ? "Engine (W)" : "Engine (B)";
        if (mode_ == GameMode::PvC) {
            if (c == (human_plays_white_ ? Color::White : Color::Black)) return "You";
            return std::string("Computer (") + std::to_string(static_cast<int>(difficulty_) + 1) + ")";
        }
        return c == Color::White ? "White" : "Black";
    };

    auto player_header = [&](Color c, const std::string& name, float yy) {
        Piece k; k.type = PieceType::King; k.color = c;
        draw_piece(window_, k, px + 4.f, yy, 36.f);
        draw_text(name, {px + 48.f, yy + 6.f}, 18, theme_.text, true);
        if (use_clock_) {
            int ms = clock_.remaining_ms[c == Color::White ? 0 : 1];
            int s = ms / 1000, m = s / 60; s %= 60;
            char buf[16];
            std::snprintf(buf, sizeof(buf), "%02d:%02d", m, s);
            sf::Color clockCol = theme_.text;
            if (clock_.running && clock_.running_for_white == (c == Color::White))
                clockCol = theme_.accent;
            if (ms < 20000) clockCol = theme_.bad;
            draw_text(buf, {px + pw, yy + 6.f}, 22, clockCol, true, 2);
        }
    };

    player_header(topColor, playerName(topColor), y);
    y += 56.f;

    auto captured_list = [&](Color c) {
        std::vector<PieceType> caps;
        for (auto& h : history_) {
            if (h.captured.empty()) continue;
            if (h.captured.color == c) caps.push_back(h.captured.type);
        }
        return caps;
    };
    auto draw_captured_row = [&](Color capturedColor, float yy) {
        auto caps = captured_list(capturedColor);
        float cx = px;
        for (PieceType t : caps) {
            Piece p; p.type = t; p.color = capturedColor;
            draw_piece(window_, p, cx, yy, 22.f);
            cx += 20.f;
            if (cx > px + pw - 30.f) break;
        }
    };

    // Captures performed by the TOP player (i.e. bottom's pieces were taken)
    draw_captured_row(bottomColor, y);
    y += 28.f;

    auto material = [&](Color c) {
        int v = 0;
        v += piece_count(board_, c, PieceType::Pawn)   * 1;
        v += piece_count(board_, c, PieceType::Knight) * 3;
        v += piece_count(board_, c, PieceType::Bishop) * 3;
        v += piece_count(board_, c, PieceType::Rook)   * 5;
        v += piece_count(board_, c, PieceType::Queen)  * 9;
        return v;
    };
    int matW = material(Color::White), matB = material(Color::Black);
    int diff = matW - matB;
    std::string matStr;
    if      (diff > 0) matStr = "White +" + std::to_string(diff);
    else if (diff < 0) matStr = "Black +" + std::to_string(-diff);
    else               matStr = "Material even";
    draw_text(matStr, {px, y}, 13, theme_.text_dim);
    y += 24.f;

    // Bottom player header
    player_header(bottomColor, playerName(bottomColor), y);
    y += 56.f;

    // Move history
    float histH = std::clamp(side_panel_rect_.height * 0.22f, 120.f, 220.f);
    draw_panel({px, y, pw, histH}, theme_.panel_alt);
    draw_text("Moves", {px + 12.f, y + 8.f}, 14, theme_.text_dim, true);
    float innerY = y + 32.f;
    int maxRows = static_cast<int>((histH - 40.f) / 22.f);
    int startIdx = 0;
    int endIdx = static_cast<int>(history_.size());
    if (view_index_ + maxRows < endIdx) startIdx = std::max(0, view_index_ - maxRows / 2);
    else                               startIdx = std::max(0, endIdx - maxRows);

    for (int i = startIdx; i < endIdx && i < startIdx + maxRows; ++i) {
        int moveNumber = i / 2 + 1;
        bool whiteMove = (i % 2) == 0;
        float rowY = innerY + (i - startIdx) * 22.f;

        std::string numStr = whiteMove ? (std::to_string(moveNumber) + ".") : "";
        draw_text(numStr, {px + 12.f, rowY}, 14, theme_.text_dim);

        sf::FloatRect moveRect(px + 48.f + (whiteMove ? 0.f : (pw - 60.f) * 0.5f),
                               rowY - 2.f, (pw - 60.f) * 0.5f, 20.f);
        if (i == view_index_) {
            sf::RectangleShape hl({moveRect.width, moveRect.height});
            hl.setPosition(moveRect.left, moveRect.top);
            hl.setFillColor(theme_.accent_dim);
            window_.draw(hl);
        }
        draw_text(history_[i].san, {moveRect.left + 4.f, rowY}, 15, theme_.text);

        if (moveRect.contains(mouse) && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            goto_move(i);
            sf::sleep(sf::milliseconds(80));
        }
    }

    // Nav buttons
    float btnY = y + histH + 12.f;
    float bw = (pw - 4 * 6.f) / 5.f;
    struct NavBtn { std::string label; std::function<void()> f; };
    std::vector<NavBtn> nav = {
        {"|<<", [&]{ goto_move(-1); }},
        {"<",   [&]{ goto_move(view_index_ - 1); }},
        {">",   [&]{ goto_move(view_index_ + 1); }},
        {">>|", [&]{ goto_move(static_cast<int>(history_.size()) - 1); }},
        {"Undo",[&]{ undo(); }},
    };
    for (size_t i = 0; i < nav.size(); ++i) {
        sf::FloatRect r(px + i * (bw + 6.f), btnY, bw, 32.f);
        bool hover = r.contains(mouse);
        draw_panel(r, hover ? theme_.panel_alt : theme_.card);
        draw_text(nav[i].label, {r.left + r.width * 0.5f, r.top + 7.f}, 14, theme_.text, false, 1);
        if (hover && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            nav[i].f();
            sf::sleep(sf::milliseconds(100));
        }
    }
    y = btnY + 44.f;

    // Action buttons
    float ay = y;
    float aw = (pw - 6.f) / 2.f;
    struct ActionBtn { std::string label; std::function<void()> f; };
    std::vector<ActionBtn> actions = {
        {"Hint",      [&]{ do_hint(); }},
        {"Best Move", [&]{ do_show_best(); }},
        {"Redo",      [&]{ redo(); }},
        {"Restart",   [&]{ reset_game(); }},
        {"FEN",       [&]{
            fen_buffer_ = board_.to_fen();
            fen_error_.clear();
            screen_ = Screen::FenDialog;
        }},
        {"Draw",      [&]{
            game_over_flag_ = true;
            result_string_ = "Draw by agreement";
            stats_.games_played++; stats_.draws++;
            io::save_stats(stats_);
            screen_ = Screen::GameOver;
        }},
        {"Resign",    [&]{
            game_over_flag_ = true;
            Color resigner = board_.side_to_move();
            result_string_ = (resigner == Color::White
                              ? "Black wins by resignation"
                              : "White wins by resignation");
            if (mode_ == GameMode::PvC) {
                bool humanLost = (resigner == Color::White) == human_plays_white_;
                if (humanLost) stats_.losses++; else stats_.wins++;
            }
            stats_.games_played++;
            io::save_stats(stats_);
            screen_ = Screen::GameOver;
        }},
    };
    for (size_t i = 0; i < actions.size(); ++i) {
        sf::FloatRect r(px + (i % 2) * (aw + 6.f), ay + (i / 2) * 40.f, aw, 32.f);
        bool hover = r.contains(mouse);
        draw_panel(r, hover ? theme_.panel_alt : theme_.card);
        draw_text(actions[i].label,
                  {r.left + r.width * 0.5f, r.top + 7.f}, 14, theme_.text, false, 1);
        if (hover && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            actions[i].f();
            sf::sleep(sf::milliseconds(100));
        }
    }
    y = ay + 4 * 40.f + 8.f;   // 8 buttons → 4 rows

    if (hint_active_ && !hint_text_.empty()) {
        draw_panel({px, y, pw, 56.f}, theme_.panel_alt);
        draw_text_wrapped(hint_text_, {px + 12.f, y + 8.f, pw - 24.f, 44.f}, 13, theme_.accent);
        y += 64.f;
    }

    // Engine analysis panel (optional)
    if (show_engine_panel_ && y + 130.f < side_panel_rect_.top + side_panel_rect_.height) {
        float eh = std::min(140.f, side_panel_rect_.top + side_panel_rect_.height - y - 8.f);
        draw_panel({px, y, pw, eh}, theme_.panel_alt);
        draw_text("Engine Analysis", {px + 12.f, y + 8.f}, 13, theme_.text_dim, true);
        std::string ev;
        if (last_result_.depth > 0) {
            double cp = last_result_.score_cp / 100.0;
            ev = (cp >= 0 ? "+" : "") + std::to_string(cp).substr(0, 4);
        }
        draw_text(ev, {px + pw - 12.f, y + 8.f}, 20, theme_.accent, true, 2);
        int ly = static_cast<int>(y) + 34;
        draw_text("Depth " + std::to_string(last_result_.depth) +
                  "   Nodes " + std::to_string(last_result_.nodes),
                  {px + 12.f, static_cast<float>(ly)}, 12, theme_.text_dim);
        ly += 18;
        draw_text("Time " + std::to_string(last_result_.time_ms) + "ms",
                  {px + 12.f, static_cast<float>(ly)}, 12, theme_.text_dim);
        ly += 22;
        draw_text("Best: " + last_result_.best_san,
                  {px + 12.f, static_cast<float>(ly)}, 13, theme_.text);
    }

    // Menu button
    Button back; back.label = "Menu"; back.rect = {20.f, H - 44.f, 100.f, 32.f};
    std::vector<Button> btn = {back};
    draw_buttons(btn, mouse);
    if (back.rect.contains(mouse) && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
        screen_ = Screen::MainMenu;
        sf::sleep(sf::milliseconds(120));
    }
}

void App::draw_analysis_screen() {
    draw_game_screen();
}

// =============================================================
// Promotion dialog
// =============================================================
void App::draw_promotion_dialog() {
    auto size = window_.getSize();
    sf::Vector2f mouse = window_.mapPixelToCoords(sf::Mouse::getPosition(window_));
    float W = static_cast<float>(size.x), H = static_cast<float>(size.y);

    sf::RectangleShape dim({W, H});
    dim.setFillColor(sf::Color(0, 0, 0, 140));
    window_.draw(dim);

    float bw = 90.f;
    float total = 4 * bw;
    float bx = W * 0.5f - total * 0.5f;
    float by = H * 0.5f - bw * 0.5f;
    draw_panel({bx - 20.f, by - 20.f, total + 40.f, bw + 40.f}, theme_.panel);

    Color c = board_.side_to_move();
    PieceType choices[4] = {PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight};
    for (int i = 0; i < 4; ++i) {
        sf::FloatRect r(bx + i * bw, by, bw, bw);
        bool hover = r.contains(mouse);
        if (hover) {
            sf::RectangleShape hl({r.width, r.height});
            hl.setPosition(r.left, r.top);
            hl.setFillColor(theme_.accent_dim);
            window_.draw(hl);
        }
        Piece p; p.type = choices[i]; p.color = c;
        draw_piece(window_, p, r.left + 10.f, r.top + 10.f, bw - 20.f);
        if (hover && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            if (pending_promo_) {
                pending_promo_->promo = choices[i];
                auto m = *pending_promo_;
                pending_promo_.reset();
                screen_ = Screen::Game;
                commit_move(m);
                sf::sleep(sf::milliseconds(100));
                return;
            }
        }
    }
    draw_text("Promote to:", {W * 0.5f, by - 50.f}, 18, theme_.text, true, 1);
}

// =============================================================
// FEN dialog
// =============================================================
void App::draw_fen_dialog() {
    auto size = window_.getSize();
    sf::Vector2f mouse = window_.mapPixelToCoords(sf::Mouse::getPosition(window_));
    float W = static_cast<float>(size.x), H = static_cast<float>(size.y);

    sf::RectangleShape dim({W, H});
    dim.setFillColor(sf::Color(0, 0, 0, 140));
    window_.draw(dim);

    float pw = 640.f, ph = 260.f;
    float sx = W * 0.5f - pw * 0.5f;
    float sy = H * 0.5f - ph * 0.5f;
    draw_panel({sx, sy, pw, ph}, theme_.panel);

    draw_text("Load FEN", {sx + 24.f, sy + 20.f}, 22, theme_.text, true);
    draw_text("Paste a valid FEN string below:",
              {sx + 24.f, sy + 56.f}, 14, theme_.text_dim);

    sf::FloatRect inputRect(sx + 24.f, sy + 90.f, pw - 48.f, 44.f);
    draw_panel(inputRect, theme_.panel_alt);
    draw_text(fen_buffer_.empty() ? "(paste here)" : fen_buffer_,
              {inputRect.left + 12.f, inputRect.top + 10.f}, 14,
              fen_buffer_.empty() ? theme_.text_dim : theme_.text);

    if (!fen_error_.empty()) {
        draw_text(fen_error_, {sx + 24.f, sy + 144.f}, 13, theme_.bad);
    }

    auto mkButton = [&](const std::string& lbl, float x, float y, float w, float h,
                        bool primary, std::function<void()> f) {
        sf::FloatRect r(x, y, w, h);
        bool hover = r.contains(mouse);
        sf::Color fill = primary ? theme_.accent : theme_.card;
        if (hover) fill = primary ? theme_.accent_dim : theme_.panel_alt;
        sf::RectangleShape s({w, h});
        s.setPosition(x, y);
        s.setFillColor(fill);
        s.setOutlineThickness(1.f);
        s.setOutlineColor(theme_.border);
        window_.draw(s);
        draw_text(lbl, {x + w * 0.5f, y + h * 0.5f - 10.f}, 16,
                  primary ? sf::Color::White : theme_.text, primary, 1);
        if (hover && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            f();
            sf::sleep(sf::milliseconds(100));
        }
    };

    mkButton("Cancel", sx + 24.f, sy + ph - 60.f, 140.f, 40.f, false, [&]{
        screen_ = Screen::Game;
        fen_error_.clear();
    });
    mkButton("Load", sx + pw - 164.f, sy + ph - 60.f, 140.f, 40.f, true, [&]{
        std::string err;
        Board tmp;
        if (tmp.set_fen(fen_buffer_, &err)) {
            board_ = tmp;
            board_.clear_history();
            history_.clear();
            redo_.clear();
            view_index_ = 0;
            selected_ = NO_SQUARE;
            legal_from_selected_.clear();
            last_from_ = last_to_ = NO_SQUARE;
            game_over_flag_ = false;
            fen_error_.clear();
            refresh_opening();
            screen_ = Screen::Game;
        } else {
            fen_error_ = "Invalid FEN: " + err;
        }
    });
}

// =============================================================
// Game over
// =============================================================
void App::draw_game_over() {
    auto size = window_.getSize();
    sf::Vector2f mouse = window_.mapPixelToCoords(sf::Mouse::getPosition(window_));
    float W = static_cast<float>(size.x), H = static_cast<float>(size.y);

    sf::RectangleShape dim({W, H});
    dim.setFillColor(sf::Color(0, 0, 0, 150));
    window_.draw(dim);

    float pw = 520.f, ph = 260.f;
    float sx = W * 0.5f - pw * 0.5f;
    float sy = H * 0.5f - ph * 0.5f;
    draw_panel({sx, sy, pw, ph}, theme_.panel);
    draw_text("Game Over", {sx + pw * 0.5f, sy + 30.f}, 30, theme_.text, true, 1);
    draw_text_wrapped(result_string_, {sx + 40.f, sy + 90.f, pw - 80.f, 60.f},
                      18, theme_.accent);

    auto mkButton = [&](const std::string& lbl, float x, float y, float w, float h,
                        bool primary, std::function<void()> f) {
        sf::FloatRect r(x, y, w, h);
        bool hover = r.contains(mouse);
        sf::Color fill = primary ? theme_.accent : theme_.card;
        if (hover) fill = primary ? theme_.accent_dim : theme_.panel_alt;
        sf::RectangleShape s({w, h});
        s.setPosition(x, y);
        s.setFillColor(fill);
        s.setOutlineThickness(1.f);
        s.setOutlineColor(theme_.border);
        window_.draw(s);
        draw_text(lbl, {x + w * 0.5f, y + h * 0.5f - 10.f}, 16,
                  primary ? sf::Color::White : theme_.text, primary, 1);
        if (hover && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            f();
            sf::sleep(sf::milliseconds(120));
        }
    };

    mkButton("New Game", sx + 30.f, sy + ph - 70.f, 200.f, 44.f, true, [&]{
        reset_game();
        screen_ = Screen::Game;
        if (mode_ == GameMode::CvC || (mode_ == GameMode::PvC && !human_plays_white_))
            maybe_start_engine_move();
    });
    mkButton("Main Menu", sx + pw - 230.f, sy + ph - 70.f, 200.f, 44.f, false, [&]{
        screen_ = Screen::MainMenu;
    });
}

// =============================================================
// Pause
// =============================================================
void App::draw_pause() {
    draw_game_screen();
    auto size = window_.getSize();
    sf::Vector2f mouse = window_.mapPixelToCoords(sf::Mouse::getPosition(window_));
    float W = static_cast<float>(size.x), H = static_cast<float>(size.y);

    sf::RectangleShape dim({W, H});
    dim.setFillColor(sf::Color(0, 0, 0, 150));
    window_.draw(dim);

    float pw = 420.f, ph = 220.f;
    float sx = W * 0.5f - pw * 0.5f;
    float sy = H * 0.5f - ph * 0.5f;
    draw_panel({sx, sy, pw, ph}, theme_.panel);
    draw_text("Paused", {sx + pw * 0.5f, sy + 30.f}, 28, theme_.text, true, 1);

    auto mkButton = [&](const std::string& lbl, float x, float y, float w, float h,
                        bool primary, std::function<void()> f) {
        sf::FloatRect r(x, y, w, h);
        bool hover = r.contains(mouse);
        sf::Color fill = primary ? theme_.accent : theme_.card;
        if (hover) fill = primary ? theme_.accent_dim : theme_.panel_alt;
        sf::RectangleShape s({w, h});
        s.setPosition(x, y);
        s.setFillColor(fill);
        s.setOutlineThickness(1.f);
        s.setOutlineColor(theme_.border);
        window_.draw(s);
        draw_text(lbl, {x + w * 0.5f, y + h * 0.5f - 10.f}, 16,
                  primary ? sf::Color::White : theme_.text, primary, 1);
        if (hover && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            f();
            sf::sleep(sf::milliseconds(120));
        }
    };

    mkButton("Resume", sx + 30.f, sy + ph - 70.f, 160.f, 44.f, true, [&]{
        screen_ = Screen::Game;
    });
    mkButton("Menu", sx + pw - 190.f, sy + ph - 70.f, 160.f, 44.f, false, [&]{
        screen_ = Screen::MainMenu;
    });
}

// =============================================================
// Puzzle
// =============================================================
void App::load_puzzle(int idx) {
    if (puzzles_.empty()) return;
    idx = std::clamp(idx, 0, static_cast<int>(puzzles_.size()) - 1);
    puzzle_index_ = idx;
    puzzle_attempts_ = 0;
    std::string err;
    if (!board_.set_fen(puzzles_[idx].fen, &err)) return;
    board_.clear_history();
    history_.clear();
    redo_.clear();
    view_index_ = 0;
    selected_ = NO_SQUARE;
    legal_from_selected_.clear();
    game_over_flag_ = false;
    mode_ = GameMode::Puzzle;
}

void App::check_puzzle_result() {
    if (puzzles_.empty() || history_.empty()) return;
    const auto& last = history_.back().move;
    std::string uci = move_to_uci(last);
    if (uci == puzzles_[puzzle_index_].best_move_uci) {
        stats_.puzzles_solved++;
        stats_.puzzles_attempted++;
        io::save_stats(stats_);
        hint_text_ = "Correct! Well done.";
        hint_active_ = true;
    } else {
        puzzle_attempts_++;
    }
}

// =============================================================
// PGN
// =============================================================
std::string App::export_pgn() const {
    std::string pgn;
    pgn += "[Event \"Casual Game\"]\n";
    pgn += "[Site \"Chess C++ App\"]\n";
    pgn += "[White \"" + std::string(
        (mode_ == GameMode::PvC && !human_plays_white_) ? "Computer" : "Player") + "\"]\n";
    pgn += "[Black \"" + std::string(
        (mode_ == GameMode::PvC && human_plays_white_) ? "Computer" : "Player") + "\"]\n";
    pgn += "[Result \"" +
        std::string(result_string_.find("White") != std::string::npos ? "1-0" :
                    result_string_.find("Black") != std::string::npos ? "0-1" : "*") + "\"]\n\n";
    for (size_t i = 0; i < history_.size(); ++i) {
        if (i % 2 == 0) pgn += std::to_string(i / 2 + 1) + ". ";
        pgn += history_[i].san + " ";
        if ((i + 1) % 12 == 0) pgn += "\n";
    }
    return pgn;
}

} // namespace ui