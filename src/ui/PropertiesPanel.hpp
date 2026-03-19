#pragma once

#include "core/Scene.hpp"
#include "core/Viewport.hpp"

namespace ui {

class PropertiesPanel {
public:
    bool render(core::Scene& scene, core::Viewport& viewport);

    bool m_lockAnchor             = false;
    bool m_lockProportions        = false;
    bool m_drawOriginsOverFigures = false;

    // Index of the polyline segment currently hovered/selected in the panel.
    // -1 means none. main.cpp reads this to draw the highlight.
    int  m_selectedSegmentIndex   = -1;
    bool m_selectedSegmentIsAngle = false;
};

} // namespace ui
