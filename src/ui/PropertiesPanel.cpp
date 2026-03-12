#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include "PropertiesPanel.hpp"
#include "core/Figures.hpp"

#include <algorithm>
#include <vector>
#include <imgui.h>

namespace ui {

    bool PropertiesPanel::render(core::Scene& scene, core::Viewport& viewport) {
        core::Figure* selectedFigure = scene.getSelectedFigure();
        bool fitRequested = false;

        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 280, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(280, ImGui::GetIO().DisplaySize.y), ImGuiCond_Always);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse;

        ImGui::Begin(u8"Свойства", nullptr, flags);

        ImGui::TextUnformatted(u8"Сдвиг вида");
        float origin[2] = { viewport.worldOrigin.x, viewport.worldOrigin.y };
        if (ImGui::DragFloat2("##ViewportPan", origin, 1.0f)) {
            viewport.worldOrigin.x = origin[0];
            viewport.worldOrigin.y = origin[1];
        }

        if (ImGui::Button(u8"Сбросить вид к (0,0)")) {
            viewport.worldOrigin = sf::Vector2f(0.f, 0.f);
        }

        ImGui::Separator();
        ImGui::TextUnformatted(u8"Точка отсчёта");

        if (!scene.customOriginActive) {
            ImGui::TextDisabled(u8"(x) Глобальная (0, 0)");
            ImGui::TextDisabled(u8"( ) Пользовательская");
        }
        else {
            ImGui::TextDisabled(u8"( ) Глобальная (0, 0)");
            ImGui::TextDisabled(u8"(x) Пользовательская");

            float custOrigin[2] = { scene.customOriginPos.x, scene.customOriginPos.y };
            ImGui::InputFloat2("##CustomOrigin", custOrigin, "%.1f", ImGuiInputTextFlags_ReadOnly);

            if (ImGui::Button(u8"Сброс")) {
                scene.resetCustomOrigin();
            }
        }

        ImGui::Checkbox(u8"Рисовать поверх фигур", &m_drawOriginsOverFigures);

        ImGui::Separator();
        ImGui::TextUnformatted(u8"Масштаб");

        if (ImGui::Button("-")) {
            viewport.zoomAt(
                sf::Vector2f(ImGui::GetIO().DisplaySize.x / 2.f, ImGui::GetIO().DisplaySize.y / 2.f),
                1.f / 1.1f
            );
        }

        ImGui::SameLine();

        float zoomPct = viewport.zoom * 100.f;
        ImGui::SetNextItemWidth(80.f);
        if (ImGui::DragFloat("##Zoom", &zoomPct, 5.0f, 5.0f, 5000.0f, "%.0f%%")) {
            float oldZoom = viewport.zoom;
            viewport.zoom = std::clamp(zoomPct / 100.f, 0.05f, 50.f);

            sf::Vector2f screenCenter(ImGui::GetIO().DisplaySize.x / 2.f,
                ImGui::GetIO().DisplaySize.y / 2.f);
            sf::Vector2f worldPoint = (screenCenter - viewport.worldOrigin) / oldZoom;
            viewport.worldOrigin = screenCenter - worldPoint * viewport.zoom;
        }

        ImGui::SameLine();

        if (ImGui::Button("+")) {
            viewport.zoomAt(
                sf::Vector2f(ImGui::GetIO().DisplaySize.x / 2.f, ImGui::GetIO().DisplaySize.y / 2.f),
                1.1f
            );
        }

        if (ImGui::Button(u8"Показать всё")) {
            fitRequested = true;
        }

        ImGui::Separator();
        ImGui::Spacing();

        if (!selectedFigure) {
            ImGui::TextDisabled(u8"Фигура не выбрана.");
            ImGui::End();
            return fitRequested;
        }

        ImGui::TextUnformatted(u8"Свойства фигуры");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextUnformatted(u8"Точка привязки");
        ImGui::SameLine();
        ImGui::Checkbox(u8"Закрепить к фигуре", &m_lockAnchor);

        float anchor[2] = { selectedFigure->anchor.x, selectedFigure->anchor.y };
        if (ImGui::DragFloat2("##Anchor", anchor, 1.0f)) {
            if (m_lockAnchor) {
                selectedFigure->anchor = sf::Vector2f(anchor[0], anchor[1]);
            }
            else {
                selectedFigure->setAnchorKeepAbsolute(sf::Vector2f(anchor[0], anchor[1]));
            }
        }

        if (ImGui::Button(u8"Сбросить привязку")) {
            selectedFigure->resetAnchor();
        }

        ImGui::Spacing();

        auto& vertices = selectedFigure->getVerticesMutable();
        if (!vertices.empty() && !selectedFigure->hasUniformEdge()) {
            ImGui::Separator();
            ImGui::TextUnformatted(u8"Перемещение по координатам вершины");

            for (size_t i = 0; i < vertices.size(); ++i) {
                ImGui::PushID(static_cast<int>(i + 100));

                ImGui::Text("V%zu", i + 1);
                ImGui::SameLine();

                sf::Vector2f absV = selectedFigure->getAbsoluteVertex(vertices[i]);
                sf::Vector2f displayV = absV - selectedFigure->parentOrigin;
                float v[2] = { displayV.x, displayV.y };

                if (ImGui::DragFloat2("##v", v, 0.5f)) {
                    sf::Vector2f newAbs(v[0], v[1]);
                    newAbs += selectedFigure->parentOrigin;
                    sf::Vector2f delta = newAbs - absV;
                    selectedFigure->move(delta);
                }

                ImGui::PopID();
            }

            ImGui::Spacing();
        }

        ImGui::Separator();
        ImGui::TextUnformatted(u8"Цвет заливки");

        float fill[4] = {
            selectedFigure->fillColor.r / 255.f,
            selectedFigure->fillColor.g / 255.f,
            selectedFigure->fillColor.b / 255.f,
            selectedFigure->fillColor.a / 255.f
        };

        if (ImGui::ColorEdit4("##FillColor", fill)) {
            selectedFigure->fillColor.r = static_cast<sf::Uint8>(fill[0] * 255.f);
            selectedFigure->fillColor.g = static_cast<sf::Uint8>(fill[1] * 255.f);
            selectedFigure->fillColor.b = static_cast<sf::Uint8>(fill[2] * 255.f);
            selectedFigure->fillColor.a = static_cast<sf::Uint8>(fill[3] * 255.f);
        }

        ImGui::Spacing();

        ImGui::Separator();
        ImGui::TextUnformatted(u8"Поворот");

        float rotation = selectedFigure->rotationAngle;
        if (ImGui::DragFloat(u8"Угол", &rotation, 1.0f, -360.0f, 360.0f, "%.1f")) {
            selectedFigure->rotationAngle = rotation;
        }

        if (ImGui::Button(u8"Сбросить поворот")) {
            selectedFigure->rotationAngle = 0.f;
        }

        ImGui::Spacing();

        ImGui::Separator();
        ImGui::TextUnformatted(u8"Масштаб");
        ImGui::Checkbox(u8"Сохранять пропорции", &m_lockProportions);

        float scalePct[2] = { selectedFigure->scale.x * 100.f, selectedFigure->scale.y * 100.f };
        if (ImGui::DragFloat2(u8"Масштаб %##Scale", scalePct, 1.0f, 1.0f, 10000.f, "%.1f%%")) {
            if (m_lockProportions) {
                if (scalePct[0] != selectedFigure->scale.x * 100.f) {
                    scalePct[1] = scalePct[0];
                }
                else if (scalePct[1] != selectedFigure->scale.y * 100.f) {
                    scalePct[0] = scalePct[1];
                }
            }

            selectedFigure->scale.x = scalePct[0] / 100.f;
            selectedFigure->scale.y = scalePct[1] / 100.f;
        }

        if (ImGui::Button(u8"Сбросить масштаб")) {
            selectedFigure->scale = sf::Vector2f(1.f, 1.f);
        }

        ImGui::SameLine();

        if (ImGui::Button(u8"Применить масштаб")) {
            selectedFigure->applyScale();
        }

        ImGui::Spacing();

        bool hasLengths = selectedFigure->hasSideLengths();
        if (!selectedFigure->edges.empty() || hasLengths) {
            ImGui::Separator();

            if (ImGui::TreeNodeEx(u8"Грани и стороны", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (selectedFigure->hasUniformEdge()) {
                    ImGui::PushID("UniformEdge");

                    float width = selectedFigure->edges.empty() ? 1.f : selectedFigure->edges[0].width;
                    if (ImGui::DragFloat(u8"Толщина", &width, 0.5f, 0.f, 100.f)) {
                        for (auto& edge : selectedFigure->edges) {
                            edge.width = width;
                        }
                    }

                    if (!selectedFigure->edges.empty()) {
                        float eCol[4] = {
                            selectedFigure->edges[0].color.r / 255.f,
                            selectedFigure->edges[0].color.g / 255.f,
                            selectedFigure->edges[0].color.b / 255.f,
                            selectedFigure->edges[0].color.a / 255.f
                        };

                        if (ImGui::ColorEdit4(u8"Цвет", eCol)) {
                            for (auto& edge : selectedFigure->edges) {
                                edge.color.r = static_cast<sf::Uint8>(eCol[0] * 255.f);
                                edge.color.g = static_cast<sf::Uint8>(eCol[1] * 255.f);
                                edge.color.b = static_cast<sf::Uint8>(eCol[2] * 255.f);
                                edge.color.a = static_cast<sf::Uint8>(eCol[3] * 255.f);
                            }
                        }
                    }

                    ImGui::PopID();
                    ImGui::Spacing();
                }
                else {
                    auto actualLengths = hasLengths ? selectedFigure->getSideLengths() : std::vector<float>();
                    auto& lockedSides = selectedFigure->lockedSides;
                    auto& lockedLengths = selectedFigure->lockedLengths;

                    if (lockedSides.size() != selectedFigure->edges.size()) {
                        lockedSides.resize(selectedFigure->edges.size(), false);
                        lockedLengths.resize(selectedFigure->edges.size(), 1.0f);

                        for (size_t i = 0; i < actualLengths.size(); ++i) {
                            if (i < lockedLengths.size()) {
                                lockedLengths[i] = actualLengths[i];
                            }
                        }
                    }

                    std::vector<float> displayLengths = actualLengths;
                    for (size_t i = 0; i < displayLengths.size(); ++i) {
                        if (lockedSides[i]) {
                            displayLengths[i] = lockedLengths[i];
                        }
                    }

                    bool anyLengthChanged = false;

                    for (size_t i = 0; i < selectedFigure->edges.size(); ++i) {
                        ImGui::PushID(static_cast<int>(i));

                        if (hasLengths && i < displayLengths.size()) {
                            bool isLocked = lockedSides[i];
                            if (ImGui::Checkbox("##lock", &isLocked)) {
                                lockedSides[i] = isLocked;
                                if (isLocked) {
                                    lockedLengths[i] = displayLengths[i];
                                }
                            }

                            if (ImGui::IsItemHovered()) {
                                ImGui::SetTooltip(u8"Зафиксировать длину стороны");
                            }

                            ImGui::SameLine();
                        }

                        ImGui::Text("%s", selectedFigure->getSideName(static_cast<int>(i)));

                        if (hasLengths && i < displayLengths.size()) {
                            ImGui::SetNextItemWidth(-1.f);
                            if (ImGui::InputFloat("Длина", &displayLengths[i], 1.f, 10.f,
                                "%.1f",
                                ImGuiInputTextFlags_EnterReturnsTrue)) {
                                if (displayLengths[i] < 1.f)
                                    displayLengths[i] = 1.f;

                                if (lockedSides[i]) {
                                    lockedLengths[i] = displayLengths[i];
                                }

                                selectedFigure->highlightEdge(i);
                                anyLengthChanged = true;
                            }
                        }

                        float width = selectedFigure->edges[i].width;
                        if (ImGui::DragFloat("Толщина", &width, 0.5f, 0.f, 100.f)) {
                            selectedFigure->edges[i].width = width;
                            selectedFigure->highlightEdge(i);
                        }


                        float eCol[4] = {
                            selectedFigure->edges[i].color.r / 255.f,
                            selectedFigure->edges[i].color.g / 255.f,
                            selectedFigure->edges[i].color.b / 255.f,
                            selectedFigure->edges[i].color.a / 255.f
                        };

                        if (ImGui::ColorEdit4("Цвет", eCol)) {
                            selectedFigure->edges[i].color.r =
                                static_cast<sf::Uint8>(eCol[0] * 255.f);
                            selectedFigure->edges[i].color.g =
                                static_cast<sf::Uint8>(eCol[1] * 255.f);
                            selectedFigure->edges[i].color.b =
                                static_cast<sf::Uint8>(eCol[2] * 255.f);
                            selectedFigure->edges[i].color.a =
                                static_cast<sf::Uint8>(eCol[3] * 255.f);

                            selectedFigure->highlightEdge(i);
                        }



                        ImGui::PopID();
                        ImGui::Spacing();
                    }

                    if (anyLengthChanged) {
                        selectedFigure->applyScale();

                        bool anyLockedNow = false;
                        for (bool l : selectedFigure->lockedSides) {
                            if (l) {
                                anyLockedNow = true;
                            }
                        }

                        if (anyLockedNow) {
                            selectedFigure->applyGenericSideLengths(displayLengths);
                        }
                        else {
                            selectedFigure->setSideLengths(displayLengths);
                        }
                    }
                }

                ImGui::TreePop();
            }

            ImGui::Spacing();
        }

        ImGui::Separator();
        ImGui::TextUnformatted(u8"Габариты");

        sf::FloatRect bounds = selectedFigure->getBoundingBox();
        ImGui::TextDisabled("TL: %.1f, %.1f", bounds.left, bounds.top);
        ImGui::TextDisabled("BR: %.1f, %.1f", bounds.left + bounds.width, bounds.top + bounds.height);

        ImGui::End();
        return fitRequested;
    }

} // namespace ui
