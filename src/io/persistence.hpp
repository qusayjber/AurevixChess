#pragma once
#include <string>
#include <vector>
#include <map>

namespace io {

struct Statistics {
    int games_played = 0;
    int wins = 0;
    int losses = 0;
    int draws = 0;
    int puzzles_attempted = 0;
    int puzzles_solved = 0;
    int total_move_count = 0;
    int last_difficulty = 2;
};

struct Settings {
    std::string board_theme = "Classic";
    std::string piece_style = "Default";
    bool dark_mode = true;
    bool sound_enabled = true;
    float sound_volume = 0.5f;
    bool show_coordinates = true;
    bool show_legal_moves = true;
    float animation_speed = 1.0f;
    bool show_engine_panel = false;
    bool reduced_motion = false;
    std::string time_control = "10+0";
};

Settings load_settings();
void save_settings(const Settings& s);

Statistics load_stats();
void save_stats(const Statistics& s);

// Save/load current game (FEN + move list in UCI + metadata)
void save_current_game(const std::string& fen, const std::vector<std::string>& moves_uci, const std::string& tag = "autosave");
std::pair<std::string, std::vector<std::string>> load_current_game(const std::string& tag = "autosave");

// Save/load recent game result summary
struct GameRecord {
    std::string white, black, result, opening, date;
    int moves = 0;
};
void append_game_record(const GameRecord& r);
std::vector<GameRecord> load_recent_games();

} // namespace io