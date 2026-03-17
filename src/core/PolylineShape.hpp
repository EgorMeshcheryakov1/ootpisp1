#pragma once

#include "Figure.hpp"
#include <vector>
#include <string>

namespace core {

// A segment descriptor: length of segment and angle (in degrees).
// Segment[0].angle = absolute initial direction (degrees from +X axis).
// Segment[i>0].angle = exterior turning angle at vertex i (CCW positive).
struct Segment {
    float length = 50.f;
    float angle  = 90.f;
};

// PolylineShape builds vertices from a chain of Segment descriptors.
// Closed shapes (Triangle, Trapezoid) loop back to the first vertex.
// Open polylines drawn by the user keep all n+1 points without closing.
class PolylineShape : public Figure {
public:
    explicit PolylineShape(std::vector<Segment> segments);

    static PolylineShape makeTriangle(float sideA, float sideB, float sideC);
    static PolylineShape makeTrapezoid(float topW, float bottomW, float height);

    // Figure interface
    bool hasSideLengths() const override { return true; }
    void setSideLengths(const std::vector<float>& lengths) override;

    std::vector<Segment>& getSegments()       { return m_segments; }
    const std::vector<Segment>& getSegments() const { return m_segments; }

    bool isClosed() const { return m_closed; }

    void rebuild();
    const char* getSideName(int idx) const override;

    // Override draw to handle open polylines (no fill, no closing edge)
    void draw(sf::RenderTarget& target) const override;

private:
    std::vector<Segment> m_segments;
    bool m_closed = true; // true for Triangle/Trapezoid, false for user-drawn polylines
    mutable std::vector<std::string> m_sideNames;

    void syncEdges();
};

} // namespace core
