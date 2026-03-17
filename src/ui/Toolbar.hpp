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
        Circle,
        Polyline   // new: polyline/polygon via segments
    };

    class Toolbar {
    public:
        Toolbar() = default;

        // Returns true if tool changed.
        // groupEnabled: show the "Group" button (active when multi-selection exists)
        bool render(Tool& currentTool, bool groupEnabled = false, bool* groupClicked = nullptr,
                    bool ungroupEnabled = false, bool* ungroupClicked = nullptr);
    };

} // namespace ui
