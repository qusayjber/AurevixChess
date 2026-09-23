#include "ui/theme.hpp"

namespace ui {

// =====================================================================
// Board themes
// =====================================================================
Palette palette_for(BoardTheme t) {
    Palette p;

    switch (t) {
    // ---------------------------------------------------------------
    // CLASSIC — Warm wooden tournament board (Staunton / FIDE style)
    // Honey-maple light squares + rich walnut dark squares.
    // ---------------------------------------------------------------
    case BoardTheme::Classic:
        p.light_sq        = sf::Color(238, 216, 180);   // aged maple
        p.dark_sq         = sf::Color(176, 126,  80);   // warm walnut
        p.light_sq_border = sf::Color(206, 184, 148);
        p.dark_sq_border  = sf::Color(146,  98,  58);

        p.last_move       = sf::Color(212, 198, 118, 155);  // soft gold
        p.selected        = sf::Color(118, 152, 108, 205);  // moss green
        p.legal_dot       = sf::Color( 62,  60,  56, 165);  // ink
        p.legal_capture   = sf::Color(196,  78,  78, 210);  // brick red
        p.check_ring      = sf::Color(216,  62,  62, 235);  // signal red

        p.hint_from       = sf::Color(118, 186, 240, 200);  // sky
        p.hint_to         = sf::Color( 62, 132, 226, 225);  // royal blue
        break;

    // ---------------------------------------------------------------
    // MIDNIGHT — Modern slate-blue with deep indigo shadows
    // ---------------------------------------------------------------
    case BoardTheme::Midnight:
        p.light_sq        = sf::Color(112, 128, 158);   // slate mist
        p.dark_sq         = sf::Color( 54,  68,  98);   // deep navy
        p.light_sq_border = sf::Color( 86, 100, 130);
        p.dark_sq_border  = sf::Color( 38,  50,  76);

        p.last_move       = sf::Color(128, 190, 240, 130);  // ice
        p.selected        = sf::Color( 96, 158, 232, 205);  // electric blue
        p.legal_dot       = sf::Color(236, 240, 250, 190);  // pale white
        p.legal_capture   = sf::Color(248, 118, 138, 215);  // coral
        p.check_ring      = sf::Color(255,  88,  96, 235);

        p.hint_from       = sf::Color(132, 224, 255, 200);
        p.hint_to         = sf::Color( 72, 168, 255, 225);
        break;

    // ---------------------------------------------------------------
    // IVORY — Light, airy, elegant (Lichess-paper style)
    // ---------------------------------------------------------------
    case BoardTheme::Ivory:
        p.light_sq        = sf::Color(252, 250, 242);   // paper
        p.dark_sq         = sf::Color(216, 210, 190);   // cream
        p.light_sq_border = sf::Color(232, 228, 216);
        p.dark_sq_border  = sf::Color(196, 190, 170);

        p.last_move       = sf::Color(232, 216, 128, 160);  // butter
        p.selected        = sf::Color(168, 178, 140, 190);  // sage
        p.legal_dot       = sf::Color( 88,  86,  78, 175);  // pencil
        p.legal_capture   = sf::Color(200,  98,  98, 205);
        p.check_ring      = sf::Color(222,  78,  78, 230);

        p.hint_from       = sf::Color(150, 202, 254, 200);
        p.hint_to         = sf::Color( 92, 156, 244, 225);
        break;

    // ---------------------------------------------------------------
    // EMERALD — Forest green, classic British chess club
    // ---------------------------------------------------------------
    case BoardTheme::Emerald:
        p.light_sq        = sf::Color(234, 234, 204);   // pale olive
        p.dark_sq         = sf::Color(108, 142,  84);   // forest
        p.light_sq_border = sf::Color(208, 208, 178);
        p.dark_sq_border  = sf::Color( 86, 116,  62);

        p.last_move       = sf::Color(244, 220, 108, 155);  // amber
        p.selected        = sf::Color(190, 218, 122, 195);  // spring
        p.legal_dot       = sf::Color( 54,  74,  38, 185);  // moss ink
        p.legal_capture   = sf::Color(202,  86,  76, 210);
        p.check_ring      = sf::Color(222,  70,  70, 235);

        p.hint_from       = sf::Color(146, 216, 250, 200);
        p.hint_to         = sf::Color( 76, 156, 240, 225);
        break;

    // ---------------------------------------------------------------
    // GRAPHITE — Industrial cool-gray with steel accent
    // ---------------------------------------------------------------
    case BoardTheme::Graphite:
        p.light_sq        = sf::Color(190, 192, 196);   // brushed steel
        p.dark_sq         = sf::Color(102, 106, 112);   // gunmetal
        p.light_sq_border = sf::Color(160, 162, 168);
        p.dark_sq_border  = sf::Color( 82,  86,  92);

        p.last_move       = sf::Color(224, 224, 130, 145);  // yellow flag
        p.selected        = sf::Color(132, 178, 226, 195);  // sky steel
        p.legal_dot       = sf::Color(248, 250, 252, 175);  // white ink
        p.legal_capture   = sf::Color(234, 106, 106, 215);
        p.check_ring      = sf::Color(244,  92,  92, 235);

        p.hint_from       = sf::Color(148, 224, 255, 200);
        p.hint_to         = sf::Color( 82, 166, 250, 225);
        break;

    // ---------------------------------------------------------------
    // TOURNAMENT — Official cool blue (chess.com / FIDE broadcast)
    // ---------------------------------------------------------------
    case BoardTheme::Tournament:
        p.light_sq        = sf::Color(236, 238, 242);   // chalk
        p.dark_sq         = sf::Color(116, 132, 154);   // muted slate
        p.light_sq_border = sf::Color(212, 216, 222);
        p.dark_sq_border  = sf::Color( 92, 108, 130);

        p.last_move       = sf::Color(255, 232, 118, 150);  // highlighter
        p.selected        = sf::Color( 84, 148, 224, 190);  // vivid blue
        p.legal_dot       = sf::Color( 52,  62,  76, 175);  // ink
        p.legal_capture   = sf::Color(214,  84,  84, 210);
        p.check_ring      = sf::Color(226,  68,  68, 235);

        p.hint_from       = sf::Color(140, 208, 254, 200);
        p.hint_to         = sf::Color( 72, 152, 240, 225);
        break;
    }
    return p;
}

// =====================================================================
// Application themes (light / dark)
// =====================================================================
AppTheme app_theme(bool dark) {
    AppTheme t;

    if (dark) {
        // Deep blue-black background with a subtle warm undertone.
        t.bg         = sf::Color( 14,  16,  22);   // near-black navy
        t.panel      = sf::Color( 22,  25,  33);   // card base
        t.panel_alt  = sf::Color( 30,  34,  44);   // hover / alternate
        t.card       = sf::Color( 26,  30,  39);   // surface

        t.text       = sf::Color(238, 241, 248);   // off-white
        t.text_dim   = sf::Color(148, 156, 172);   // muted

        t.accent     = sf::Color(104, 172, 254);   // soft electric blue
        t.accent_dim = sf::Color( 52,  96, 168);   // pressed state
        t.border     = sf::Color( 44,  50,  62);   // hairlines

        t.good       = sf::Color(118, 206, 138);   // mint
        t.bad        = sf::Color(238, 112, 118);   // coral red
        t.warn       = sf::Color(244, 192, 108);   // amber
    } else {
        // Warm off-white — never pure white, feels like fine paper.
        t.bg         = sf::Color(244, 245, 248);   // soft mist
        t.panel      = sf::Color(255, 255, 255);   // card
        t.panel_alt  = sf::Color(240, 243, 247);   // hover
        t.card       = sf::Color(255, 255, 255);

        t.text       = sf::Color( 26,  30,  38);   // ink
        t.text_dim   = sf::Color(108, 116, 132);   // muted

        t.accent     = sf::Color( 34, 104, 212);   // deep royal blue
        t.accent_dim = sf::Color(140, 176, 232);   // pressed
        t.border     = sf::Color(222, 226, 234);   // hairlines

        t.good       = sf::Color( 34, 148,  72);   // forest
        t.bad        = sf::Color(198,  60,  62);   // wine
        t.warn       = sf::Color(204, 140,  38);   // bronze
    }

    return t;
}

} // namespace ui