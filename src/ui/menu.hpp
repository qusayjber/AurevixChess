#pragma once
#include <SFML/Graphics.hpp>
#include "ui/theme.hpp"
#include <functional>
#include <string>
#include <vector>

namespace ui {

struct Button {
    sf::FloatRect rect;
    std::string  label;
    std::string  tooltip;
    bool         primary = false;
    bool         enabled = true;
    std::function<void()> on_click;
};

class MenuRenderer {
public:
    MenuRenderer(sf::Font& font) : font_(font) {}
    void draw(sf::RenderTarget& target, const AppTheme& theme,
              const std::string& title, const std::string& subtitle,
              const std::vector<Button>& buttons, sf::Vector2f mouse) const;

private:
    sf::Font& font_;
};

} // namespace ui