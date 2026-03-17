#include "PolylineShape.hpp"
#include "MathUtils.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace core {

PolylineShape::PolylineShape(std::vector<Segment> segments)
    : m_segments(std::move(segments))
{
    rebuild();
}

void PolylineShape::rebuild() {
    const size_t n = m_segments.size();
    if (n == 0) {
        m_vertices.clear();
        syncEdges();
        return;
    }

    // Build n+1 points (start + one per segment)
    std::vector<sf::Vector2f> pts(n + 1);
    float dirDeg = m_segments[0].angle;
    pts[0] = sf::Vector2f(0.f, 0.f);

    for (size_t i = 0; i < n; ++i) {
        float rad = dirDeg * math::DEG_TO_RAD;
        pts[i + 1] = pts[i] + sf::Vector2f(std::cos(rad), std::sin(rad)) * m_segments[i].length;
        if (i + 1 < n)
            dirDeg += m_segments[i + 1].angle;
    }

    // Detect closed shape: last point coincides with first (within 2px)
    sf::Vector2f diff = pts[n] - pts[0];
    m_closed = (std::hypot(diff.x, diff.y) < 2.f);

    if (m_closed) {
        // Closed polygon: store n vertices (drop duplicate last point)
        m_vertices.assign(pts.begin(), pts.begin() + n);
    } else {
        // Open polyline: store all n+1 vertices
        m_vertices = pts;
    }

    // Centre at centroid
    float cx = 0.f, cy = 0.f;
    for (auto& v : m_vertices) { cx += v.x; cy += v.y; }
    cx /= (float)m_vertices.size();
    cy /= (float)m_vertices.size();
    for (auto& v : m_vertices) { v.x -= cx; v.y -= cy; }

    syncEdges();
}

void PolylineShape::syncEdges() {
    // edges count = number of drawn segments = n for closed, n for open (n segments between n+1 pts)
    size_t n = m_segments.size();
    if (edges.size() != n) {
        float oldW   = edges.empty() ? 2.f          : edges[0].width;
        sf::Color oldC = edges.empty() ? sf::Color::Black : edges[0].color;
        edges.resize(n);
        for (auto& e : edges) { e.width = oldW; e.color = oldC; }
    }
}

void PolylineShape::setSideLengths(const std::vector<float>& lengths) {
    for (size_t i = 0; i < m_segments.size() && i < lengths.size(); ++i)
        m_segments[i].length = std::max(1.f, lengths[i]);
    rebuild();
}

const char* PolylineShape::getSideName(int idx) const {
    if (m_sideNames.size() != m_segments.size()) {
        m_sideNames.resize(m_segments.size());
        for (size_t i = 0; i < m_segments.size(); ++i) {
            std::ostringstream oss;
            oss << "Side " << (i + 1);
            m_sideNames[i] = oss.str();
        }
    }
    if (idx < 0 || (size_t)idx >= m_sideNames.size()) return "Side";
    return m_sideNames[(size_t)idx].c_str();
}

// Override draw: open polylines have no fill and do not close the last edge
void PolylineShape::draw(sf::RenderTarget& target) const {
    const auto& verts = getVertices();
    if (verts.empty()) return;

    const size_t n = m_segments.size(); // number of segments = number of edges to draw

    if (m_closed) {
        // Closed shape: use base class draw (fills + n edges closing back to v[0])
        Figure::draw(target);
        return;
    }

    // Open polyline: draw fill as convex hull approximation only if alpha > 0
    if (fillColor.a > 0) {
        sf::ConvexShape fill(verts.size());
        for (size_t i = 0; i < verts.size(); ++i)
            fill.setPoint(i, verts[i]);
        fill.setPosition(parentOrigin + anchor);
        fill.setRotation(rotationAngle);
        fill.setScale(scale);
        fill.setFillColor(fillColor);
        target.draw(fill);
    }

    if (edges.empty()) return;

    // Compute absolute vertices
    std::vector<sf::Vector2f> V(verts.size());
    for (size_t i = 0; i < verts.size(); ++i)
        V[i] = getAbsoluteVertex(verts[i]);

    // Draw exactly n segments: V[0]->V[1], V[1]->V[2], ..., V[n-1]->V[n]
    // (For open polyline verts.size() == n+1)
    for (size_t i = 0; i < n; ++i) {
        size_t eIdx = i < edges.size() ? i : 0;

        float drawWidth = edges[eIdx].width;
        sf::Color drawColor = edges[eIdx].color;

        if (edges[eIdx].flashEnabled && edges[eIdx].flashDuration > 0.f) {
            float t = edges[eIdx].flashTime / edges[eIdx].flashDuration;
            if (t < 0.f) t = 0.f; if (t > 1.f) t = 1.f;
            float pulse = 0.5f + 0.5f * std::sin((1.f - t) * 18.f);
            drawWidth += 2.0f + pulse * 3.0f;
            auto mix = [&](sf::Uint8 a, sf::Uint8 b) -> sf::Uint8 {
                float k = 0.55f + 0.45f * pulse;
                return static_cast<sf::Uint8>(a + (b - a) * k);
            };
            drawColor.r = mix(drawColor.r, 255);
            drawColor.g = mix(drawColor.g, 255);
            drawColor.b = mix(drawColor.b, 0);
        }

        if (drawWidth <= 0.001f) continue;

        sf::Vector2f delta = V[i + 1] - V[i];
        float len = std::hypot(delta.x, delta.y);
        if (len <= 0.0001f) continue;

        sf::Vector2f dir    = delta / len;
        sf::Vector2f normal = sf::Vector2f(-dir.y, dir.x);
        float hw = drawWidth / 2.f;

        sf::ConvexShape quad(4);
        quad.setPoint(0, V[i]     - normal * hw);
        quad.setPoint(1, V[i]     + normal * hw);
        quad.setPoint(2, V[i + 1] + normal * hw);
        quad.setPoint(3, V[i + 1] - normal * hw);
        quad.setFillColor(drawColor);
        target.draw(quad);
    }
}

// ─── Named constructors ──────────────────────────────────────────────────────

PolylineShape PolylineShape::makeTriangle(float sideA, float sideB, float sideC) {
    sideA = std::max(1.f, sideA);
    sideB = std::max(1.f, sideB);
    sideC = std::max(1.f, sideC);

    sideA = std::min(sideA, sideB + sideC - 0.1f);
    sideB = std::min(sideB, sideA + sideC - 0.1f);
    sideC = std::min(sideC, sideA + sideB - 0.1f);

    // V0 --(sideA)--> V1 --(sideB)--> V2 --(sideC)--> V0
    // Interior angles by law of cosines
    float cosV1 = (sideA*sideA + sideB*sideB - sideC*sideC) / (2.f*sideA*sideB);
    cosV1 = std::max(-1.f, std::min(1.f, cosV1));
    float angleV1 = std::acos(cosV1) * math::RAD_TO_DEG;

    float cosV2 = (sideB*sideB + sideC*sideC - sideA*sideA) / (2.f*sideB*sideC);
    cosV2 = std::max(-1.f, std::min(1.f, cosV2));
    float angleV2 = std::acos(cosV2) * math::RAD_TO_DEG;

    // CW winding (Y-down screen): turn right at each vertex
    float turn1 = -(180.f - angleV1);
    float turn2 = -(180.f - angleV2);

    std::vector<Segment> segs = {
        {sideA,  0.f},
        {sideB, turn1},
        {sideC, turn2}
    };
    return PolylineShape(std::move(segs));
}

PolylineShape PolylineShape::makeTrapezoid(float topW, float bottomW, float height) {
    topW    = std::max(1.f, topW);
    bottomW = std::max(1.f, bottomW);
    height  = std::max(1.f, height);

    // Explicit vertices (Y-down): BL -> BR -> TR -> TL -> BL (closed)
    float offset = (bottomW - topW) / 2.f;
    sf::Vector2f BL(0.f,              0.f);
    sf::Vector2f BR(bottomW,          0.f);
    sf::Vector2f TR(bottomW - offset, -height);
    sf::Vector2f TL(offset,           -height);

    auto seg = [](sf::Vector2f a, sf::Vector2f b) -> std::pair<float,float> {
        sf::Vector2f d = b - a;
        return { std::hypot(d.x, d.y), std::atan2(d.y, d.x) * math::RAD_TO_DEG };
    };
    auto norm = [](float a) -> float {
        while (a >  180.f) a -= 360.f;
        while (a <= -180.f) a += 360.f;
        return a;
    };

    auto [l0, d0] = seg(BL, BR);
    auto [l1, d1] = seg(BR, TR);
    auto [l2, d2] = seg(TR, TL);
    auto [l3, d3] = seg(TL, BL);

    std::vector<Segment> segs = {
        {l0, d0},            // seg0: absolute dir of bottom
        {l1, norm(d1 - d0)}, // turn at BR
        {l2, norm(d2 - d1)}, // turn at TR
        {l3, norm(d3 - d2)}, // turn at TL
    };
    return PolylineShape(std::move(segs));
}

} // namespace core
