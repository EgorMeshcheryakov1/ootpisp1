#include "Scene.hpp"
#include "utils/GeometryUtils.hpp"

namespace core {

void Scene::addFigure(std::unique_ptr<Figure> fig) {
  if (fig) {
    m_figures.push_back(std::move(fig));
  }
}

bool Scene::removeFigure(Figure *fig) {
  removeFromSelection(fig);
  if (m_selectedFigure == fig) m_selectedFigure = nullptr;
  for (auto it = m_figures.begin(); it != m_figures.end(); ++it) {
    if (it->get() == fig) {
      m_figures.erase(it);
      return true;
    }
  }
  return false;
}

Figure *Scene::hitTest(sf::Vector2f point) const {
  for (auto it = m_figures.rbegin(); it != m_figures.rend(); ++it) {
    if ((*it)->contains(point)) {
      return it->get();
    }
  }
  return nullptr;
}

// ── Selection helpers ───────────────────────────────────────────────────────

void Scene::setSelectedFigure(Figure *fig) {
  m_selectedFigure = fig;
  m_selection.clear();
  if (fig) m_selection.push_back(fig);
}

void Scene::addToSelection(Figure *fig) {
  if (!fig) return;
  if (!isSelected(fig)) m_selection.push_back(fig);
  m_selectedFigure = fig; // last selected becomes "primary"
}

void Scene::removeFromSelection(Figure *fig) {
  m_selection.erase(std::remove(m_selection.begin(), m_selection.end(), fig), m_selection.end());
  if (m_selectedFigure == fig) {
    m_selectedFigure = m_selection.empty() ? nullptr : m_selection.back();
  }
}

void Scene::clearSelection() {
  m_selection.clear();
  m_selectedFigure = nullptr;
}

bool Scene::isSelected(Figure *fig) const {
  return std::find(m_selection.begin(), m_selection.end(), fig) != m_selection.end();
}

// ── Drawing ─────────────────────────────────────────────────────────────────

void Scene::drawAll(sf::RenderTarget &target, float markerScale) const {
  for (const auto &fig : m_figures) {
    fig->draw(target);
  }

  // Draw selection highlight for every selected figure
  for (Figure* sel : m_selection) {
    sf::FloatRect bounds = sel->getBoundingBox();
    sf::RectangleShape bbox(sf::Vector2f(bounds.width, bounds.height));
    bbox.setPosition(bounds.left, bounds.top);
    bbox.setFillColor(sf::Color::Transparent);
    // Primary selection = bright blue, others = lighter blue
    bool isPrimary = (sel == m_selectedFigure);
    bbox.setOutlineColor(isPrimary ? sf::Color(0, 120, 215) : sf::Color(100, 170, 240));
    bbox.setOutlineThickness(1.f * markerScale);
    target.draw(bbox);

    const float ms = 6.f * markerScale;
    sf::RectangleShape marker(sf::Vector2f(ms, ms));
    marker.setFillColor(sf::Color::White);
    marker.setOutlineColor(isPrimary ? sf::Color(0, 120, 215) : sf::Color(100, 170, 240));
    marker.setOutlineThickness(1.f * markerScale);
    marker.setOrigin(ms / 2.f, ms / 2.f);

    marker.setPosition(bounds.left, bounds.top); target.draw(marker);
    marker.setPosition(bounds.left + bounds.width, bounds.top); target.draw(marker);
    marker.setPosition(bounds.left + bounds.width, bounds.top + bounds.height); target.draw(marker);
    marker.setPosition(bounds.left, bounds.top + bounds.height); target.draw(marker);
  }
}

// ── World Origin ─────────────────────────────────────────────────────────────

void Scene::setCustomOrigin(sf::Vector2f newOriginWorld) {
  for (auto &figure : m_figures) {
    if (!customOriginActive) {
      figure->parentOrigin = newOriginWorld;
      figure->anchor -= newOriginWorld;
    } else {
      sf::Vector2f delta = newOriginWorld - customOriginPos;
      figure->parentOrigin = newOriginWorld;
      figure->anchor -= delta;
    }
  }
  customOriginPos = newOriginWorld;
  customOriginActive = true;
}

void Scene::resetCustomOrigin() {
  if (!customOriginActive)
    return;
  for (auto &figure : m_figures) {
    figure->anchor += customOriginPos;
    figure->parentOrigin = sf::Vector2f(0.f, 0.f);
  }
  customOriginActive = false;
}

} // namespace core
