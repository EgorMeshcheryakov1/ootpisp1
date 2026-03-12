#pragma once

#include <SFML/Graphics.hpp>
#include <vector>

namespace core {

    struct Edge {
        float width = 1.0f;
        sf::Color color = sf::Color::Black;

        float flashTime = 0.0f;
        float flashDuration = 0.6f;
        bool flashEnabled = false;
    };

    class Figure {
    public:
        virtual ~Figure() = default;

        sf::Vector2f anchor{ 0.f, 0.f };
        sf::Vector2f parentOrigin{ 0.f, 0.f };
        sf::Color fillColor = sf::Color::White;
        float rotationAngle = 0.f;
        sf::Vector2f scale{ 1.f, 1.f };

        std::vector<Edge> edges;

        sf::FloatRect getBoundingBox() const;
        sf::FloatRect getLocalBoundingBox() const;

        virtual const std::vector<sf::Vector2f>& getVertices() const {
            return m_vertices;
        }

        virtual std::vector<sf::Vector2f>& getVerticesMutable() {
            return m_vertices;
        }

        sf::Vector2f getAbsoluteVertex(sf::Vector2f relative) const;
        virtual void draw(sf::RenderTarget& target) const;
        virtual bool contains(sf::Vector2f point) const;

        void move(sf::Vector2f delta);
        void resetAnchor();
        void setAnchorKeepAbsolute(sf::Vector2f newAnchor);
        void applyScale();

        virtual bool hasUniformEdge() const { return false; }
        virtual bool hasSideLengths() const { return false; }
        virtual const char* getSideName(int /*idx*/) const { return "Side"; }

        virtual std::vector<float> getSideLengths() const;
        virtual void setSideLengths(const std::vector<float>& /*lengths*/) {}

        std::vector<bool> lockedSides;
        std::vector<float> lockedLengths;

        void applyGenericSideLengths(const std::vector<float>& lengths);

        void highlightEdge(size_t idx, float duration = 0.6f) {
            if (idx >= edges.size()) return;
            edges[idx].flashEnabled = true;
            edges[idx].flashTime = duration;
            edges[idx].flashDuration = duration;
        }

        void updateEdgeEffects(float dt) {
            for (auto& edge : edges) {
                if (!edge.flashEnabled) continue;

                edge.flashTime -= dt;
                if (edge.flashTime <= 0.f) {
                    edge.flashTime = 0.f;
                    edge.flashEnabled = false;
                }
            }
        }

    protected:
        std::vector<sf::Vector2f> m_vertices;
    };

} // namespace core
