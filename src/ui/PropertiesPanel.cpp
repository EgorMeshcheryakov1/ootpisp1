#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include "PropertiesPanel.hpp"
#include "core/Figures.hpp"
#include "core/PolylineShape.hpp"
#include "core/CompositeFigure.hpp"

#include <algorithm>
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

    // ── Viewport controls ────────────────────────────────────────────────────
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

            // ── Add all selected to an existing composite ───────────────────
            // Collect composites from the scene
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
                        auto selCopy = scene.getSelection();
                        for (core::Figure* f : selCopy) {
                            if (dynamic_cast<core::CompositeFigure*>(f) == composites[ci]) continue;
                            // find unique_ptr in scene
                            std::unique_ptr<core::Figure> owned;
                            for (auto& up : const_cast<std::vector<std::unique_ptr<core::Figure>>&>(scene.getFigures())) {
                                if (up.get() == f) { owned = std::move(up); break; }
                            }
                            scene.removeFigure(f);
                            if (owned) composites[ci]->addChild(std::move(owned));
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

    // ── Composite figure editor ────────────────────────────────────────────────────
    auto* composite = dynamic_cast<core::CompositeFigure*>(selectedFigure);
    if (composite) {
        if (ImGui::TreeNodeEx(u8"\u0421\u043e\u0441\u0442\u0430\u0432\u043d\u0430\u044f \u0444\u0438\u0433\u0443\u0440\u0430", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& children = composite->getChildren();
            ImGui::Text(u8"\u0414\u0435\u0442\u0435\u0439: %zu", children.size());
            ImGui::Spacing();

            // List children with remove buttons
            int removeIdx = -1;
            for (size_t i = 0; i < children.size(); ++i) {
                ImGui::PushID((int)i);
                // Show type name
                const char* typeName = u8"\u0424\u0438\u0433\u0443\u0440\u0430";
                if (dynamic_cast<core::PolylineShape*>(children[i].get()))
                    typeName = u8"\u041f\u043e\u043b\u0438\u043b\u0438\u043d\u0438\u044f";
                else if (dynamic_cast<core::Rectangle*>(children[i].get()))
                    typeName = u8"\u041f\u0440\u044f\u043c\u043e\u0443\u0433.";
                else if (dynamic_cast<core::Circle*>(children[i].get()))
                    typeName = u8"\u041a\u0440\u0443\u0433";
                ImGui::Text("%s %zu", typeName, i + 1);
                ImGui::SameLine();
                if (ImGui::SmallButton(u8"\u0423\u0434\u0430\u043b\u0438\u0442\u044c")) removeIdx = (int)i;
                ImGui::PopID();
            }

            if (removeIdx >= 0 && (size_t)removeIdx < children.size()) {
                auto extracted = composite->removeChild(children[removeIdx].get());
                if (extracted) {
                    // Place extracted figure back at same world position
                    scene.addFigure(std::move(extracted));
                }
            }

            // ── Add a scene figure into this composite ──────────────────────
            ImGui::Separator();
            ImGui::TextUnformatted(u8"\u0414\u043e\u0431\u0430\u0432\u0438\u0442\u044c \u0444\u0438\u0433\u0443\u0440\u0443:");
            const auto& allFigs = scene.getFigures();
            for (const auto& fig : allFigs) {
                if (fig.get() == composite) continue; // skip self
                if (dynamic_cast<core::CompositeFigure*>(fig.get())) continue; // skip other composites
                ImGui::PushID(fig.get());
                // Show a small identifying label (use bounding box position)
                sf::FloatRect b = fig->getBoundingBox();
                char label[64];
                std::snprintf(label, sizeof(label), "(%.0f,%.0f)", b.left + b.width/2.f, b.top + b.height/2.f);
                if (ImGui::SmallButton(label)) {
                    // We need the unique_ptr: find it in scene
                    std::unique_ptr<core::Figure> owned;
                    for (auto& up : const_cast<std::vector<std::unique_ptr<core::Figure>>&>(allFigs)) {
                        if (up.get() == fig.get()) { owned = std::move(up); break; }
                    }
                    scene.removeFigure(fig.get());
                    if (owned) composite->addChild(std::move(owned));
                    ImGui::PopID();
                    break; // iterator invalidated
                }
                ImGui::PopID();
            }

            ImGui::TreePop();
        }
        ImGui::Separator();
        ImGui::Spacing();
    }

    // ── Polyline segment editor ─────────────────────────────────────────────────────
    auto* polyline = dynamic_cast<core::PolylineShape*>(selectedFigure);
    if (polyline) {
        if (ImGui::TreeNodeEx(u8"\u041e\u0442\u0440\u0435\u0437\u043a\u0438 (\u043f\u043e\u043b\u0438\u043b\u0438\u043d\u0438\u044f)", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& segs = polyline->getSegments();
            bool changed = false;

            for (size_t i = 0; i < segs.size(); ++i) {
                ImGui::PushID((int)i);
                ImGui::Separator();
                ImGui::Text(u8"\u041e\u0442\u0440\u0435\u0437\u043e\u043a %zu", i + 1);

                if (ImGui::InputFloat(u8"\u0414\u043b\u0438\u043d\u0430", &segs[i].length, 1.f, 10.f, "%.1f",
                                      ImGuiInputTextFlags_EnterReturnsTrue)) {
                    segs[i].length = std::max(1.f, segs[i].length);
                    changed = true;
                }

                if (i == 0) {
                    if (ImGui::InputFloat(u8"\u041d\u0430\u043f\u0440\u0430\u0432\u043b\u0435\u043d\u0438\u0435 (\u0430\u0431\u0441)",
                                         &segs[i].angle, 1.f, 10.f, "%.1f\xc2\xb0",
                                         ImGuiInputTextFlags_EnterReturnsTrue))
                        changed = true;
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip(u8"\u0410\u0431\u0441\u043e\u043b\u044e\u0442\u043d\u044b\u0439 \u0443\u0433\u043e\u043b \u043f\u0435\u0440\u0432\u043e\u0433\u043e \u043e\u0442\u0440\u0435\u0437\u043a\u0430 (\u0433\u0440\u0430\u0434. \u043e\u0442 +X)");
                } else {
                    if (ImGui::InputFloat(u8"\u0423\u0433\u043e\u043b \u043f\u043e\u0432\u043e\u0440\u043e\u0442\u0430",
                                         &segs[i].angle, 1.f, 10.f, "%.1f\xc2\xb0",
                                         ImGuiInputTextFlags_EnterReturnsTrue))
                        changed = true;
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip(u8"\u0423\u0433\u043e\u043b \u043f\u043e\u0432\u043e\u0440\u043e\u0442\u0430 \u043e\u0442 \u043f\u0440\u0435\u0434\u044b\u0434\u0443\u0449\u0435\u0433\u043e \u043e\u0442\u0440\u0435\u0437\u043a\u0430");
                }

                ImGui::PopID();
            }

            if (changed) polyline->rebuild();

            ImGui::TreePop();
        }
        ImGui::Separator();
        ImGui::Spacing();
    }

    // ── Anchor ─────────────────────────────────────────────────────────────────────
    ImGui::TextUnformatted(u8"\u0422\u043e\u0447\u043a\u0430 \u043f\u0440\u0438\u0432\u044f\u0437\u043a\u0438");
    ImGui::SameLine();
    ImGui::Checkbox(u8"\u0417\u0430\u043a\u0440\u0435\u043f\u0438\u0442\u044c \u043a \u0444\u0438\u0433\u0443\u0440\u0435", &m_lockAnchor);
    float anchor[2] = { selectedFigure->anchor.x, selectedFigure->anchor.y };
    if (ImGui::DragFloat2("##Anchor", anchor, 1.0f)) {
        if (m_lockAnchor)
            selectedFigure->anchor = sf::Vector2f(anchor[0], anchor[1]);
        else
            selectedFigure->setAnchorKeepAbsolute(sf::Vector2f(anchor[0], anchor[1]));
    }
    if (ImGui::Button(u8"\u0421\u0431\u0440\u043e\u0441\u0438\u0442\u044c \u043f\u0440\u0438\u0432\u044f\u0437\u043a\u0443"))
        selectedFigure->resetAnchor();
    ImGui::Spacing();

    // ── Vertex editing (non-composite, non-uniform) ───────────────────────────────
    if (!composite) {
        auto& vertices = selectedFigure->getVerticesMutable();
        if (!vertices.empty() && !selectedFigure->hasUniformEdge()) {
            ImGui::Separator();
            ImGui::TextUnformatted(u8"\u041f\u0435\u0440\u0435\u043c\u0435\u0449\u0435\u043d\u0438\u0435 \u043f\u043e \u043a\u043e\u043e\u0440\u0434\u0438\u043d\u0430\u0442\u0430\u043c \u0432\u0435\u0440\u0448\u0438\u043d\u044b");
            for (size_t i = 0; i < vertices.size(); ++i) {
                ImGui::PushID((int)(i + 100));
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
    }

    // ── Fill colour ────────────────────────────────────────────────────────────────────
    ImGui::Separator();
    ImGui::TextUnformatted(u8"\u0426\u0432\u0435\u0442 \u0437\u0430\u043b\u0438\u0432\u043a\u0438");
    float fill[4] = {
        selectedFigure->fillColor.r / 255.f,
        selectedFigure->fillColor.g / 255.f,
        selectedFigure->fillColor.b / 255.f,
        selectedFigure->fillColor.a / 255.f
    };
    if (ImGui::ColorEdit4("##FillColor", fill)) {
        selectedFigure->fillColor.r = (sf::Uint8)(fill[0]*255.f);
        selectedFigure->fillColor.g = (sf::Uint8)(fill[1]*255.f);
        selectedFigure->fillColor.b = (sf::Uint8)(fill[2]*255.f);
        selectedFigure->fillColor.a = (sf::Uint8)(fill[3]*255.f);
    }
    ImGui::Spacing();

    // ── Rotation ───────────────────────────────────────────────────────────────────────
    ImGui::Separator();
    ImGui::TextUnformatted(u8"\u041f\u043e\u0432\u043e\u0440\u043e\u0442");
    float rotation = selectedFigure->rotationAngle;
    if (ImGui::DragFloat(u8"\u0423\u0433\u043e\u043b", &rotation, 1.0f, -360.0f, 360.0f, "%.1f"))
        selectedFigure->rotationAngle = rotation;
    if (ImGui::Button(u8"\u0421\u0431\u0440\u043e\u0441\u0438\u0442\u044c \u043f\u043e\u0432\u043e\u0440\u043e\u0442"))
        selectedFigure->rotationAngle = 0.f;
    ImGui::Spacing();

    // ── Scale ────────────────────────────────────────────────────────────────────────
    ImGui::Separator();
    ImGui::TextUnformatted(u8"\u041c\u0430\u0441\u0448\u0442\u0430\u0431");
    ImGui::Checkbox(u8"\u0421\u043e\u0445\u0440\u0430\u043d\u044f\u0442\u044c \u043f\u0440\u043e\u043f\u043e\u0440\u0446\u0438\u0438", &m_lockProportions);
    float scalePct[2] = { selectedFigure->scale.x*100.f, selectedFigure->scale.y*100.f };
    if (ImGui::DragFloat2(u8"\u041c\u0430\u0441\u0448\u0442\u0430\u0431 %##Scale", scalePct, 1.0f, 1.0f, 10000.f, "%.1f%%")) {
        if (m_lockProportions) {
            if (scalePct[0] != selectedFigure->scale.x*100.f) scalePct[1] = scalePct[0];
            else if (scalePct[1] != selectedFigure->scale.y*100.f) scalePct[0] = scalePct[1];
        }
        selectedFigure->scale.x = scalePct[0]/100.f;
        selectedFigure->scale.y = scalePct[1]/100.f;
    }
    if (ImGui::Button(u8"\u0421\u0431\u0440\u043e\u0441\u0438\u0442\u044c \u043c\u0430\u0441\u0448\u0442\u0430\u0431"))
        selectedFigure->scale = sf::Vector2f(1.f, 1.f);
    ImGui::SameLine();
    if (ImGui::Button(u8"\u041f\u0440\u0438\u043c\u0435\u043d\u0438\u0442\u044c \u043c\u0430\u0441\u0448\u0442\u0430\u0431"))
        selectedFigure->applyScale();
    ImGui::Spacing();

    // ── Edges & side lengths (non-composite) ──────────────────────────────────────
    bool hasLengths = selectedFigure->hasSideLengths();
    if (!composite && (!selectedFigure->edges.empty() || hasLengths)) {
        ImGui::Separator();
        if (ImGui::TreeNodeEx(u8"\u0413\u0440\u0430\u043d\u0438 \u0438 \u0441\u0442\u043e\u0440\u043e\u043d\u044b", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (selectedFigure->hasUniformEdge()) {
                ImGui::PushID("UniformEdge");
                float width = selectedFigure->edges.empty() ? 1.f : selectedFigure->edges[0].width;
                if (ImGui::DragFloat(u8"\u0422\u043e\u043b\u0449\u0438\u043d\u0430", &width, 0.5f, 0.f, 100.f))
                    for (auto& e : selectedFigure->edges) e.width = width;
                if (!selectedFigure->edges.empty()) {
                    float eCol[4] = {
                        selectedFigure->edges[0].color.r/255.f, selectedFigure->edges[0].color.g/255.f,
                        selectedFigure->edges[0].color.b/255.f, selectedFigure->edges[0].color.a/255.f
                    };
                    if (ImGui::ColorEdit4(u8"\u0426\u0432\u0435\u0442", eCol))
                        for (auto& e : selectedFigure->edges) {
                            e.color.r=(sf::Uint8)(eCol[0]*255.f); e.color.g=(sf::Uint8)(eCol[1]*255.f);
                            e.color.b=(sf::Uint8)(eCol[2]*255.f); e.color.a=(sf::Uint8)(eCol[3]*255.f);
                        }
                }
                ImGui::PopID();
                ImGui::Spacing();
            } else {
                auto actualLengths = hasLengths ? selectedFigure->getSideLengths() : std::vector<float>();
                auto& lockedSides   = selectedFigure->lockedSides;
                auto& lockedLengths = selectedFigure->lockedLengths;

                if (lockedSides.size() != selectedFigure->edges.size()) {
                    lockedSides.resize(selectedFigure->edges.size(), false);
                    lockedLengths.resize(selectedFigure->edges.size(), 1.0f);
                    for (size_t i = 0; i < actualLengths.size() && i < lockedLengths.size(); ++i)
                        lockedLengths[i] = actualLengths[i];
                }

                std::vector<float> displayLengths = actualLengths;
                for (size_t i = 0; i < displayLengths.size(); ++i)
                    if (lockedSides[i]) displayLengths[i] = lockedLengths[i];

                bool anyLengthChanged = false;

                for (size_t i = 0; i < selectedFigure->edges.size(); ++i) {
                    ImGui::PushID((int)i);

                    if (hasLengths && i < displayLengths.size()) {
                        bool isLocked = lockedSides[i];
                        if (ImGui::Checkbox("##lock", &isLocked)) {
                            lockedSides[i] = isLocked;
                            if (isLocked) lockedLengths[i] = displayLengths[i];
                        }
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip(u8"\u0417\u0430\u0444\u0438\u043a\u0441\u0438\u0440\u043e\u0432\u0430\u0442\u044c \u0434\u043b\u0438\u043d\u0443 \u0441\u0442\u043e\u0440\u043e\u043d\u044b");
                        ImGui::SameLine();
                    }

                    ImGui::Text("%s", selectedFigure->getSideName((int)i));

                    if (hasLengths && i < displayLengths.size()) {
                        ImGui::SetNextItemWidth(-1.f);
                        if (ImGui::InputFloat(u8"\u0414\u043b\u0438\u043d\u0430", &displayLengths[i], 1.f, 10.f, "%.1f",
                                              ImGuiInputTextFlags_EnterReturnsTrue)) {
                            if (displayLengths[i] < 1.f) displayLengths[i] = 1.f;
                            if (lockedSides[i]) lockedLengths[i] = displayLengths[i];
                            selectedFigure->highlightEdge(i);
                            anyLengthChanged = true;
                        }
                    }

                    float width = selectedFigure->edges[i].width;
                    if (ImGui::DragFloat(u8"\u0422\u043e\u043b\u0449\u0438\u043d\u0430", &width, 0.5f, 0.f, 100.f)) {
                        selectedFigure->edges[i].width = width;
                        selectedFigure->highlightEdge(i);
                    }

                    float eCol[4] = {
                        selectedFigure->edges[i].color.r/255.f, selectedFigure->edges[i].color.g/255.f,
                        selectedFigure->edges[i].color.b/255.f, selectedFigure->edges[i].color.a/255.f
                    };
                    if (ImGui::ColorEdit4(u8"\u0426\u0432\u0435\u0442", eCol)) {
                        selectedFigure->edges[i].color.r=(sf::Uint8)(eCol[0]*255.f);
                        selectedFigure->edges[i].color.g=(sf::Uint8)(eCol[1]*255.f);
                        selectedFigure->edges[i].color.b=(sf::Uint8)(eCol[2]*255.f);
                        selectedFigure->edges[i].color.a=(sf::Uint8)(eCol[3]*255.f);
                        selectedFigure->highlightEdge(i);
                    }

                    ImGui::PopID();
                    ImGui::Spacing();
                }

                if (anyLengthChanged) {
                    selectedFigure->applyScale();
                    bool anyLocked = false;
                    for (bool l : selectedFigure->lockedSides) if (l) { anyLocked = true; break; }
                    if (anyLocked) selectedFigure->applyGenericSideLengths(displayLengths);
                    else           selectedFigure->setSideLengths(displayLengths);
                }
            }
            ImGui::TreePop();
        }
        ImGui::Spacing();
    }

    // ── Add this figure to an existing composite ──────────────────────────────────
    if (!composite) {
        std::vector<core::CompositeFigure*> composites;
        for (const auto& fig : scene.getFigures()) {
            auto* c = dynamic_cast<core::CompositeFigure*>(fig.get());
            if (c) composites.push_back(c);
        }
        if (!composites.empty()) {
            ImGui::Separator();
            if (ImGui::TreeNodeEx(u8"\u0414\u043e\u0431\u0430\u0432\u0438\u0442\u044c \u0432 \u0441\u043e\u0441\u0442\u0430\u0432\u043d\u0443\u044e", ImGuiTreeNodeFlags_None)) {
                for (size_t ci = 0; ci < composites.size(); ++ci) {
                    ImGui::PushID((int)ci);
                    char label[64];
                    std::snprintf(label, sizeof(label), u8"\u0421\u043e\u0441\u0442\u0430\u0432\u043d\u0430\u044f %zu", ci + 1);
                    if (ImGui::Button(label)) {
                        core::CompositeFigure* targetComp = composites[ci];
                        core::Figure* rawFig = selectedFigure;
                        std::unique_ptr<core::Figure> owned;
                        for (auto& up : const_cast<std::vector<std::unique_ptr<core::Figure>>&>(scene.getFigures())) {
                            if (up.get() == rawFig) { owned = std::move(up); break; }
                        }
                        scene.removeFigure(rawFig);
                        if (owned) targetComp->addChild(std::move(owned));
                        ImGui::PopID();
                        ImGui::TreePop();
                        ImGui::End();
                        return fitRequested;
                    }
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
            ImGui::Spacing();
        }
    }

    // ── Bounding box ───────────────────────────────────────────────────────────────────
    ImGui::Separator();
    ImGui::TextUnformatted(u8"\u0413\u0430\u0431\u0430\u0440\u0438\u0442\u044b");
    sf::FloatRect bounds = selectedFigure->getBoundingBox();
    ImGui::TextDisabled("TL: %.1f, %.1f", bounds.left, bounds.top);
    ImGui::TextDisabled("BR: %.1f, %.1f", bounds.left+bounds.width, bounds.top+bounds.height);

    ImGui::End();
    return fitRequested;
}

} // namespace ui
