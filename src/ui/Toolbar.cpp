#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include "Toolbar.hpp"

namespace ui {

    bool Toolbar::render(Tool& currentTool, bool groupEnabled, bool* groupClicked,
                         bool ungroupEnabled, bool* ungroupClicked) {
        bool toolChanged = false;

        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(90, ImGui::GetIO().DisplaySize.y), ImGuiCond_Always);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::Begin(u8"\u041f\u0430\u043d\u0435\u043b\u044c", nullptr, flags);

        auto renderToolButton = [&](const char* label, Tool expectedTool) {
            bool isActive = (currentTool == expectedTool);
            if (isActive)
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
            if (ImGui::Button(label, ImVec2(-1, 40))) {
                if (!isActive) { currentTool = expectedTool; toolChanged = true; }
            }
            if (isActive) ImGui::PopStyleColor();
        };

        renderToolButton(u8"\u0412\u044b\u0431\u043e\u0440",    Tool::Select);
        ImGui::Separator();
        renderToolButton(u8"\u041f\u0440\u044f\u043c.",         Tool::Rectangle);
        renderToolButton(u8"\u0422\u0440\u0435\u0443\u0433.",   Tool::Triangle);
        renderToolButton(u8"\u0428\u0435\u0441\u0442\u0438\u0443\u0433.", Tool::Hexagon);
        renderToolButton(u8"\u0420\u043e\u043c\u0431",          Tool::Rhombus);
        renderToolButton(u8"\u0422\u0440\u0430\u043f\u0435\u0446\u0438\u044f", Tool::Trapezoid);
        renderToolButton(u8"\u041a\u0440\u0443\u0433",          Tool::Circle);
        renderToolButton(u8"\u041f\u043e\u043b\u0438\u043b\u0438\u043d\u0438\u044f", Tool::Polyline);

        ImGui::Separator();

        // Group / Ungroup buttons
        if (!groupEnabled) ImGui::BeginDisabled();
        if (ImGui::Button(u8"\u0421\u0433\u0440\u0443\u043f\u043f.", ImVec2(-1, 36))) {
            if (groupClicked) *groupClicked = true;
        }
        if (!groupEnabled) ImGui::EndDisabled();

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip(u8"\u0412\u044b\u0434\u0435\u043b\u0438\u0442\u0435 >= 2 \u0444\u0438\u0433\u0443\u0440");

        if (!ungroupEnabled) ImGui::BeginDisabled();
        if (ImGui::Button(u8"\u0420\u0430\u0437\u0433\u0440\u0443\u043f\u043f.", ImVec2(-1, 36))) {
            if (ungroupClicked) *ungroupClicked = true;
        }
        if (!ungroupEnabled) ImGui::EndDisabled();

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip(u8"\u0412\u044b\u0431\u0435\u0440\u0438\u0442\u0435 \u0441\u043e\u0441\u0442\u0430\u0432\u043d\u0443\u044e \u0444\u0438\u0433\u0443\u0440\u0443");

        ImGui::End();
        return toolChanged;
    }

} // namespace ui
