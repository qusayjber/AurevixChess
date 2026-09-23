#pragma once
#include <SFML/Graphics.hpp>
#include "chess/chess.hpp"
#include "ui/theme.hpp"
#include <optional>

namespace ui {

struct BoardView {
    sf::FloatRect rect;      // pixel rect of the 8x8 board
    bool flipped = false;    // true => black at bottom
    float square_size() const { return rect.width / 8.f; }
    sf::Vector2f square_pos(chess::Square s) const;
    chess::Square square_at(sf::Vector2f p) const;
};

// Draw board squares, coordinates and (optionally) highlights.
void draw_board(sf::RenderTarget& target, const BoardView& view, const Palette& pal,
                bool show_coords, bool dark);

// Highlight overlays
void draw_last_move(sf::RenderTarget& target, const BoardView& view, const Palette& pal,
                    chess::Square from, chess::Square to);
void draw_selected(sf::RenderTarget& target, const BoardView& view, const Palette& pal, chess::Square s);
void draw_legal_indicator(sf::RenderTarget& target, const BoardView& view, const Palette& pal,
                          chess::Square s, bool capture);
void draw_check_ring(sf::RenderTarget& target, const BoardView& view, const Palette& pal, chess::Square s);
void draw_hint_squares(sf::RenderTarget& target, const BoardView& view, const Palette& pal,
                       chess::Square from, chess::Square to, float pulse);

} // namespace ui