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

    m_vertices.resize(n + 1);

    // Segment 0: absolute direction angle (degrees)
    float dirDeg = m_segments[0].angle;

    sf::Vector2f cur(0.f, 0.f);
    m_vertices[0] = cur;

    for (size_t i = 0; i < n; ++i) {
        float rad = dirDeg * math::DEG_TO_RAD;
        cur += sf::Vector2f(std::cos(rad), std::sin(rad)) * m_segments[i].length;
        m_vertices[i + 1] = cur;

        if (i + 1 < n) {
            dirDeg += m_segments[i + 1].angle;
        }
    }

    // For closed shapes (polygon): last vertex == first vertex within tolerance.
    // If so, drop the duplicate and treat as closed polygon with n vertices.
    bool closed = false;
    {
        sf::Vector2f diff = m_vertices[n] - m_vertices[0];
        if (std::hypot(diff.x, diff.y) < 1.f) {
            m_vertices.resize(n);
            closed = true;
        }
    }

    // Centre at centroid
    size_t cnt = m_vertices.size();
    float cx = 0.f, cy = 0.f;
    for (auto& v : m_vertices) { cx += v.x; cy += v.y; }
    cx /= (float)cnt; cy /= (float)cnt;
    for (auto& v : m_vertices) { v.x -= cx; v.y -= cy; }

    syncEdges();
}

void PolylineShape::syncEdges() {
    size_t n = m_segments.size();
    if (edges.size() != n) {
        float oldW = edges.empty() ? 2.f : edges[0].width;
        sf::Color oldC = edges.empty() ? sf::Color::Black : edges[0].color;
        edges.resize(n);
        for (auto& e : edges) { e.width = oldW; e.color = oldC; }
    }
}

void PolylineShape::setSideLengths(const std::vector<float>& lengths) {
    for (size_t i = 0; i < m_segments.size() && i < lengths.size(); ++i) {
        m_segments[i].length = std::max(1.f, lengths[i]);
    }
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

// ─── Named constructors ─────────────────────────────────────────────────────

PolylineShape PolylineShape::makeTriangle(float sideA, float sideB, float sideC) {
    sideA = std::max(1.f, sideA);
    sideB = std::max(1.f, sideB);
    sideC = std::max(1.f, sideC);

    // Clamp to valid triangle inequality
    sideA = std::min(sideA, sideB + sideC - 0.1f);
    sideB = std::min(sideB, sideA + sideC - 0.1f);
    sideC = std::min(sideC, sideA + sideB - 0.1f);

    // Place sideA along 0 degrees (base).
    // Compute angle at V0 (between sideC and sideA) using law of cosines:
    // cos(V0) = (sideA^2 + sideC^2 - sideB^2) / (2*sideA*sideC)
    // We go: V0 --(sideA)--> V1 --(sideB)--> V2 --(sideC)--> V0
    // Segment[0]: dir=0, len=sideA
    // At V1 we turn left (CCW) by exterior angle = 180 - angle_at_V1
    // cos(V1) = (sideA^2 + sideB^2 - sideC^2) / (2*sideA*sideB)
    float cosV1 = (sideA*sideA + sideB*sideB - sideC*sideC) / (2.f*sideA*sideB);
    cosV1 = std::max(-1.f, std::min(1.f, cosV1));
    float angleV1 = std::acos(cosV1) * math::RAD_TO_DEG; // interior angle at V1

    // cos(V2) = (sideB^2 + sideC^2 - sideA^2) / (2*sideB*sideC)
    float cosV2 = (sideB*sideB + sideC*sideC - sideA*sideA) / (2.f*sideB*sideC);
    cosV2 = std::max(-1.f, std::min(1.f, cosV2));
    float angleV2 = std::acos(cosV2) * math::RAD_TO_DEG;

    // Exterior turn (CCW = positive in screen coords where Y down means CW is positive).
    // We want a CCW-wound triangle (standard mathematical orientation):
    // turn at V1 = -(180 - angleV1)  (turn left = negative in Y-down coords)
    float turn1 = -(180.f - angleV1);
    float turn2 = -(180.f - angleV2);

    std::vector<Segment> segs = {
        {sideA, 0.f},
        {sideB, turn1},
        {sideC, turn2}
    };

    return PolylineShape(std::move(segs));
}

PolylineShape PolylineShape::makeTrapezoid(float topW, float bottomW, float height) {
    topW    = std::max(1.f, topW);
    bottomW = std::max(1.f, bottomW);
    height  = std::max(1.f, height);

    // Build vertices explicitly then convert to segments.
    // Layout (Y down, screen coords):
    //   BL=(0,0), BR=(bottomW,0), TR=(bottomW - (bottomW-topW)/2, -height), TL=((bottomW-topW)/2, -height)
    float offset = (bottomW - topW) / 2.f;

    sf::Vector2f BL(0.f,       0.f);
    sf::Vector2f BR(bottomW,   0.f);
    sf::Vector2f TR(bottomW - offset, -height);
    sf::Vector2f TL(offset,          -height);

    // Segments: BL->BR, BR->TR, TR->TL, TL->BL
    auto segFromPts = [](sf::Vector2f a, sf::Vector2f b) -> std::pair<float,float> {
        sf::Vector2f d = b - a;
        float len = std::hypot(d.x, d.y);
        float ang = std::atan2(d.y, d.x) * math::RAD_TO_DEG;
        return {len, ang};
    };

    auto [len0, dir0] = segFromPts(BL, BR);
    auto [len1, dir1] = segFromPts(BR, TR);
    auto [len2, dir2] = segFromPts(TR, TL);
    auto [len3, dir3] = segFromPts(TL, BL);

    // Normalise turn angles to (-180, 180]
    auto normAngle = [](float a) -> float {
        while (a >  180.f) a -= 360.f;
        while (a <= -180.f) a += 360.f;
        return a;
    };

    std::vector<Segment> segs = {
        {len0, dir0},                      // seg0: absolute direction
        {len1, normAngle(dir1 - dir0)},    // turn at BR
        {len2, normAngle(dir2 - dir1)},    // turn at TR
        {len3, normAngle(dir3 - dir2)},    // turn at TL
    };

    return PolylineShape(std::move(segs));
}

} // namespace core
