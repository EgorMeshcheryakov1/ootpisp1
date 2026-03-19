#pragma once

#include "Figure.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace core {

class CompositeFigure : public Figure {
public:
    CompositeFigure() = default;

    void addChild(std::unique_ptr<Figure> child);
    std::unique_ptr<Figure> removeChild(Figure* child);

    const std::vector<std::unique_ptr<Figure>>& getChildren() const { return m_children; }
    std::vector<std::unique_ptr<Figure>>& getChildren() { return m_children; }

    void draw(sf::RenderTarget& target) const override;
    bool contains(sf::Vector2f point) const override;
    sf::FloatRect getBoundingBox() const override;

    const std::vector<sf::Vector2f>& getVertices() const override;
    std::vector<sf::Vector2f>& getVerticesMutable() override;

    void move(sf::Vector2f delta) override;

    static std::string makeDefaultName(std::size_t index) {
        return "Child " + std::to_string(index + 1);
    }

private:
    std::vector<std::unique_ptr<Figure>> m_children;
    mutable std::vector<sf::Vector2f> m_boundingVerts;

    sf::Vector2f applyCompositeToWorldPoint(sf::Vector2f point) const;
    sf::Vector2f applyInverseCompositeToWorldPoint(sf::Vector2f point) const;
    void rebuildBoundingVerts() const;
};

} // namespace core
