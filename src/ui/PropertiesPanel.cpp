#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include "PropertiesPanel.hpp"
#include "core/Figures.hpp"
#include "core/PolylineShape.hpp"
#include "core/CompositeFigure.hpp"

#include <algorithm>
#include <cmath>
#include <vector>
#include <imgui.h>

namespace ui {

bool PropertiesPanel::render(core::Scene& scene, core::Viewport& viewport) {
    core::Figure* selectedFigure = scene.getSelectedFigure();
    bool fitRequested = false;

    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 300, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(300, ImGui::GetIO().DisplaySize.y), ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove    | ImGuiWindowFlags_NoCollapse;

    ImGui::Begin(u8"\u0421\u0432\u043e\u0439\u0441\u0442\u0432\u0430", nullptr, flags);

    // ── Viewport controls ─────────────────────────────────────────
    ImGui::TextUnformatted(u8"\u0421\u0434\u0432\u0438\u0433 \u0432\u0438\u0434\u0430");
    float origin[2] = { viewport.worldOrigin.x, viewport.worldOrigin.y };
    if (ImGui::DragFloat2("##ViewportPan", origin, 1.0f)) {
        viewport.worldOrigin.x = origin[0];
        viewport.worldOrigin.y = origin[1];
    }
    if (ImGui::Button(u8"\u0421\u0431\u0440\u043e\u0441\u0438\u0442\u044c \u0432\u0438\u0434 \u043a (0,0)"))
        viewport.worldOrigin = sf::Vector2f(0.f, 0.f);

    ImGui::Separator();
    ImGui::TextUnformatted(u8"\u0422\u043e\u0447\u043a\u0430 \u043e\u0442\u0441\u0447\u0451\u0442\u0430");
    if (!scene.customOriginActive) {
        ImGui::TextDisabled(u8"(x) \u0413\u043b\u043e\u0431\u0430\u043b\u044c\u043d\u0430\u044f (0, 0)");
        ImGui::TextDisabled(u8"( ) \u041f\u043e\u043b\u044c\u0437\u043e\u0432\u0430\u0442\u0435\u043b\u044c\u0441\u043a\u0430\u044f");
    } else {
        ImGui::TextDisabled(u8"( ) \u0413\u043b\u043e\u0431\u0430\u043b\u044c\u043d\u0430\u044f (0, 0)");
        ImGui::TextDisabled(u8"(x) \u041f\u043e\u043b\u044c\u0437\u043e\u0432\u0430\u0442\u0435\u043b\u044c\u0441\u043a\u0430\u044f");
        float custOrigin[2] = { scene.customOriginPos.x, scene.customOriginPos.y };
        ImGui::InputFloat2("##CustomOrigin", custOrigin, "%.1f", ImGuiInputTextFlags_ReadOnly);
        if (ImGui::Button(u8"\u0421\u0431\u0440\u043e\u0441")) scene.resetCustomOrigin();
    }
    ImGui::Checkbox(u8"\u0420\u0438\u0441\u043e\u0432\u0430\u0442\u044c \u043f\u043e\u0432\u0435\u0440\u0445 \u0444\u0438\u0433\u0443\u0440", &m_drawOriginsOverFigures);

    ImGui::Separator();
    ImGui::TextUnformatted(u8"\u041c\u0430\u0441\u0448\u0442\u0430\u0431");
    if (ImGui::Button("-"))
        viewport.zoomAt(sf::Vector2f(ImGui::GetIO().DisplaySize.x/2.f, ImGui::GetIO().DisplaySize.y/2.f), 1.f/1.1f);
    ImGui::SameLine();
    float zoomPct = viewport.zoom * 100.f;
    ImGui::SetNextItemWidth(80.f);
    if (ImGui::DragFloat("##Zoom", &zoomPct, 5.0f, 5.0f, 5000.0f, "%.0f%%")) {
        float oldZoom = viewport.zoom;
        viewport.zoom = std::clamp(zoomPct / 100.f, 0.05f, 50.f);
        sf::Vector2f screenCenter(ImGui::GetIO().DisplaySize.x/2.f, ImGui::GetIO().DisplaySize.y/2.f);
        sf::Vector2f worldPoint = (screenCenter - viewport.worldOrigin) / oldZoom;
        viewport.worldOrigin = screenCenter - worldPoint * viewport.zoom;
    }
    ImGui::SameLine();
    if (ImGui::Button("+"))
        viewport.zoomAt(sf::Vector2f(ImGui::GetIO().DisplaySize.x/2.f, ImGui::GetIO().DisplaySize.y/2.f), 1.1f);
    if (ImGui::Button(u8"\u041f\u043e\u043a\u0430\u0437\u0430\u0442\u044c \u0432\u0441\u0451")) fitRequested = true;

    ImGui::Separator();
    ImGui::Spacing();

    if (!selectedFigure) {
        if (scene.getSelection().size() > 1) {
            ImGui::TextUnformatted(u8"\u0412\u044b\u0431\u0440\u0430\u043d\u043e \u0444\u0438\u0433\u0443\u0440: ");
            ImGui::SameLine();
            ImGui::Text("%zu", scene.getSelection().size());

            std::vector<core::CompositeFigure*> composites;
            for (const auto& fig : scene.getFigures()) {
                auto* c = dynamic_cast<core::CompositeFigure*>(fig.get());
                if (c) composites.push_back(c);
            }
            if (!composites.empty()) {
                ImGui::Separator();
                ImGui::TextUnformatted(u8"\u0414\u043e\u0431\u0430\u0432\u0438\u0442\u044c \u0432 \u0441\u043e\u0441\u0442\u0430\u0432\u043d\u0443\u044e:");
                for (size_t ci = 0; ci < composites.size(); ++ci) {
                    ImGui::PushID((int)ci);
                    char label[64];
                    std::snprintf(label, sizeof(label), u8"\u0421\u043e\u0441\u0442\u0430\u0432\u043d\u0430\u044f %zu", ci + 1);
                    if (ImGui::SmallButton(label)) {
                        // Safe: collect raw ptrs first, then extract owned ptrs
                        auto selCopy = scene.getSelection();
                        std::vector<std::unique_ptr<core::Figure>> toAdd;
                        for (core::Figure* f : selCopy) {
                            if (dynamic_cast<core::CompositeFigure*>(f) == composites[ci]) continue;
                            for (auto& up : const_cast<std::vector<std::unique_ptr<core::Figure>>&>(scene.getFigures())) {
                                if (up.get() == f) { toAdd.push_back(std::move(up)); break; }
                            }
                        }
                        for (auto& owned : toAdd) {
                            scene.removeFigure(owned.get());
                            composites[ci]->addChild(std::move(owned));
                        }
                        scene.clearSelection();
                    }
                    ImGui::PopID();
                }
            }
        } else {
            ImGui::TextDisabled(u8"\u0424\u0438\u0433\u0443\u0440\u0430 \u043d\u0435 \u0432\u044b\u0431\u0440\u0430\u043d\u0430.");
        }
        ImGui::End();
        return fitRequested;
    }

    ImGui::TextUnformatted(u8"\u0421\u0432\u043e\u0439\u0441\u0442\u0432\u0430 \u0444\u0438\u0433\u0443\u0440\u044b");
    ImGui::Separator();
    ImGui::Spacing();

    // ── Composite figure editor ────────────────────────────────────────────────
    auto* composite = dynamic_cast<core::CompositeFigure*>(selectedFigure);
    if (composite) {
        if (ImGui::TreeNodeEx(u8"\u0421\u043e\u0441\u0442\u0430\u0432\u043d\u0430\u044f \u0444\u0438\u0433\u0443\u0440\u0430", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& children = composite->getChildren();
            ImGui::Text(u8"\u0414\u0435\u0442\u0435\u0439: %