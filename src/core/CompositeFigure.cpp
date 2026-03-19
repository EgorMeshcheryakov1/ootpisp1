#include "CompositeFigure.hpp"
#include "MathUtils.hpp"
#include "utils/GeometryUtils.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace core {

sf::Vector2f CompositeFigure::applyCompositeToWorldPoint(sf::Vector2f point) const {
    sf::Vector2f compositeWorld = parentOrigin + anchor;
    sf::Vector2f local = point - compositeWorld;
    sf::Vector2f rotated = math::rotate(local, rotationAngle * math::DEG_TO_RAD);
    rotated.x *= scale.x;
    rotated.y *= scale.y;
    return compositeWorld + rotated;
}

sf::Vector2f CompositeFigure::applyInverseCompositeToWorldPoint(sf::Vector2f point) const {
    sf::Vector2f compositeWorld = parentOrigin + anchor;
    sf::Vector2f local = point - compositeWorld;
    if (std::abs(scale.x) > 1e-6f) local.x /= scale.x; else local.x = 0.f;
    if (std::abs(scale.y) > 1e-6f) local.y /= scale.y; else local.y = 0.f;
    local = math::rotate(local, -rotationAngle * math::DEG_TO_RAD);
    return compositeWorld + local;
}

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
            if (ptr) {
                sf::Vector2f world = applyCompositeToWorldPoint(ptr->parentOrigin + ptr->anchor);
                ptr->anchor = world - parentOrigin;
                ptr->parentOrigin = parentOrigin;
                ptr->rotationAngle += rotationAngle;
                ptr->scale.x *= scale.x;
                ptr->scale.y *= scale.y;
            }
            return ptr;
        }
    }
    return nullptr;
}

void CompositeFigure::draw(sf::RenderTarget& target) const {
    for (const auto& child : m_children) {
        if (!child) continue;

        sf::Vector2f savedAnchor = child->anchor;
        sf::Vector2f savedParent = child->parentOrigin;
        float savedRot = child->rotationAngle;
        sf::Vector2f savedScale = child->scale;

        sf::Vector2f childWorld = child->parentOrigin + child->anchor;
        sf::Vector2f transformedWorld = applyCompositeToWorldPoint(childWorld);

        child->anchor = transformedWorld;
        child->parentOrigin = sf::Vector2f(0.f, 0.f);
        child->rotationAngle = savedRot + rotationAngle;
        child->scale = sf::Vector2f(savedScale.x * scale.x, savedScale.y * scale.y);

        child->draw(target);

        child->anchor = savedAnchor;
        child->parentOrigin = savedParent;
        child->rotationAngle = savedRot;
        child->scale = savedScale;
    }
}

bool CompositeFigure::contains(sf::Vector2f point) const {
    sf::Vector2f childSpacePoint = applyInverseCompositeToWorldPoint(point);
    for (const auto& child : m_children) {
        if (child && child->contains(childSpacePoint)) return true;
    }
    return false;
}

sf::FloatRect CompositeFigure::getBoundingBox() const {
    rebuildBoundingVerts();
    if (m_boundingVerts.empty()) {
        sf::Vector2f world = parentOrigin + anchor;
        return sf::FloatRect(world.x, world.y, 0.f, 0.f);
    }

    std::vector<sf::Vector2f> absVerts;
    absVerts.reserve(m_boundingVerts.size());
    for (const auto& v : m_boundingVerts) {
        absVerts.push_back(parentOrigin + anchor + v);
    }
    return geometry::computeBoundingBox(absVerts);
}

void CompositeFigure::rebuildBoundingVerts() const {
    if (m_children.empty()) {
        m_boundingVerts = {{0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}};
        return;
    }

    float minX =  std::numeric_limits<float>::max();
    float minY =  std::numeric_limits<float>::max();
    float maxX = -std::numeric_limits<float>::max();
    float maxY = -std::numeric_limits<float>::max();

    for (const auto& child : m_children) {
        if (!child) continue;
        const auto& verts = child->getVertices();
        for (const auto& v : verts) {
            sf::Vector2f childWorld = child->getAbsoluteVertex(v);
            sf::Vector2f transformed = applyCompositeToWorldPoint(childWorld);
            minX = std::min(minX, transformed.x);
            minY = std::min(minY, transformed.y);
            maxX = std::max(maxX, transformed.x);
            maxY = std::max(maxY, transformed.y);
        }
    }

    if (minX == std::numeric_limits<float>::max()) {
        m_boundingVerts = {{0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}};
        return;
    }

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
    for (auto& child : m_children) {
        if (child) {
            child->anchor += delta;
        }
    }
}

} // namespace core
