#pragma once

#include "Figure.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace core {

struct Segment {
    float length = 50.f;
    float angle  = 90.f;
};

class PolylineShape : public Figure {
public:
    explicit PolylineShape(std::vector<Segment> segments, bool closed = true);

    static PolylineShape makeTriangle(float sideA, float sideB, float sideC);
    static PolylineShape makeTrapezoid(float topW, float bottomW, float height);

    bool hasSideLengths() const override { return true; }
    std::vector<float> getSideLengths() const override;
    void setSideLengths(const std::vector<float>& lengths) override;

    std::vector<Segment>& getSegments() { return m_segments; }
    const std::vector<Segment>& getSegments() const { return m_segments; }

    bool isClosed() const { return m_closed; }
    void setClosed(bool closed);
    void setSegmentAngle(std::size_t idx, float angleDegrees);

    void rebuild();
    const char* getSideName(int idx) const override;
    void draw(sf::RenderTarget& target) const override;
    bool contains(sf::Vector2f point) const override;

private:
    std::vector<Segment> m_segments;
    bool m_closed = true;
    mutable std::vector<std::string> m_sideNames;

    void syncEdges();
};

} // namespace core
