#pragma once

#include "Figure.hpp"
#include <vector>
#include <string>

namespace core {

// A segment descriptor: length of segment and angle (in degrees) between
// this segment and the PREVIOUS segment (exterior turning angle, CCW positive).
// The first segment has angle == initial direction (absolute, degrees from +X axis).
struct Segment {
    float length  = 50.f;
    float angle   = 90.f;   // For segment 0: absolute direction; for i>0: turn from previous
};

// PolylineShape builds its vertices from a chain of Segment descriptors.
// The polygon is CLOSED automatically (last vertex connects back to first).
// Triangle and Trapezoid are now instances of PolylineShape.
class PolylineShape : public Figure {
public:
    // Build with explicit segments.
    explicit PolylineShape(std::vector<Segment> segments);

    // Named constructors for the two refactored shapes:
    static PolylineShape makeTriangle(float sideA, float sideB, float sideC);
    static PolylineShape makeTrapezoid(float topW, float bottomW, float height);

    // Figure interface
    bool hasSideLengths() const override { return true; }
    void setSideLengths(const std::vector<float>& lengths) override;

    // PolylineShape-specific: returns mutable segments for UI editing
    std::vector<Segment>& getSegments() { return m_segments; }
    const std::vector<Segment>& getSegments() const { return m_segments; }

    // Rebuild vertices from m_segments
    void rebuild();

    const char* getSideName(int idx) const override;

private:
    std::vector<Segment> m_segments;
    mutable std::vector<std::string> m_sideNames;

    void syncEdges();
};

} // namespace core
