#include "PolylineShape.hpp"
#include "MathUtils.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace core {

namespace {
float normalizeTurn(float angle) {
    while (angle > 180.f) angle -= 360.f;
    while (angle <= -180.f) angle += 360.f;
    return angle;
}

float distancePointToSegment(sf::Vector2f p, sf::Vector2f a, sf::Vector2f b) {
    sf::Vector2f ab = b - a;
    float abLenSq = ab.x * ab.x + ab.y * ab.y;
    if (abLenSq <= 1e-6f) {
        sf::Vector2f d = p - a;
        return std::hypot(d.x, d.y);
    }
    float t = ((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / abLenSq;
    t = std::clamp(t, 0.f, 1.f);
    sf::Vector2f closest = a + ab * t;
    sf::Vector2f d = p - closest;
    return std::hypot(d.x, d.y);
}
}

PolylineShape::PolylineShape(std::vector<Segment> segments, bool closed)
    : m_segments(std::move(segments)), m_closed(closed) {
    rebuild();
}

void PolylineShape::setClosed(bool closed) {
    m_closed = closed;
    rebuild();
}

void PolylineShape::setSegmentAngle(size_t idx, float angleDegrees) {
    if (idx >= m_segments.size()) return;
    if (idx == 0)
        m_segments[idx].angle = angleDegrees;
    else
        m_segments[idx].angle = normalizeTurn(angleDegrees);
    rebuild();
}

void PolylineShape::rebuild() {
    const size_t n = m_segments.size();
    if (n == 0) {
        m_vertices.clear();
        syncEdges();
        return;
    }

    std::vector<sf::Vector2f> pts;
    pts.reserve(n + 1);
    pts.push_back(sf::Vector2f(0.f, 0.f));

    float dirDeg = m_segments[0].angle;
    for (size_t i = 0; i < n; ++i) {
        float rad = dirDeg * math::DEG_TO_RAD;
        float segLen = std::max(1.f, m_segments[i].length);
        pts.push_back(pts.back() + sf::Vector2f(std::cos(rad), std::sin(rad)) * segLen);
        if (i + 1 < n)
            dirDeg += m_segments[i + 1].angle;
    }

    bool geometricallyClosed = false;
    if (pts.size() >= 2) {
        sf::Vector2f diff = pts.back() - pts.front();
        geometricallyClosed = (std::hypot(diff.x, diff.y) < 2.f);
    }

    if (m_closed || geometricallyClosed) {
        m_closed = true;
        m_vertices.assign(pts.begin(), pts.end() - 1);
    } else {
        m_vertices = pts;
    }

    if (m_vertices.empty()) {
        syncEdges();
        return;
    }

    float cx = 0.f, cy = 0.f;
    for (const auto& v : m_vertices) { cx += v.x; cy += v.y; }
    cx /= static_cast<float>(m_vertices.size());
    cy /= static_cast<float>(m_vertices.size());
    for (auto& v : m_vertices) {
        v.x -= cx;
        v.y -= cy;
    }

    syncEdges();
}

void PolylineShape::syncEdges() {
    const size_t n = m_segments.size();
    float oldW = edges.empty() ? 2.f : edges.front().width;
    sf::Color oldC = edges.empty() ? sf::Color::Black : edges.front().color;
    edges.resize(n);
    for (auto& e : edges) {
        e.width = oldW;
        e.color = oldC;
    }
}

std::vector<float> PolylineShape::getSideLengths() const {
    std::vector<float> lengths;
    lengths.reserve(m_segments.size());
    for (const auto& segment : m_segments)
        lengths.push_back(segment.length);
    return lengths;
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
            oss << (m_closed ? "Edge " : "Segment ") << (i + 1);
            m_sideNames[i] = oss.str();
        }
    }
    if (idx < 0 || static_cast<size_t>(idx) >= m_sideNames.size()) return "Side";
    return m_sideNames[static_cast<size_t>(idx)].c_str();
}

bool PolylineShape::contains(sf::Vector2f point) const {
    if (m_closed)
        return Figure::contains(point);

    const auto& verts = getVertices();
    if (verts.size() < 2)
        return false;

    std::vector<sf::Vector2f> absVerts;
    absVerts.reserve(verts.size());
    for (const auto& v : verts)
        absVerts.push_back(getAbsoluteVertex(v));

    for (size_t i = 0; i + 1 < absVerts.size(); ++i) {
        float edgeWidth = (i < edges.size()) ? edges[i].width : 2.f;
        if (distancePointToSegment(point, absVerts[i], absVerts[i + 1]) <= std::max(6.f, edgeWidth * 1.5f))
            return true;
    }
    return false;
}

void PolylineShape::draw(sf::RenderTarget& target) const {
    const auto& verts = getVertices();
    if (verts.empty()) return;

    const size_t n = m_segments.size();

    if (m_closed) {
        Figure::draw(target);
        return;
    }

    if (edges.empty() || verts.size() < 2) return;

    std::vector<sf::Vector2f> V(verts.size());
    for (size_t i = 0; i < verts.size(); ++i)
        V[i] = getAbsoluteVertex(verts[i]);

    for (size_t i = 0; i < n && i + 1 < V.size(); ++i) {
        size_t eIdx = i < edges.size() ? i : 0;

        float drawWidth = edges[eIdx].width;
        sf::Color drawColor = edges[eIdx].color;

        if (edges[eIdx].flashEnabled && edges[eIdx].flashDuration > 0.f) {
            float t = edges[eIdx].flashTime / edges[eIdx].flashDuration;
            t = std::clamp(t, 0.f, 1.f);
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

        sf::Vector2f dir = delta / len;
        sf::Vector2f normal(-dir.y, dir.x);
        float hw = drawWidth / 2.f;

        sf::ConvexShape quad(4);
        quad.setPoint(0, V[i] - normal * hw);
        quad.setPoint(1, V[i] + normal * hw);
        quad.setPoint(2, V[i + 1] + normal * hw);
        quad.setPoint(3, V[i + 1] - normal * hw);
        quad.setFillColor(drawColor);
        target.draw(quad);
    }
}

PolylineShape PolylineShape::makeTriangle(float sideA, float sideB, float sideC) {
    sideA = std::max(1.f, sideA);
    sideB = std::max(1.f, sideB);
    sideC = std::max(1.f, sideC);

    sideA = std::min(sideA, sideB + sideC - 0.1f);
    sideB = std::min(sideB, sideA + sideC - 0.1f);
    sideC = std::min(sideC, sideA + sideB - 0.1f);

    float cosV1 = (sideA*sideA + sideB*sideB - sideC*sideC) / (2.f*sideA*sideB);
    cosV1 = std::clamp(cosV1, -1.f, 1.f);
    float angleV1 = std::acos(cosV1) * math::RAD_TO_DEG;

    float cosV2 = (sideB*sideB + sideC*sideC - sideA*sideA) / (2.f*sideB*sideC);
    cosV2 = std::clamp(cosV2, -1.f, 1.f);
    float angleV2 = std::acos(cosV2) * math::RAD_TO_DEG;

    float turn1 = -(180.f - angleV1);
    float turn2 = -(180.f - angleV2);

    std::vector<Segment> segs = {
        {sideA,  0.f},
        {sideB, turn1},
        {sideC, turn2}
    };
    return PolylineShape(std::move(segs), true);
}

PolylineShape PolylineShape::makeTrapezoid(float topW, float bottomW, float height) {
    topW = std::max(1.f, topW);
    bottomW = std::max(1.f, bottomW);
    height = std::max(1.f, height);

    float offset = (bottomW - topW) / 2.f;
    sf::Vector2f BL(0.f, 0.f);
    sf::Vector2f BR(bottomW, 0.f);
    sf::Vector2f TR(bottomW - offset, -height);
    sf::Vector2f TL(offset, -height);

    auto seg = [](sf::Vector2f a, sf::Vector2f b) -> std::pair<float,float> {
        sf::Vector2f d = b - a;
        return {std::hypot(d.x, d.y), std::atan2(d.y, d.x) * math::RAD_TO_DEG};
    };

    auto [l0, d0] = seg(BL, BR);
    auto [l1, d1] = seg(BR, TR);
    auto [l2, d2] = seg(TR, TL);
    auto [l3, d3] = seg(TL, BL);

    std::vector<Segment> segs = {
        {l0, d0},
        {l1, normalizeTurn(d1 - d0)},
        {l2, normalizeTurn(d2 - d1)},
        {l3, normalizeTurn(d3 - d2)},
    };
    return PolylineShape(std::move(segs), true);
}

} // namespace core
