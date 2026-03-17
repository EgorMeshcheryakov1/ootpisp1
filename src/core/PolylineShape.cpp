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

    m_vertices.resize(n);

    // Segment 0: absolute direction angle (degrees)
    float dirDeg = m_segments[0].angle;

    // Build vertices starting from (0,0)
    sf::Vector2f cur(0.f, 0.f);
    m_vertices[0] = cur;

    for (size_t i = 0; i < n; ++i) {
        float rad = dirDeg * math::DEG_TO_RAD;
        sf::Vector2f dir(std::cos(rad), std::sin(rad));
        cur += dir * m_segments[i].length;

        size_t nextIdx = (i + 1) % n;
        m_vertices[nextIdx] = cur;

        if (i + 1 < n) {
            // Turn by exterior angle of next segment
            dirDeg += m_segments[i + 1].angle;
        }
    }

    // Centre vertices at centroid
    float cx = 0.f, cy = 0.f;
    for (auto& v : m_vertices) { cx += v.x; cy += v.y; }
    cx /= (float)n;
    cy /= (float)n;
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
    // Build names lazily
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

    // Clamp to valid triangle
    auto clampSide = [](float s, float a, float b) {
        return std::min(s, a + b - 0.1f);
    };
    sideA = clampSide(sideA, sideB, sideC);
    sideB = clampSide(sideB, sideA, sideC);
    sideC = clampSide(sideC, sideA, sideB);

    // Place base (sideA) along 0 degrees, compute angle at vertex 0 using law of cosines
    // cos(A) = (b^2 + c^2 - a^2)/(2bc)  -- angle at V0 opposite to sideA
    float cosA = (sideB * sideB + sideC * sideC - sideA * sideA) / (2.f * sideB * sideC);
    cosA = std::max(-1.f, std::min(1.f, cosA));
    float angleAtV0 = std::acos(cosA) * math::RAD_TO_DEG;

    // Segment 0: sideC, direction 0deg
    // Segment 1: sideA, direction = turn by (180 - angleAtV0) at V1
    // Segment 2: sideB, closes back
    // Interior angle at V1: use law of cosines
    float cosB = (sideA * sideA + sideC * sideC - sideB * sideB) / (2.f * sideA * sideC);
    cosB = std::max(-1.f, std::min(1.f, cosB));
    float angleAtV1 = std::acos(cosB) * math::RAD_TO_DEG;

    // Exterior turn at each vertex = 180 - interior_angle
    // Segment[0].angle = initial absolute direction (0 degrees)
    // Segment[1].angle = exterior turn at V1 = 180 - angleAtV1 (but we go CW for typical triangle)
    // For a CW wound triangle (standard screen coords, Y down):
    float turn1 = -(180.f - angleAtV1); // negative = clockwise turn
    float turn2 = -(180.f - angleAtV0);

    std::vector<Segment> segs = {
        {sideC, 0.f},
        {sideA, turn1},
        {sideB, turn2}
    };

    PolylineShape ps(std::move(segs));
    return ps;
}

PolylineShape PolylineShape::makeTrapezoid(float topW, float bottomW, float height) {
    topW    = std::max(1.f, topW);
    bottomW = std::max(1.f, bottomW);
    height  = std::max(1.f, height);

    // BL -> BR -> TR -> TL
    // dx = (topW - bottomW) / 2.f  (offset of each leg)
    float dx     = (topW - bottomW) / 2.f;
    float legR   = std::sqrt(dx * dx + height * height); // right leg
    float legL   = legR;                                  // symmetric

    // Angles for legs (atan2 gives angle from +X)
    float legAngle = std::atan2(-height, dx) * math::RAD_TO_DEG; // from BR to TR

    // Build segments: bottom, right leg, top (reversed), left leg
    // Start at BL going right (0 deg)
    float turnAfterBottom = legAngle; // at BR: absolute dir to TR
    float turnAfterRightLeg = 180.f;  // at TR: go left along top
    float turnAfterTop = -legAngle + 180.f; // at TL: go down-right

    std::vector<Segment> segs = {
        {bottomW,  0.f},
        {legR,     legAngle},
        {topW,     180.f - legAngle},
        {legL,     -(180.f - legAngle)}
    };

    // Recompute as exterior turns:
    // Segment[0].angle = absolute direction of seg0 = 0 (going right)
    // Segment[i].angle for i>0 = turn at junction V_{i-1} -> V_i
    // i.e. how much to change direction at that vertex
    // We compute cumulative absolute directions:
    float dir0 = 0.f;
    float dir1 = std::atan2(-height, dx) * math::RAD_TO_DEG;  // BL->BR to BR->TR
    float dir2 = 180.f;  // going left along top
    float dir3 = std::atan2(height, -dx) * math::RAD_TO_DEG;  // TL->BL direction

    segs[0].angle = dir0;        // initial absolute dir
    segs[1].angle = dir1 - dir0; // turn at V1
    segs[2].angle = dir2 - dir1; // turn at V2
    segs[3].angle = dir3 - dir2; // turn at V3

    segs[0].length = bottomW;
    segs[1].length = legR;
    segs[2].length = topW;
    segs[3].length = legL;

    PolylineShape ps(std::move(segs));
    return ps;
}

} // namespace core
