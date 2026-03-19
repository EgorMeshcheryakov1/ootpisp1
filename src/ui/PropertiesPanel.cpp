#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include "PropertiesPanel.hpp"
#include "core/CompositeFigure.hpp"
#include "core/PolylineShape.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <imgui.h>
#include <string>

namespace ui {
namespace {

std::array<float, 4> colorToArray(const sf::Color& color) {
    return {color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f};
}

sf::Color arrayToColor(const std::array<float, 4>& value) {
    return sf::Color(
        static_cast<sf::Uint8>(std::clamp(value[0], 0.f, 1.f) * 255.f),
        static_cast<sf::Uint8>(std::clamp(value[1], 0.f, 1.f) * 255.f),
        static_cast<sf::Uint8>(std::clamp(value[2], 0.f, 1.f) * 255.f),
        static_cast<sf::Uint8>(std::clamp(value[3], 0.f, 1.f) * 255.f));
}

void renderFigureGeometry(core::Figure* figure, int& selectedSegmentIndex) {
    if (!figure)
        return;

    float anchor[2] = {figure->anchor.x, figure->anchor.y};
    if (ImGui::DragFloat2(u8"Положение", anchor, 1.f)) {
        figure->anchor.x = anchor[0];
        figure->anchor.y = anchor[1];
    }

    ImGui::DragFloat(u8"Поворот", &figure->rotationAngle, 0.5f, -3600.f, 3600.f, "%.1f°");
    float scale[2] = {figure->scale.x, figure->scale.y};
    if (ImGui::DragFloat2(u8"Масштаб", scale, 0.01f, -20.f, 20.f, "%.2f")) {
        if (std::abs(scale[0]) < 0.01f) scale[0] = (scale[0] < 0.f ? -0.01f : 0.01f);
        if (std::abs(scale[1]) < 0.01f) scale[1] = (scale[1] < 0.f ? -0.01f : 0.01f);
        figure->scale.x = scale[0];
        figure->scale.y = scale[1];
    }

    auto fill = colorToArray(figure->fillColor);
    if (ImGui::ColorEdit4(u8"Заливка", fill.data())) {
        figure->fillColor = arrayToColor(fill);
    }

    if (!figure->edges.empty() && ImGui::TreeNodeEx(u8"Контур", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (size_t i = 0; i < figure->edges.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));
            bool selected = selectedSegmentIndex == static_cast<int>(i);
            char header[64];
            std::snprintf(header, sizeof(header), "%s %zu", u8"Ребро", i + 1);
            if (ImGui::Selectable(header, selected)) {
                selectedSegmentIndex = static_cast<int>(i);
            }
            auto edgeColor = colorToArray(figure->edges[i].color);
            ImGui::DragFloat(u8"Толщина", &figure->edges[i].width, 0.2f, 0.f, 100.f, "%.1f");
            if (ImGui::ColorEdit4(u8"Цвет", edgeColor.data())) {
                figure->edges[i].color = arrayToColor(edgeColor);
            }
            ImGui::Separator();
            ImGui::PopID();
        }
        ImGui::TreePop();
    }

    if (figure->hasSideLengths() && ImGui::TreeNodeEx(u8"Длины сторон", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto lengths = figure->getSideLengths();
        bool changed = false;
        for (size_t i = 0; i < lengths.size(); ++i) {
            ImGui::PushID(static_cast<int>(1000 + i));
            float length = lengths[i];
            if (ImGui::DragFloat(figure->getSideName(static_cast<int>(i)), &length, 1.f, 1.f, 5000.f, "%.1f")) {
                lengths[i] = length;
                changed = true;
                selectedSegmentIndex = static_cast<int>(i);
            }
            ImGui::PopID();
        }
        if (changed) {
            figure->setSideLengths(lengths);
        }
        ImGui::TreePop();
    }

    if (auto* polyline = dynamic_cast<core::PolylineShape*>(figure)) {
        if (ImGui::TreeNodeEx(u8"Сегменты", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& segments = polyline->getSegments();
            for (size_t i = 0; i < segments.size(); ++i) {
                ImGui::PushID(static_cast<int>(2000 + i));
                char label[64];
                std::snprintf(label, sizeof(label), "%s %zu", u8"Сегмент", i + 1);
                bool selected = selectedSegmentIndex == static_cast<int>(i);
                if (ImGui::Selectable(label, selected)) {
                    selectedSegmentIndex = static_cast<int>(i);
                }

                float length = segments[i].length;
                if (ImGui::DragFloat(u8"Длина", &length, 1.f, 1.f, 5000.f, "%.1f")) {
                    segments[i].length = std::max(1.f, length);
                    polyline->rebuild();
                    selectedSegmentIndex = static_cast<int>(i);
                }

                float angle = segments[i].angle;
                const char* angleLabel = (i == 0) ? u8"Начальный угол" : u8"Угол к соседнему";
                if (ImGui::DragFloat(angleLabel, &angle, 0.5f, -360.f, 360.f, "%.1f°")) {
                    polyline->setSegmentAngle(i, angle);
                    selectedSegmentIndex = static_cast<int>(i);
                }
                ImGui::Separator();
                ImGui::PopID();
            }
            ImGui::TreePop();
        }
    }
}

} // namespace

bool PropertiesPanel::render(core::Scene& scene, core::Viewport& viewport) {
    core::Figure* selectedFigure = scene.getSelectedFigure();
    bool fitRequested = false;

    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 300, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(300, ImGui::GetIO().DisplaySize.y), ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

    ImGui::Begin(u8"Свойства", nullptr, flags);

    ImGui::TextUnformatted(u8"Сдвиг вида");
    float origin[2] = {viewport.worldOrigin.x, viewport.worldOrigin.y};
    if (ImGui::DragFloat2("##ViewportPan", origin, 1.0f)) {
        viewport.worldOrigin.x = origin[0];
        viewport.worldOrigin.y = origin[1];
    }
    if (ImGui::Button(u8"Сбросить вид к (0,0)"))
        viewport.worldOrigin = sf::Vector2f(0.f, 0.f);

    ImGui::Separator();
    ImGui::TextUnformatted(u8"Точка отсчёта");
    if (!scene.customOriginActive) {
        ImGui::TextDisabled(u8"(x) Глобальная (0, 0)");
        ImGui::TextDisabled(u8"( ) Пользовательская");
    } else {
        ImGui::TextDisabled(u8"( ) Глобальная (0, 0)");
        ImGui::TextDisabled(u8"(x) Пользовательская");
        float customOrigin[2] = {scene.customOriginPos.x, scene.customOriginPos.y};
        ImGui::InputFloat2("##CustomOrigin", customOrigin, "%.1f", ImGuiInputTextFlags_ReadOnly);
        if (ImGui::Button(u8"Сброс"))
            scene.resetCustomOrigin();
    }
    ImGui::Checkbox(u8"Рисовать поверх фигур", &m_drawOriginsOverFigures);

    ImGui::Separator();
    ImGui::TextUnformatted(u8"Масштаб");
    if (ImGui::Button("-"))
        viewport.zoomAt(sf::Vector2f(ImGui::GetIO().DisplaySize.x / 2.f, ImGui::GetIO().DisplaySize.y / 2.f), 1.f / 1.1f);
    ImGui::SameLine();
    float zoomPct = viewport.zoom * 100.f;
    ImGui::SetNextItemWidth(80.f);
    if (ImGui::DragFloat("##Zoom", &zoomPct, 5.0f, 5.0f, 5000.0f, "%.0f%%")) {
        float oldZoom = viewport.zoom;
        viewport.zoom = std::clamp(zoomPct / 100.f, 0.05f, 50.f);
        sf::Vector2f screenCenter(ImGui::GetIO().DisplaySize.x / 2.f, ImGui::GetIO().DisplaySize.y / 2.f);
        sf::Vector2f worldPoint = (screenCenter - viewport.worldOrigin) / oldZoom;
        viewport.worldOrigin = screenCenter - worldPoint * viewport.zoom;
    }
    ImGui::SameLine();
    if (ImGui::Button("+"))
        viewport.zoomAt(sf::Vector2f(ImGui::GetIO().DisplaySize.x / 2.f, ImGui::GetIO().DisplaySize.y / 2.f), 1.1f);
    if (ImGui::Button(u8"Показать всё"))
        fitRequested = true;

    ImGui::Separator();
    ImGui::Spacing();

    if (!selectedFigure) {
        if (scene.getSelection().size() > 1) {
            ImGui::Text(u8"Выбрано фигур: %zu", scene.getSelection().size());
        } else {
            ImGui::TextDisabled(u8"Фигура не выбрана.");
        }
        ImGui::End();
        return fitRequested;
    }

    ImGui::TextUnformatted(u8"Свойства фигуры");
    ImGui::Separator();

    if (auto* composite = dynamic_cast<core::CompositeFigure*>(selectedFigure)) {
        if (ImGui::TreeNodeEx(u8"Составная фигура", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text(u8"Детей: %zu", composite->getChildren().size());

            if (scene.getSelection().size() > 1 && ImGui::Button(u8"Добавить выбранные в составную")) {
                auto selection = scene.getSelection();
                for (core::Figure* figure : selection) {
                    if (!figure || figure == composite)
                        continue;
                    auto owned = scene.takeFigure(figure);
                    if (owned)
                        composite->addChild(std::move(owned));
                }
                scene.setSelectedFigure(composite);
            }

            core::Figure* childToSelect = nullptr;
            core::Figure* childToRemove = nullptr;
            for (size_t i = 0; i < composite->getChildren().size(); ++i) {
                auto& child = composite->getChildren()[i];
                if (!child)
                    continue;

                ImGui::PushID(static_cast<int>(3000 + i));
                std::string childLabel = core::CompositeFigure::makeDefaultName(i);
                if (ImGui::TreeNode(childLabel.c_str())) {
                    if (ImGui::Button(u8"Редактировать"))
                        childToSelect = child.get();
                    ImGui::SameLine();
                    if (ImGui::Button(u8"Извлечь"))
                        childToRemove = child.get();
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }

            if (childToRemove) {
                auto detached = composite->removeChild(childToRemove);
                if (detached) {
                    core::Figure* raw = detached.get();
                    scene.addFigure(std::move(detached));
                    scene.setSelectedFigure(raw);
                }
            } else if (childToSelect) {
                scene.setSelectedFigure(childToSelect);
                selectedFigure = childToSelect;
            }
            ImGui::TreePop();
            ImGui::Separator();
        }
    }

    renderFigureGeometry(selectedFigure, m_selectedSegmentIndex);

    ImGui::End();
    return fitRequested;
}

} // namespace ui
