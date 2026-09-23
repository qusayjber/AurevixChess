#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include <string>

namespace ui {

enum class BoardTheme { Classic, Midnight, Ivory, Emerald, Graphite, Tournament };

struct Palette {
    sf::Color light_sq;
    sf::Color dark_sq;
    sf::Color light_sq_border;
    sf::Color dark_sq_border;
    sf::Color last_move;
    sf::Color selected;
    sf::Color legal_dot;
    sf::Color legal_capture;
    sf::Color check_ring;
    sf::Color hint_from;
    sf::Color hint_to;
};

Palette palette_for(BoardTheme t);

struct AppTheme {
    sf::Color bg;
    sf::Color panel;
    sf::Color panel_alt;
    sf::Color card;
    sf::Color text;
    sf::Color text_dim;
    sf::Color accent;
    sf::Color accent_dim;
    sf::Color border;
    sf::Color good;
    sf::Color bad;
    sf::Color warn;
};

AppTheme app_theme(bool dark);

} // namespace ui