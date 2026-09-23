#pragma once
#include <SFML/Graphics.hpp>
#include "chess/chess.hpp"

namespace ui {

void draw_piece(sf::RenderTarget& target,
                chess::Piece piece,
                float x, float y, float size,
                bool outline_white = true);

} 