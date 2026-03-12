#pragma once

#include <imgui.h>

namespace ui {

    enum class Tool {
        Select,
        Rectangle,
        Triangle,
        Hexagon,
        Rhombus,
        Trapezoid,
        Circle
    };

    class Toolbar {
    public:
        Toolbar() = default;

        bool render(Tool& currentTool);
    };

} // namespace ui
