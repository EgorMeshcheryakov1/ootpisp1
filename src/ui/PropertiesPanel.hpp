#pragma once

#include "core/Scene.hpp"
#include "core/Viewport.hpp"

namespace ui {

class PropertiesPanel {
public:
    bool render(core::Scene& scene, core::Viewport& viewport);

    bool m_lockAnchor       = false;
    bool m_lockProportions  = false;
    bool m_drawOriginsOverFigures = false;
};

} // namespace ui
