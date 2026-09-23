#include "ui/pieces.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

namespace ui {
using namespace chess;

namespace {

// ---------------------------------------------------------------
// Path builder: a polyline with Bezier smoothing helpers.
// ---------------------------------------------------------------
struct Path {
    std::vector<sf::Vector2f> pts;
    bool closed = false;

    Path& start(sf::Vector2f p) { pts.clear(); pts.push_back(p); closed = false; return *this; }
    Path& to(sf::Vector2f p)    { pts.push_back(p); return *this; }

    // Quadratic Bezier from current point through control `c` to end `e`.
    Path& q(sf::Vector2f c, sf::Vector2f e, int n = 14) {
        sf::Vector2f a = pts.back();
        for (int i = 1; i <= n; ++i) {
            const float t = static_cast<float>(i) / n;
            const float u = 1.f - t;
            pts.push_back({
                u*u*a.x + 2.f*u*t*c.x + t*t*e.x,
                u*u*a.y + 2.f*u*t*c.y + t*t*e.y
            });
        }
        return *this;
    }

    Path& close() { closed = true; return *this; }
};

// ---------------------------------------------------------------
// Fill a closed (star-shaped) path using fan triangulation.
// The `center` must be strictly inside the shape.
// ---------------------------------------------------------------
void fill_path(sf::RenderTarget& tgt, const Path& p, sf::Vector2f center, sf::Color color) {
    if (p.pts.size() < 3) return;

    sf::VertexArray va(sf::PrimitiveType::Triangles);
    for (std::size_t i = 0; i + 1 < p.pts.size(); ++i) {
        va.append(sf::Vertex(center, color));
        va.append(sf::Vertex(p.pts[i], color));
        va.append(sf::Vertex(p.pts[i + 1], color));
    }
    if (p.closed) {
        va.append(sf::Vertex(center, color));
        va.append(sf::Vertex(p.pts.back(), color));
        va.append(sf::Vertex(p.pts.front(), color));
    }
    tgt.draw(va);
}

// ---------------------------------------------------------------
// Draw a thick, anti-aliased-ish stroke along the path.
// - Each segment becomes a quad.
// - Sharp corners get a small round joint.
// ---------------------------------------------------------------
void stroke_path(sf::RenderTarget& tgt, const Path& p, sf::Color color, float thickness) {
    if (p.pts.size() < 2) return;

    const float r = thickness * 0.5f;
    const std::size_t n = p.pts.size();
    const std::size_t segs = p.closed ? n : n - 1;

    // --- Segment quads (single batch) ---
    {
        sf::VertexArray va(sf::PrimitiveType::Triangles);
        for (std::size_t i = 0; i < segs; ++i) {
            const sf::Vector2f a = p.pts[i];
            const sf::Vector2f b = p.pts[(i + 1) % n];
            const sf::Vector2f d = b - a;
            const float len = std::sqrt(d.x*d.x + d.y*d.y);
            if (len < 0.0001f) continue;
            const sf::Vector2f nrm{-d.y / len * r, d.x / len * r};

            va.append(sf::Vertex(a + nrm, color));
            va.append(sf::Vertex(a - nrm, color));
            va.append(sf::Vertex(b + nrm, color));
            va.append(sf::Vertex(b + nrm, color));
            va.append(sf::Vertex(a - nrm, color));
            va.append(sf::Vertex(b - nrm, color));
        }
        tgt.draw(va);
    }

    // --- Round joints only at sharp corners (single batch) ---
    {
        sf::VertexArray va(sf::PrimitiveType::Triangles);
        constexpr int ngon = 8;
        for (std::size_t i = 0; i < n; ++i) {
            const sf::Vector2f prev = p.pts[(i + n - 1) % n];
            const sf::Vector2f curr = p.pts[i];
            const sf::Vector2f next = p.pts[(i + 1) % n];

            const sf::Vector2f d1 = curr - prev;
            const sf::Vector2f d2 = next - curr;
            const float l1 = std::sqrt(d1.x*d1.x + d1.y*d1.y);
            const float l2 = std::sqrt(d2.x*d2.x + d2.y*d2.y);
            if (l1 < 0.001f || l2 < 0.001f) continue;

            // Skip nearly-collinear joints (avoids wasting triangles on smooth curves).
            const float dot = (d1.x*d2.x + d1.y*d2.y) / (l1 * l2);
            if (dot > 0.9995f) continue;

            for (int j = 0; j < ngon; ++j) {
                const float a1 = (static_cast<float>(j) / ngon) * 6.2831853f;
                const float a2 = (static_cast<float>(j + 1) / ngon) * 6.2831853f;
                const sf::Vector2f v1{curr.x + std::cos(a1) * r, curr.y + std::sin(a1) * r};
                const sf::Vector2f v2{curr.x + std::cos(a2) * r, curr.y + std::sin(a2) * r};
                va.append(sf::Vertex(curr, color));
                va.append(sf::Vertex(v1, color));
                va.append(sf::Vertex(v2, color));
            }
        }
        tgt.draw(va);
    }
}

// ---------------------------------------------------------------
// Piece palette
// ---------------------------------------------------------------
struct PieceColors {
    sf::Color fill;
    sf::Color outline;
    sf::Color highlight;
};

PieceColors colors_for(Color c) {
    PieceColors col;
    if (c == Color::White) {
        col.fill      = sf::Color(250, 248, 240);   // warm ivory
        col.outline   = sf::Color( 40,  36,  32);   // deep brown-black
        col.highlight = sf::Color(255, 255, 255, 220);
    } else {
        col.fill      = sf::Color( 44,  42,  40);   // warm off-black
        col.outline   = sf::Color(200, 195, 185);   // pale highlight edge
        col.highlight = sf::Color(150, 145, 135, 150);
    }
    return col;
}

// ---------------------------------------------------------------
// Piece silhouettes (design space: 100 x 100, x-center = 50, base = y 94)
// ---------------------------------------------------------------

Path path_pawn() {
    Path p;
    p.start({20, 94}).to({80, 94})
     // base right
     .q({82, 90}, {78, 86})
     .q({74, 82}, {72, 78})
     // collar bulge
     .q({76, 74}, {74, 70})
     .q({72, 66}, {66, 62})
     // neck rising
     .q({60, 58}, {60, 52})
     .q({60, 48}, {62, 45})
     // head (round)
     .q({68, 40}, {68, 34})
     .q({68, 26}, {58, 22})
     .q({50, 20}, {42, 22})
     .q({32, 26}, {32, 34})
     .q({32, 40}, {38, 45})
     .q({40, 48}, {40, 52})
     // neck left
     .q({40, 58}, {34, 62})
     .q({28, 66}, {26, 70})
     // collar left
     .q({24, 74}, {28, 78})
     // base left
     .q({26, 82}, {22, 86})
     .q({18, 90}, {20, 94})
     .close();
    return p;
}

Path path_rook() {
    Path p;
    p.start({18, 94}).to({82, 94})
     // base
     .q({84, 90}, {80, 88})
     .q({76, 86}, {74, 84})
     // body taper
     .q({70, 78}, {70, 70})
     .q({70, 60}, {72, 52})
     // collar
     .q({76, 48}, {78, 46})
     .q({80, 44}, {78, 40})
     // turret
     .q({74, 38}, {72, 34})
     .to({72, 22})
     // crenellations (3 merlons, 2 crenels)
     .to({66, 22}).to({66, 12}).to({58, 12}).to({58, 22})
     .to({42, 22}).to({42, 12}).to({34, 12}).to({34, 22})
     .to({28, 22}).to({28, 34})
     // left side down
     .q({26, 38}, {22, 40})
     .q({20, 44}, {22, 46})
     .q({24, 48}, {28, 52})
     .q({30, 60}, {30, 70})
     .q({30, 78}, {26, 84})
     .q({24, 86}, {20, 88})
     .q({16, 90}, {18, 94})
     .close();
    return p;
}

Path path_bishop() {
    Path p;
    p.start({20, 94}).to({80, 94})
     // base
     .q({82, 90}, {78, 86})
     .q({72, 82}, {70, 78})
     // collar
     .q({74, 74}, {72, 70})
     // stem
     .q({68, 64}, {62, 58})
     .q({56, 52}, {52, 48})
     // mitre (widens)
     .q({56, 42}, {58, 36})
     .q({58, 30}, {54, 24})
     // mitre top
     .q({50, 20}, {46, 24})
     .q({42, 30}, {42, 36})
     .q({44, 42}, {48, 48})
     // stem left
     .q({44, 52}, {38, 58})
     .q({32, 64}, {28, 70})
     // collar left
     .q({26, 74}, {30, 78})
     .q({28, 82}, {22, 86})
     .q({18, 90}, {20, 94})
     .close();
    return p;
}

Path path_knight() {
    // Knight faces RIGHT: muzzle protrudes to x≈80, mane flows down the left.
    Path p;
    p.start({18, 94}).to({82, 94})
     // base right
     .q({82, 90}, {78, 88})
     .q({74, 84}, {74, 80})
     // neck right side (rising)
     .q({74, 68}, {76, 56})
     .q({76, 48}, {76, 42})
     // jaw and muzzle
     .q({80, 38}, {80, 34})
     .q({80, 30}, {76, 26})
     // top of muzzle
     .q({70, 22}, {62, 20})
     .q({54, 18}, {48, 18})
     // ear
     .q({44, 18}, {42, 14})
     .q({40, 8},  {36, 10})
     .q({32, 14}, {28, 18})
     // mane
     .q({24, 22}, {22, 30})
     .q({20, 42}, {24, 52})
     .q({28, 60}, {28, 66})
     // lower neck / base
     .q({30, 72}, {28, 80})
     .q({24, 84}, {20, 88})
     .q({16, 90}, {18, 94})
     .close();
    return p;
}

Path path_queen() {
    Path p;
    p.start({16, 94}).to({84, 94})
     // base
     .q({86, 90}, {82, 87})
     .q({76, 84}, {74, 80})
     // body taper
     .q({70, 72}, {66, 64})
     .q({60, 54}, {58, 46})
     // crown right shoulder
     .q({62, 42}, {66, 38})
     // 5-point crown: zigzag across the top
     .to({70, 20})   // rightmost tip
     .to({65, 30})   // valley
     .to({60, 18})   // peak
     .to({55, 30})   // valley
     .to({50, 14})   // middle peak (highest)
     .to({45, 30})   // valley
     .to({40, 18})   // peak
     .to({35, 30})   // valley
     .to({30, 20})   // leftmost tip
     // left shoulder
     .q({28, 30}, {30, 38})
     // body left
     .q({34, 42}, {38, 46})
     .q({40, 54}, {34, 64})
     .q({30, 72}, {26, 80})
     // base left
     .q({24, 84}, {20, 87})
     .q({14, 90}, {16, 94})
     .close();
    return p;
}

Path path_king() {
    Path p;
    p.start({16, 94}).to({84, 94})
     // base
     .q({86, 90}, {82, 87})
     .q({76, 84}, {74, 80})
     // body
     .q({70, 70}, {64, 60})
     .q({58, 50}, {56, 44})
     // crown (bulbous)
     .q({62, 40}, {66, 34})
     .q({70, 28}, {64, 22})
     .q({56, 18}, {48, 18})
     .q({40, 18}, {32, 22})
     .q({28, 28}, {30, 34})
     .q({34, 40}, {40, 44})
     // body left
     .q({36, 50}, {32, 60})
     .q({28, 70}, {26, 80})
     .q({24, 84}, {20, 87})
     .q({14, 90}, {16, 94})
     .close();
    return p;
}

} // namespace

// ===============================================================
// Public entry point
// ===============================================================
void draw_piece(sf::RenderTarget& tgt,
                Piece piece,
                float x, float y, float size,
                bool /*outline_white*/)
{
    if (piece.empty()) return;

    const float s  = size / 100.f;
    const float ox = x;
    const float oy = y;

    // Design-space -> screen-space transform.
    auto T = [&](sf::Vector2f p) { return sf::Vector2f(ox + p.x * s, oy + p.y * s); };

    const PieceColors col = colors_for(piece.color);

    // ---- Pick the silhouette ----
    Path shape;
    switch (piece.type) {
        case PieceType::Pawn:   shape = path_pawn();   break;
        case PieceType::Knight: shape = path_knight(); break;
        case PieceType::Bishop: shape = path_bishop(); break;
        case PieceType::Rook:   shape = path_rook();   break;
        case PieceType::Queen:  shape = path_queen();  break;
        case PieceType::King:   shape = path_king();   break;
        default: return;
    }

    // Scale every vertex.
    for (auto& p : shape.pts) p = T(p);

    // Fan center (interior point of every silhouette).
    const sf::Vector2f center = T({50.f, 60.f});

    // Outline thickness scales with piece but never collapses below 1px.
    const float ot = std::max(1.0f, 1.5f * s);

    // ---------------------------------------------------------
    // 1) Soft drop shadow
    // ---------------------------------------------------------
    {
        Path shadow = shape;
        for (auto& p : shadow.pts) p += sf::Vector2f(0.f, 2.2f * s);
        fill_path(tgt, shadow, center + sf::Vector2f(0.f, 2.2f * s),
                  sf::Color(0, 0, 0, 45));
    }

    // ---------------------------------------------------------
    // 2) Main fill
    // ---------------------------------------------------------
    fill_path(tgt, shape, center, col.fill);

    // ---------------------------------------------------------
    // 3) Ink outline
    // ---------------------------------------------------------
    stroke_path(tgt, shape, col.outline, ot);

    // ---------------------------------------------------------
    // 4) Piece-specific details
    // ---------------------------------------------------------
    if (piece.type == PieceType::King) {
        // Latin cross above the crown
        const float cw = 5.f * s;
        const float ch = 22.f * s;

        sf::RectangleShape vbar({cw, ch});
        vbar.setOrigin(cw * 0.5f, ch);
        vbar.setPosition(T({50.f, 20.f}));
        vbar.setFillColor(col.fill);
        vbar.setOutlineColor(col.outline);
        vbar.setOutlineThickness(ot);
        tgt.draw(vbar);

        sf::RectangleShape hbar({ch * 0.6f, cw});
        hbar.setOrigin(ch * 0.6f * 0.5f, cw * 0.5f);
        hbar.setPosition(T({50.f, 8.f}));
        hbar.setFillColor(col.fill);
        hbar.setOutlineColor(col.outline);
        hbar.setOutlineThickness(ot);
        tgt.draw(hbar);
    }

    if (piece.type == PieceType::Bishop) {
        // Small finial ball on top of the mitre
        const float br = 2.8f * s;
        sf::CircleShape ball(br, 22);
        ball.setOrigin(br, br);
        ball.setPosition(T({50.f, 18.f}));
        ball.setFillColor(col.fill);
        ball.setOutlineColor(col.outline);
        ball.setOutlineThickness(ot);
        tgt.draw(ball);
    }

    if (piece.type == PieceType::Queen) {
        // Orb on the middle crown peak
        const float br = 3.0f * s;
        sf::CircleShape ball(br, 22);
        ball.setOrigin(br, br);
        ball.setPosition(T({50.f, 12.f}));
        ball.setFillColor(col.fill);
        ball.setOutlineColor(col.outline);
        ball.setOutlineThickness(ot);
        tgt.draw(ball);
    }

    if (piece.type == PieceType::Knight) {
        // Single dark eye for the horse head.
        const float er = 1.5f * s;
        sf::CircleShape eye(er, 14);
        eye.setOrigin(er, er);
        eye.setPosition(T({62.f, 26.f}));
        eye.setFillColor(col.outline);
        tgt.draw(eye);
    }

    // ---------------------------------------------------------
    // 5) Soft highlight on the upper-left of the piece
    // ---------------------------------------------------------
    {
        float hr = 4.f * s;
        float hx = 42.f, hy = 40.f;

        switch (piece.type) {
            case PieceType::Pawn:   hx = 43.f; hy = 38.f; hr = 4.0f * s; break;
            case PieceType::Knight: hx = 44.f; hy = 22.f; hr = 2.5f * s; break;
            case PieceType::Bishop: hx = 46.f; hy = 34.f; hr = 2.5f * s; break;
            case PieceType::Rook:   hx = 32.f; hy = 30.f; hr = 2.0f * s; break;
            case PieceType::Queen:  hx = 44.f; hy = 32.f; hr = 3.0f * s; break;
            case PieceType::King:   hx = 44.f; hy = 32.f; hr = 3.5f * s; break;
            default: break;
        }

        sf::CircleShape hl(hr, 22);
        hl.setOrigin(hr, hr);
        hl.setPosition(T({hx, hy}));
        hl.setFillColor(col.highlight);
        tgt.draw(hl);
    }
}

} // namespace ui