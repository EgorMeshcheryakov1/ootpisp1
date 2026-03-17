#pragma once

#include "Figure.hpp"
#include <memory>
#include <vector>
#include <algorithm>

namespace core {

class Scene {
public:
  void addFigure(std::unique_ptr<Figure> fig);

  // Removes the given figure from the scene. Returns true if removed.
  bool removeFigure(Figure *fig);

  // Returns the top-most figure at the given absolute point, or nullptr if none
  Figure *hitTest(sf::Vector2f point) const;

  // Draw all figures
  void drawAll(sf::RenderTarget &target, float markerScale = 1.0f) const;

  const std::vector<std::unique_ptr<Figure>> &getFigures() const {
    return m_figures;
  }

  // ── Single selection ─────────────────────────────────────────────────────
  void setSelectedFigure(Figure *fig);
  Figure *getSelectedFigure() const { return m_selectedFigure; }

  // ── Multi-selection ──────────────────────────────────────────────────────
  void addToSelection(Figure *fig);
  void removeFromSelection(Figure *fig);
  void clearSelection();
  bool isSelected(Figure *fig) const;
  const std::vector<Figure*>& getSelection() const { return m_selection; }

  // World Origin properties
  bool customOriginActive = false;
  sf::Vector2f customOriginPos{0.f, 0.f};

  void setCustomOrigin(sf::Vector2f newOriginWorld);
  void resetCustomOrigin();

private:
  std::vector<std::unique_ptr<Figure>> m_figures;
  Figure *m_selectedFigure = nullptr;
  std::vector<Figure*> m_selection; // multi-selection list
};

} // namespace core
