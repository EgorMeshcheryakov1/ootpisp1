#include "CompositeFigure.hpp"
#include "MathUtils.hpp"
#include "utils/GeometryUtils.hpp"
#include <algorithm>
#include <limits>

namespace core {

void CompositeFigure::addChild(std::unique_ptr<Figure> child) {
    if (child) {
        m_children.push_back(std::move(child));
    }
}

std::unique_ptr<Figure> CompositeFigure::removeChild(Figure* child) {
    for (auto it = m_children.begin(); it != m_children.end(); ++it) {
        if (it->get() == child) {
            auto ptr = std::move(*it);
            m_children.erase(it);
            return ptr;
        }
    }
    return nullptr;
}

void CompositeFigure::draw(sf::RenderTarget& target) const {
    // Apply composite transform to each child.
    // The composite anchor/rotation/scale act as a group transform.
    // Each child draws itself using its own anchor (already in world space when added).
    // We apply the composite's rotation and scale delta on top.
    for (const auto& child : m_children) {
        // Translate child relative to composite anchor
        sf::Vector2f compositeWorld = parentOrigin + anchor;
        sf::Vector2f childWorld = child->parentOrigin + child->anchor;
        sf::Vector2f localOffset = childWorld - compositeWorld;

        // Apply composite rotation to localOffset
        sf::Vector2f rotatedOffset = math::rotate(localOffset, rotationAngle * math::DEG_TO_RAD);
        // Apply composite scale
        rotatedOffset.x *= scale.x;
        rotatedOffset.y *= scale.y;

        // Temporarily override child transform for drawing
        sf::Vector2f savedAnchor = child->anchor;
        sf::Vector2f savedParent = child->parentOrigin;
        float savedRot = child->rotationAngle;
        sf::Vector2f savedScale = child->scale;

        child->anchor = compositeWorld + rotatedOffset;
        child->parentOrigin = sf::Vector2f(0.f, 0.f);
        child->rotationAngle = savedRot + rotationAngle;
        child->scale = sf::Vector2f(savedScale.x * scale.x, savedScale.y * scale.y);

        child->draw(target);

        // Restore
        child->anchor = savedAnchor;
        child->parentOrigin = savedParent;
        child->rotationAngle = savedRot;
        child->scale = savedScale;
    }
}

bool CompositeFigure::contains(sf::Vector2f point) const {
    for (const auto& child : m_children) {
        if (child->contains(point)) return true;
    }
    return false;
}

void CompositeFigure::rebuildBoundingVerts() const {
    if (m_children.empty()) {
        m_boundingVerts = { {0,0},{0,0},{0,0},{0,0} };
        return;
    }

    float minX =  std::numeric_limits<float>::max();
    float minY =  std::numeric_limits<float>::max();
    float maxX = -std::numeric_limits<float>::max();
    float maxY = -std::numeric_limits<float>::max();

    for (const auto& child : m_children) {
        sf::FloatRect b = child->getBoundingBox();
        minX = std::min(minX, b.left);
        minY = std::min(minY, b.top);
        maxX = std::max(maxX, b.left + b.width);
        maxY = std::max(maxY, b.top + b.height);
    }

    // Convert to local (composite-anchor-relative) coords
    sf::Vector2f compositeWorld = parentOrigin + anchor;
    m_boundingVerts = {
        sf::Vector2f(minX - compositeWorld.x, minY - compositeWorld.y),
        sf::Vector2f(maxX - compositeWorld.x, minY - compositeWorld.y),
        sf::Vector2f(maxX - compositeWorld.x, maxY - compositeWorld.y),
        sf::Vector2f(minX - compositeWorld.x, maxY - compositeWorld.y)
    };
}

const std::vector<sf::Vector2f>& CompositeFigure::getVertices() const {
    rebuildBoundingVerts();
    return m_boundingVerts;
}

std::vector<sf::Vector2f>& CompositeFigure::getVerticesMutable() {
    rebuildBoundingVerts();
    return m_boundingVerts;
}

void CompositeFigure::move(sf::Vector2f delta) {
    anchor += delta;
    // Move all children too
    for (auto& child : m_children) {
        child->anchor += delta;
    }
}

} // namespace core
