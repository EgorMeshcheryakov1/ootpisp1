#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include "Toolbar.hpp"

namespace ui {

    bool Toolbar::render(Tool& currentTool) {
        bool toolChanged = false;

        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(90, ImGui::GetIO().DisplaySize.y), ImGuiCond_Always);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::Begin(u8"Панель", nullptr, flags);

        auto renderToolButton = [&](const char* label, Tool expectedTool) {
            bool isActive = (currentTool == expectedTool);

            if (isActive) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
            }

            if (ImGui::Button(label, ImVec2(-1, 40))) {
                if (!isActive) {
                    currentTool = expectedTool;
                    toolChanged = true;
                }
            }

            if (isActive) {
                ImGui::PopStyleColor();
            }
            };

        renderToolButton(u8"Выбор", Tool::Select);
        ImGui::Separator();
        renderToolButton(u8"Прям.", Tool::Rectangle);
        renderToolButton(u8"Треуг.", Tool::Triangle);
        renderToolButton(u8"Шестиуг.", Tool::Hexagon);
        renderToolButton(u8"Ромб", Tool::Rhombus);
        renderToolButton(u8"Трапеция", Tool::Trapezoid);
        renderToolButton(u8"Круг", Tool::Circle);

        ImGui::End();
        return toolChanged;
    }

} // namespace ui
