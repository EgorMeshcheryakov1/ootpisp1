#pragma once

#include "Figure.hpp"
#include <memory>
#include <vector>
#include <string>

namespace core {

// CompositeFigure owns a collection of child Figure objects.
// Children are stored with their positions relative to the composite anchor.
// The composite itself is also a Figure (can be selected, moved, rotated, scaled).
class CompositeFigure : public Figure {
public:
    CompositeFigure() = default;

    // Add a child.  The child's anchor is kept as-is (world-relative offsets
    // are preserved by converting to composite-local space on the first draw).
    void addChild(std::unique_ptr<Figure> child);

    // Remove child by pointer.  Returns the unique_ptr (ownership returned to caller).
    std::unique_ptr<Figure> removeChild(Figure* child);

    // Access children
    const std::vector<std::unique_ptr<Figure>>& getChildren() const { return m_children; }
    std::vector<std::unique_ptr<Figure>>& getChildren() { return m_children; }

    // Figure overrides
    void draw(sf::RenderTarget& target) const override;
    bool contains(sf::Vector2f point) const override;
    sf::FloatRect getBoundingBox() const;

    // We delegate getVertices to a synthetic bounding polygon (4 corners of AABB)
    const std::vector<sf::Vector2f>& getVertices() const override;
    std::vector<sf::Vector2f>& getVerticesMutable() override;

    void move(sf::Vector2f delta);

    // Names for UI display
    static std::string makeDefaultName(size_t index) {
        return "Child " + std::to_string(index + 1);
    }

private:
    std::vector<std::unique_ptr<Figure>> m_children;
    mutable std::vector<sf::Vector2f> m_boundingVerts; // 4 corners, updated on access

    void rebuildBoundingVerts() const;
};

} // namespace core
