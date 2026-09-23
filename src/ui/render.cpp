#include "ui/render.hpp"
#include <algorithm>
#include <cmath>
#include <string>

namespace ui {
using namespace chess;

// ---------------------------------------------------------------------
// Coordinate font (set once from App; nullptr disables coordinates)
// ---------------------------------------------------------------------
namespace {
const sf::Font* g_coord_font = nullptr;
}

void set_coordinate_font(const sf::Font* f) { g_coord_font = f; }

// ---------------------------------------------------------------------
// BoardView
// ---------------------------------------------------------------------
sf::Vector2f BoardView::square_pos(Square s) const {
    int f = file_of(s), r = rank_of(s);
    if (flipped) { f = 7 - f; r = 7 - r; }
    return { rect.left + f * square_size(), rect.top + (7 - r) * square_size() };
}

Square BoardView::square_at(sf::Vector2f p) const {
    if (!rect.contains(p)) return NO_SQUARE;
    int col = static_cast<int>((p.x - rect.left) / square_size());
    int row = static_cast<int>((p.y - rect.top ) / square_size());
    if (col < 0 || col > 7 || row < 0 || row > 7) return NO_SQUARE;
    int f = col, r = 7 - row;
    if (flipped) { f = 7 - f; r = 7 - r; }
    return mk_square(f, r);
}

// =====================================================================
// Private helpers
// =====================================================================
namespace {

sf::Color mix(sf::Color a, sf::Color b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    return sf::Color(
        static_cast<sf::Uint8>(a.r * (1.f - t) + b.r * t),
        static_cast<sf::Uint8>(a.g * (1.f - t) + b.g * t),
        static_cast<sf::Uint8>(a.b * (1.f - t) + b.b * t),
        static_cast<sf::Uint8>(a.a * (1.f - t) + b.a * t));
}

// Draw four L-shaped corner brackets (modern "focus" indicator).
void draw_corner_brackets(sf::RenderTarget& target, const sf::FloatRect& r,
                          sf::Color color, float length, float thickness,
                          float inset) {
    const float x0 = r.left + inset;
    const float y0 = r.top  + inset;
    const float x1 = r.left + r.width  - inset;
    const float y1 = r.top  + r.height - inset;

    sf::RectangleShape bar({length, thickness});
    bar.setFillColor(color);

    // top-left
    bar.setSize({length, thickness}); bar.setPosition(x0, y0);                target.draw(bar);
    bar.setSize({thickness, length}); bar.setPosition(x0, y0);                target.draw(bar);
    // top-right
    bar.setSize({length, thickness}); bar.setPosition(x1 - length, y0);       target.draw(bar);
    bar.setSize({thickness, length}); bar.setPosition(x1 - thickness, y0);    target.draw(bar);
    // bottom-left
    bar.setSize({length, thickness}); bar.setPosition(x0, y1 - thickness);    target.draw(bar);
    bar.setSize({thickness, length}); bar.setPosition(x0, y1 - length);       target.draw(bar);
    // bottom-right
    bar.setSize({length, thickness}); bar.setPosition(x1 - length, y1 - thickness); target.draw(bar);
    bar.setSize({thickness, length}); bar.setPosition(x1 - thickness, y1 - length); target.draw(bar);
}

// Ring with centre `c`, radius `radius`, given outline thickness.
void draw_soft_ring(sf::RenderTarget& target, sf::Vector2f c, float radius,
                    float thickness, sf::Color color) {
    sf::CircleShape s(radius, 64);
    s.setOrigin(radius, radius);
    s.setPosition(c);
    s.setFillColor(sf::Color::Transparent);
    s.setOutlineColor(color);
    s.setOutlineThickness(thickness);
    target.draw(s);
}

} // namespace

// =====================================================================
// Board
// =====================================================================
void draw_board(sf::RenderTarget& target, const BoardView& view, const Palette& pal,
                bool show_coords, bool dark) {
    const float ss = view.square_size();
    const sf::FloatRect r = view.rect;

    // ---- Soft multi-layer drop shadow ----
    for (int i = 0; i < 4; ++i) {
        const float off = 3.f + i * 3.f;
        const int alpha = 55 - i * 12;
        sf::RectangleShape sh({r.width + off * 2.f, r.height + off * 2.f});
        sh.setPosition(r.left - off, r.top - off + 3.f);
        sh.setFillColor(sf::Color(0, 0, 0, std::max(0, alpha)));
        target.draw(sh);
    }

    // ---- Thin outer frame ----
    {
        sf::RectangleShape frame({r.width + 6.f, r.height + 6.f});
        frame.setPosition(r.left - 3.f, r.top - 3.f);
        frame.setFillColor(sf::Color::Transparent);
        frame.setOutlineColor(sf::Color(0, 0, 0, 90));
        frame.setOutlineThickness(2.f);
        target.draw(frame);
    }

    // ---- Squares (with subtle inner gradient feel) ----
    sf::RectangleShape sq;
    sq.setOutlineThickness(0);
    for (int rk = 0; rk < 8; ++rk) {
        for (int f = 0; f < 8; ++f) {
            const Square s = mk_square(f, rk);
            const sf::Vector2f p = view.square_pos(s);
            const bool light = ((f + rk) & 1) == 1;

            const sf::Color base = light ? pal.light_sq : pal.dark_sq;

            sq.setPosition(p);
            sq.setSize({ss, ss});
            sq.setFillColor(base);
            target.draw(sq);

            // Top highlight strip (very subtle)
            {
                const sf::Color tone = mix(base, sf::Color(255, 255, 255),
                                           light ? 0.05f : 0.08f);
                sf::RectangleShape top({ss, ss * 0.42f});
                top.setPosition(p);
                top.setFillColor(sf::Color(tone.r, tone.g, tone.b, 42));
                target.draw(top);
            }
            // Bottom shadow strip (very subtle)
            {
                const sf::Color tone = mix(base, sf::Color(0, 0, 0),
                                           light ? 0.05f : 0.10f);
                sf::RectangleShape bot({ss, ss * 0.28f});
                bot.setPosition(p.x, p.y + ss * 0.72f);
                bot.setFillColor(sf::Color(tone.r, tone.g, tone.b, 42));
                target.draw(bot);
            }
        }
    }

    // ---- Coordinates ----
    if (show_coords && g_coord_font) {
        const unsigned fs = static_cast<unsigned>(std::max(9.f, ss * 0.20f));

        auto label_color = [&](bool light) {
            if (light)
                return dark ? sf::Color(72, 62, 52, 220)
                            : sf::Color(95, 85, 70, 220);
            return dark ? sf::Color(240, 235, 225, 220)
                        : sf::Color(248, 244, 236, 220);
        };

        // File letters a..h — bottom visual row
        for (int c = 0; c < 8; ++c) {
            const int file = view.flipped ? 7 - c : c;
            const int bottom_rank = view.flipped ? 7 : 0;
            const bool is_light = ((file + bottom_rank) & 1) == 1;

            const float x = r.left + c * ss + ss * 0.5f;
            const float y = r.top + r.height - ss * 0.02f - ss * 0.22f;

            sf::Text t(std::string(1, static_cast<char>('a' + file)),
                       *g_coord_font, fs);
            t.setFillColor(label_color(is_light));
            auto b = t.getLocalBounds();
            t.setPosition(x - b.width * 0.5f - b.left, y - b.top);
            target.draw(t);
        }

        // Rank numbers 1..8 — left visual column
        for (int rr = 0; rr < 8; ++rr) {
            const int rank = view.flipped ? rr : 7 - rr;
            const int left_file = view.flipped ? 7 : 0;
            const bool is_light = ((left_file + rank) & 1) == 1;

            const float x = r.left + ss * 0.06f;
            const float y = r.top  + rr * ss + ss * 0.06f;

            sf::Text t(std::to_string(rank + 1), *g_coord_font, fs);
            t.setFillColor(label_color(is_light));
            auto b = t.getLocalBounds();
            t.setPosition(x - b.left, y - b.top);
            target.draw(t);
        }
    }
}

// =====================================================================
// Last move
// =====================================================================
void draw_last_move(sf::RenderTarget& target, const BoardView& view, const Palette& pal,
                    Square from, Square to) {
    if (from == NO_SQUARE && to == NO_SQUARE) return;
    const float ss = view.square_size();
    const float inset = ss * 0.05f;

    auto paint = [&](Square s) {
        if (!valid_square(s)) return;
        const sf::Vector2f p = view.square_pos(s);

        // Soft tint
        sf::RectangleShape base({ss, ss});
        base.setPosition(p);
        base.setFillColor(pal.last_move);
        target.draw(base);

        // Thin inner border
        sf::Color bc = pal.last_move;
        bc.a = 190;
        sf::RectangleShape border({ss - inset * 2.f, ss - inset * 2.f});
        border.setPosition(p.x + inset, p.y + inset);
        border.setFillColor(sf::Color::Transparent);
        border.setOutlineColor(bc);
        border.setOutlineThickness(std::max(1.f, ss * 0.03f));
        target.draw(border);
    };
    paint(from);
    paint(to);
}

// =====================================================================
// Selected square — tint + corner brackets
// =====================================================================
void draw_selected(sf::RenderTarget& target, const BoardView& view, const Palette& pal,
                   Square s) {
    if (!valid_square(s)) return;
    const float ss = view.square_size();
    const sf::Vector2f p = view.square_pos(s);
    const sf::FloatRect rr(p.x, p.y, ss, ss);

    sf::RectangleShape base({ss, ss});
    base.setPosition(p);
    base.setFillColor(pal.selected);
    target.draw(base);

    sf::Color acc = pal.selected;
    acc.a = 255;
    draw_corner_brackets(target, rr, acc,
                         ss * 0.30f, std::max(2.f, ss * 0.06f),
                         ss * 0.05f);
}

// =====================================================================
// Legal-move indicator
// =====================================================================
void draw_legal_indicator(sf::RenderTarget& target, const BoardView& view,
                          const Palette& pal, Square s, bool capture) {
    if (!valid_square(s)) return;
    const sf::Vector2f p = view.square_pos(s);
    const float ss = view.square_size();
    const sf::Vector2f c = {p.x + ss * 0.5f, p.y + ss * 0.5f};

    if (capture) {
        // Outer glow
        sf::Color glow = pal.legal_capture; glow.a = 70;
        draw_soft_ring(target, c, ss * 0.44f, ss * 0.10f, glow);

        // Mid ring
        sf::Color mid = pal.legal_capture; mid.a = 150;
        draw_soft_ring(target, c, ss * 0.40f, ss * 0.06f, mid);

        // Bright solid ring
        sf::Color bright = pal.legal_capture; bright.a = 255;
        draw_soft_ring(target, c, ss * 0.34f,
                       std::max(2.f, ss * 0.035f), bright);

        // Small central dot
        sf::CircleShape dot(ss * 0.05f, 24);
        dot.setOrigin(ss * 0.05f, ss * 0.05f);
        dot.setPosition(c);
        dot.setFillColor(bright);
        target.draw(dot);
    } else {
        // Soft halo
        sf::CircleShape halo(ss * 0.20f, 40);
        halo.setOrigin(ss * 0.20f, ss * 0.20f);
        halo.setPosition(c);
        sf::Color gcol = pal.legal_dot; gcol.a = 60;
        halo.setFillColor(gcol);
        target.draw(halo);

        // Thin pale ring
        sf::Color ring_col = pal.legal_dot; ring_col.a = 130;
        draw_soft_ring(target, c, ss * 0.16f,
                       std::max(1.f, ss * 0.012f), ring_col);

        // Solid centre dot
        sf::CircleShape dot(ss * 0.115f, 32);
        dot.setOrigin(ss * 0.115f, ss * 0.115f);
        dot.setPosition(c);
        dot.setFillColor(pal.legal_dot);
        target.draw(dot);
    }
}

// =====================================================================
// Check ring
// =====================================================================
void draw_check_ring(sf::RenderTarget& target, const BoardView& view,
                     const Palette& pal, Square s) {
    if (!valid_square(s)) return;
    const sf::Vector2f p = view.square_pos(s);
    const float ss = view.square_size();
    const sf::Vector2f c = {p.x + ss * 0.5f, p.y + ss * 0.5f};

    const sf::Color base = pal.check_ring;

    sf::Color g1 = base; g1.a = 40;
    draw_soft_ring(target, c, ss * 0.52f, ss * 0.14f, g1);

    sf::Color g2 = base; g2.a = 90;
    draw_soft_ring(target, c, ss * 0.46f, ss * 0.08f, g2);

    sf::Color g3 = base; g3.a = 255;
    draw_soft_ring(target, c, ss * 0.42f,
                   std::max(2.f, ss * 0.045f), g3);
}

// =====================================================================
// Hint squares
// =====================================================================
void draw_hint_squares(sf::RenderTarget& target, const BoardView& view,
                       const Palette& pal,
                       Square from, Square to, float pulse) {
    pulse = std::clamp(pulse, 0.f, 1.f);
    const float ss = view.square_size();

    auto paint = [&](Square s, sf::Color base, bool outgoing) {
        if (!valid_square(s)) return;
        const sf::Vector2f p = view.square_pos(s);
        const sf::FloatRect rr(p.x, p.y, ss, ss);

        // Pulsing fill
        const int a = static_cast<int>((outgoing ? 70 : 55) + pulse * 60.f);
        sf::Color fill = base;
        fill.a = static_cast<sf::Uint8>(std::clamp(a, 0, 255));

        sf::RectangleShape r({ss, ss});
        r.setPosition(p);
        r.setFillColor(fill);
        target.draw(r);

        // Bright corner brackets
        sf::Color acc = base; acc.a = 255;
        draw_corner_brackets(target, rr, acc,
                             ss * 0.30f, std::max(2.f, ss * 0.06f),
                             ss * 0.05f);

        // Pulsing outer ring for the destination square only
        if (!outgoing) {
            sf::Color ring = base;
            ring.a = static_cast<sf::Uint8>(80 + pulse * 120.f);
            sf::RectangleShape outer({ss - 4.f, ss - 4.f});
            outer.setPosition(p.x + 2.f, p.y + 2.f);
            outer.setFillColor(sf::Color::Transparent);
            outer.setOutlineColor(ring);
            outer.setOutlineThickness(std::max(2.f, ss * 0.04f));
            target.draw(outer);
        }
    };

    paint(from, pal.hint_from, true);
    paint(to,   pal.hint_to,   false);
}

} // namespace ui