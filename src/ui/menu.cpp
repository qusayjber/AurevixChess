#include "ui/menu.hpp"
#include <algorithm>

namespace ui {

// =====================================================================
// MenuRenderer — draws a modern menu: title, subtitle, buttons, tooltips.
//
// ⚠️  This file must NOT define draw_piece() or any other helper that
//     lives in pieces.cpp / render.cpp. Defining them again causes a
//     duplicate-symbol error at link time.
// =====================================================================
void MenuRenderer::draw(sf::RenderTarget& target,
                        const AppTheme& theme,
                        const std::string& title,
                        const std::string& subtitle,
                        const std::vector<Button>& buttons,
                        sf::Vector2f mouse) const
{
    const sf::Vector2u size = target.getSize();
    const float W = static_cast<float>(size.x);
    const float H = static_cast<float>(size.y);

    // -----------------------------------------------------------------
    // Background
    // -----------------------------------------------------------------
    sf::RectangleShape bg({W, H});
    bg.setFillColor(theme.bg);
    target.draw(bg);

    // Thin accent strip below the title area
    {
        sf::RectangleShape accent({W, 3.f});
        accent.setPosition(0.f, H * 0.14f + 96.f);
        accent.setFillColor(sf::Color(theme.accent.r, theme.accent.g, theme.accent.b, 70));
        target.draw(accent);
    }

    // -----------------------------------------------------------------
    // Title
    // -----------------------------------------------------------------
    {
        sf::Text t(title, font_, 56);
        t.setFillColor(theme.text);
        sf::FloatRect b = t.getLocalBounds();
        t.setPosition(W * 0.5f - b.width * 0.5f - b.left, H * 0.14f);
        target.draw(t);
    }

    // -----------------------------------------------------------------
    // Subtitle (optional)
    // -----------------------------------------------------------------
    if (!subtitle.empty()) {
        sf::Text s(subtitle, font_, 18);
        s.setFillColor(theme.text_dim);
        sf::FloatRect b = s.getLocalBounds();
        s.setPosition(W * 0.5f - b.width * 0.5f - b.left, H * 0.14f + 66.f);
        target.draw(s);
    }

    // -----------------------------------------------------------------
    // Buttons
    // -----------------------------------------------------------------
    for (const auto& btn : buttons) {
        const bool hover   = btn.enabled && btn.rect.contains(mouse);
        const bool primary = btn.primary;

        // ---- Base fill ----
        sf::Color fill;
        if (primary) {
            fill = theme.accent;
            if (hover) {
                fill.r = static_cast<sf::Uint8>(std::min<int>(255, fill.r + 25));
                fill.g = static_cast<sf::Uint8>(std::min<int>(255, fill.g + 25));
                fill.b = static_cast<sf::Uint8>(std::min<int>(255, fill.b + 25));
            }
        } else {
            fill = hover ? theme.panel_alt : theme.card;
        }
        if (!btn.enabled) fill = theme.panel_alt;

        // ---- Drop shadow (enabled buttons only) ----
        if (btn.enabled) {
            sf::RectangleShape shadow({btn.rect.width, btn.rect.height});
            shadow.setPosition(btn.rect.left + 1.f, btn.rect.top + 3.f);
            shadow.setFillColor(sf::Color(0, 0, 0, theme.bg.r < 100 ? 90 : 30));
            target.draw(shadow);
        }

        // ---- Main rectangle ----
        sf::RectangleShape rect({btn.rect.width, btn.rect.height});
        rect.setPosition(btn.rect.left, btn.rect.top);
        rect.setFillColor(fill);
        rect.setOutlineThickness(1.f);
        rect.setOutlineColor(primary ? sf::Color::Transparent : theme.border);
        target.draw(rect);

        // ---- Left hover indicator bar ----
        if (hover && btn.enabled) {
            sf::RectangleShape bar({3.f, btn.rect.height});
            bar.setPosition(btn.rect.left, btn.rect.top);
            bar.setFillColor(primary ? sf::Color(255, 255, 255, 180) : theme.accent);
            target.draw(bar);
        }

        // ---- Label ----
        {
            sf::Text t(btn.label, font_, 17);
            t.setFillColor(primary ? sf::Color::White
                                   : (btn.enabled ? theme.text : theme.text_dim));
            sf::FloatRect b = t.getLocalBounds();
            t.setPosition(
                btn.rect.left + btn.rect.width  * 0.5f - b.width  * 0.5f - b.left,
                btn.rect.top  + btn.rect.height * 0.5f - b.height * 0.5f - b.top - 2.f);
            target.draw(t);
        }
    }

    // -----------------------------------------------------------------
    // Tooltip (only one at a time)
    // -----------------------------------------------------------------
    for (const auto& btn : buttons) {
        if (btn.enabled && btn.rect.contains(mouse) && !btn.tooltip.empty()) {
            sf::Text tip(btn.tooltip, font_, 13);
            sf::FloatRect tb = tip.getLocalBounds();

            const float pad = 8.f;
            const float tw = tb.width + pad * 2.f;
            const float th = tb.height + pad * 2.f;

            float tx = mouse.x + 16.f;
            float ty = mouse.y + 20.f;
            if (tx + tw > W - 8.f) tx = W - 8.f - tw;
            if (ty + th > H - 8.f) ty = H - 8.f - th;

            sf::RectangleShape box({tw, th});
            box.setPosition(tx, ty);
            box.setFillColor(sf::Color(0, 0, 0, 220));
            box.setOutlineThickness(1.f);
            box.setOutlineColor(sf::Color(theme.accent.r, theme.accent.g, theme.accent.b, 180));
            target.draw(box);

            tip.setFillColor(sf::Color(240, 240, 245));
            tip.setPosition(tx + pad - tb.left, ty + pad - tb.top);
            target.draw(tip);
            break;
        }
    }
}

} // namespace ui