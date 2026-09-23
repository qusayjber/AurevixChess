#include "io/persistence.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <ctime>
#include <algorithm>

namespace io {

namespace fs = std::filesystem;

static fs::path data_dir() {
    fs::path d = "chess_data";
    std::error_code ec;
    fs::create_directories(d, ec);
    return d;
}

// ---------------- Settings ----------------
Settings load_settings() {
    Settings s;
    std::ifstream in(data_dir() / "settings.txt");
    if (!in) return s;
    std::string line;
    while (std::getline(in, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        try {
            if (key == "board_theme") s.board_theme = val;
            else if (key == "piece_style") s.piece_style = val;
            else if (key == "dark_mode") s.dark_mode = (val == "1");
            else if (key == "sound_enabled") s.sound_enabled = (val == "1");
            else if (key == "sound_volume") s.sound_volume = std::stof(val);
            else if (key == "show_coordinates") s.show_coordinates = (val == "1");
            else if (key == "show_legal_moves") s.show_legal_moves = (val == "1");
            else if (key == "animation_speed") s.animation_speed = std::stof(val);
            else if (key == "show_engine_panel") s.show_engine_panel = (val == "1");
            else if (key == "reduced_motion") s.reduced_motion = (val == "1");
            else if (key == "time_control") s.time_control = val;
        } catch (...) {
            // ignore malformed values
        }
    }
    return s;
}

void save_settings(const Settings& s) {
    std::ofstream out(data_dir() / "settings.txt");
    out << "board_theme=" << s.board_theme << "\n";
    out << "piece_style=" << s.piece_style << "\n";
    out << "dark_mode=" << (s.dark_mode ? 1 : 0) << "\n";
    out << "sound_enabled=" << (s.sound_enabled ? 1 : 0) << "\n";
    out << "sound_volume=" << s.sound_volume << "\n";
    out << "show_coordinates=" << (s.show_coordinates ? 1 : 0) << "\n";
    out << "show_legal_moves=" << (s.show_legal_moves ? 1 : 0) << "\n";
    out << "animation_speed=" << s.animation_speed << "\n";
    out << "show_engine_panel=" << (s.show_engine_panel ? 1 : 0) << "\n";
    out << "reduced_motion=" << (s.reduced_motion ? 1 : 0) << "\n";
    out << "time_control=" << s.time_control << "\n";
}

// ---------------- Stats ----------------
Statistics load_stats() {
    Statistics s;
    std::ifstream in(data_dir() / "stats.txt");
    if (!in) return s;
    std::string line;
    while (std::getline(in, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        try {
            int v = std::stoi(val);
            if (key == "games_played") s.games_played = v;
            else if (key == "wins") s.wins = v;
            else if (key == "losses") s.losses = v;
            else if (key == "draws") s.draws = v;
            else if (key == "puzzles_attempted") s.puzzles_attempted = v;
            else if (key == "puzzles_solved") s.puzzles_solved = v;
            else if (key == "total_move_count") s.total_move_count = v;
            else if (key == "last_difficulty") s.last_difficulty = v;
        } catch (...) {}
    }
    return s;
}

void save_stats(const Statistics& s) {
    std::ofstream out(data_dir() / "stats.txt");
    out << "games_played=" << s.games_played << "\n";
    out << "wins=" << s.wins << "\n";
    out << "losses=" << s.losses << "\n";
    out << "draws=" << s.draws << "\n";
    out << "puzzles_attempted=" << s.puzzles_attempted << "\n";
    out << "puzzles_solved=" << s.puzzles_solved << "\n";
    out << "total_move_count=" << s.total_move_count << "\n";
    out << "last_difficulty=" << s.last_difficulty << "\n";
}

// ---------------- Current game ----------------
void save_current_game(const std::string& fen, const std::vector<std::string>& moves, const std::string& tag) {
    std::ofstream out(data_dir() / (tag + ".game"));
    out << "FEN " << fen << "\n";
    for (const auto& m : moves) out << "MOVE " << m << "\n";
}

std::pair<std::string, std::vector<std::string>> load_current_game(const std::string& tag) {
    std::ifstream in(data_dir() / (tag + ".game"));
    std::string fen;
    std::vector<std::string> moves;
    if (!in) return {fen, moves};
    std::string line;
    while (std::getline(in, line)) {
        if (line.rfind("FEN ", 0) == 0) fen = line.substr(4);
        else if (line.rfind("MOVE ", 0) == 0) moves.push_back(line.substr(5));
    }
    return {fen, moves};
}

// ---------------- Recent games ----------------
void append_game_record(const GameRecord& r) {
    std::ofstream out(data_dir() / "recent.txt", std::ios::app);
    out << r.date << '|' << r.white << '|' << r.black << '|' << r.result
        << '|' << r.opening << '|' << r.moves << "\n";
}

std::vector<GameRecord> load_recent_games() {
    std::vector<GameRecord> out;
    std::ifstream in(data_dir() / "recent.txt");
    std::string line;
    while (std::getline(in, line)) {
        GameRecord r;
        std::stringstream ss(line);
        std::string tok;
        std::vector<std::string> parts;
        while (std::getline(ss, tok, '|')) parts.push_back(tok);
        if (parts.size() >= 6) {
            r.date = parts[0]; r.white = parts[1]; r.black = parts[2];
            r.result = parts[3]; r.opening = parts[4];
            try { r.moves = std::stoi(parts[5]); } catch (...) { r.moves = 0; }
            out.push_back(r);
        }
    }
    // Keep most recent 25
    if (out.size() > 25) out.erase(out.begin(), out.end() - 25);
    return out;
}

} // namespace io