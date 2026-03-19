#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include "CreateFigureModal.hpp"
#include "core/Figures.hpp"
#include "core/PolylineShape.hpp"

#include <imgui.h>
#include <memory>
#include <vector>

namespace ui {

    static const char* kFigureNames[] = {
        u8"Прямоугольник (4)",
        u8"Треугольник (3)",
        u8"Шестиугольник (6)",
        u8"Ромб (4)",
        u8"Трапеция (4)",
        u8"Эллипс"
    };

    static int kEdgeCounts[] = { 4, 3, 6, 4, 4, 1 };

    void CreateFigureModal::open(sf::Vector2f pos) {
        m_open = true;
        m_createPos = pos;
        resetDefaults();
    }

    void CreateFigureModal::resetDefaults() {
        m_figureType = 0;
        m_radiusX = 50.f;
        m_radiusY = 50.f;
        m_fillColor[0] = 0.6f;
        m_fillColor[1] = 0.6f;
        m_fillColor[2] = 0.6f;
        m_fillColor[3] = 1.0f;
        reinitEdges();
    }

    void CreateFigureModal::reinitEdges() {
        int n = kEdgeCounts[m_figureType];
        m_edges.resize(n);

        for (auto& e : m_edges) {
            e.length = 100.f;
            e.width = 2.0f;
            e.color[0] = 0.f;
            e.color[1] = 0.f;
            e.color[2] = 0.f;
            e.color[3] = 1.f;
        }
    }

    std::unique_ptr<core::Figure> CreateFigureModal::createConfiguredFigure() {
        std::unique_ptr<core::Figure> fig;
        std::vector<float> lengths;

        for (auto& e : m_edges) {
            lengths.push_back(e.length);
        }

        switch (m_figureType) {
        case 0:
            fig = std::make_unique<core::Rectangle>(lengths[0], lengths[3]);
            break;
        case 1:
            fig = std::make_unique<core::PolylineShape>(core::PolylineShape::makeTriangle(lengths[0], lengths[1], lengths[2]));
            break;
        case 2: {
            float avg = 0.f;
            for (auto l : lengths) avg += l;
            avg /= lengths.size();
            fig = std::make_unique<core::Hexagon>(avg * 2.f, avg * 2.f);
            break;
        }
        case 3: {
            float avg = 0.f;
            for (auto l : lengths) avg += l;
            avg /= lengths.size();
            fig = std::make_unique<core::Rhombus>(avg * 2.f, avg * 2.f);
            break;
        }
        case 4:
            fig = std::make_unique<core::PolylineShape>(core::PolylineShape::makeTrapezoid(lengths[0], lengths[2], 100.f));
            break;
        case 5:
            fig = std::make_unique<core::Circle>(m_radiusX, m_radiusY);
            break;
        }

        if (!fig) {
            return nullptr;
        }

        if (fig->hasSideLengths()) {
            fig->setSideLengths(lengths);
        }

        fig->fillColor = sf::Color(
            static_cast<sf::Uint8>(m_fillColor[0] * 255),
            static_cast<sf::Uint8>(m_fillColor[1] * 255),
            static_cast<sf::Uint8>(m_fillColor[2] * 255),
            static_cast<sf::Uint8>(m_fillColor[3] * 255)
        );

        for (size_t i = 0; i < m_edges.size() && i < fig->edges.size(); ++i) {
            fig->edges[i].width = m_edges[i].width;
            fig->edges[i].color = sf::Color(
                static_cast<sf::Uint8>(m_edges[i].color[0] * 255),
                static_cast<sf::Uint8>(m_edges[i].color[1] * 255),
                static_cast<sf::Uint8>(m_edges[i].color[2] * 255),
                static_cast<sf::Uint8>(m_edges[i].color[3] * 255)
            );
        }

        return fig;
    }

    static const char* getSideName(int figureType, int idx) {
        static const char* rect4[] = { u8"Верх", u8"Право", u8"Низ", u8"Лево" };
        static const char* trap4[] = { u8"Верх", u8"Правая боковая", u8"Низ", u8"Левая боковая" };
        static const char* tri3[] = { u8"Низ", u8"Правая сторона", u8"Левая сторона" };
        static const char* hex6[] = { u8"Сторона 1", u8"Сторона 2", u8"Сторона 3",
                                      u8"Сторона 4", u8"Сторона 5", u8"Сторона 6" };
        static const char* rho4[] = { u8"Верх-П", u8"Низ-П", u8"Низ-Л", u8"Верх-Л" };

        switch (figureType) {
        case 0: return idx < 4 ? rect4[idx] : "?";
        case 1: return idx < 3 ? tri3[idx] : "?";
        case 2: return idx < 6 ? hex6[idx] : "?";
        case 3: return idx < 4 ? rho4[idx] : "?";
        case 4: return idx < 4 ? trap4[idx] : "?";
        default: return u8"Контур";
        }
    }

    void CreateFigureModal::render(core::Scene& scene) {
        if (m_open) {
            ImGui::OpenPopup(u8"Создать фигуру##Modal");
        }

        ImGui::SetNextWindowPos(
            ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f),
            ImGuiCond_Appearing,
            ImVec2(0.5f, 0.5f)
        );
        ImGui::SetNextWindowSizeConstraints(ImVec2(380, 300), ImVec2(500, 700));

        if (ImGui::BeginPopupModal(u8"Создать фигуру##Modal", &m_open, ImGuiWindowFlags_AlwaysAutoResize)) {
            int previousType = m_figureType;
            if (ImGui::Combo(u8"Тип фигуры", &m_figureType, kFigureNames, IM_ARRAYSIZE(kFigureNames))) {
                if (m_figureType != previousType) {
                    reinitEdges();
                }
            }

            ImGui::Separator();
            ImGui::TextUnformatted(u8"Внешний вид");
            ImGui::ColorEdit4(u8"Цвет заливки", m_fillColor);

            ImGui::Separator();

            if (m_figureType == 5) {
                ImGui::TextUnformatted(u8"Геометрия");
                ImGui::DragFloat(u8"Радиус X", &m_radiusX, 1.f, 5.f, 2000.f);
                ImGui::DragFloat(u8"Радиус Y", &m_radiusY, 1.f, 5.f, 2000.f);

                ImGui::Separator();
                ImGui::TextUnformatted(u8"Контур");

                ImGui::PushID(0);
                ImGui::DragFloat(u8"Толщина", &m_edges[0].width, 0.5f, 0.f, 100.f);
                ImGui::ColorEdit4(u8"Цвет##e", m_edges[0].color, ImGuiColorEditFlags_NoInputs);
                ImGui::PopID();
            }
            else {
                ImGui::TextUnformatted(u8"Длины сторон, толщина и цвет");
                ImGui::BeginChild("SidesScroll", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

                ImGui::Columns(4, "SidesTable", true);
                ImGui::SetColumnWidth(0, 120.f);
                ImGui::SetColumnWidth(1, 80.f);
                ImGui::SetColumnWidth(2, 80.f);
                ImGui::SetColumnWidth(3, 60.f);

                ImGui::TextUnformatted(u8"Сторона");
                ImGui::NextColumn();
                ImGui::TextUnformatted(u8"Длина");
                ImGui::NextColumn();
                ImGui::TextUnformatted(u8"Толщина");
                ImGui::NextColumn();
                ImGui::TextUnformatted(u8"Цвет");
                ImGui::NextColumn();
                ImGui::Separator();

                for (int i = 0; i < static_cast<int>(m_edges.size()); ++i) {
                    ImGui::PushID(i);

                    ImGui::TextUnformatted(getSideName(m_figureType, i));
                    ImGui::NextColumn();

                    ImGui::SetNextItemWidth(-1.f);
                    ImGui::DragFloat("##len", &m_edges[i].length, 1.f, 10.f, 2000.f, "%.0f");
                    ImGui::NextColumn();

                    ImGui::SetNextItemWidth(-1.f);
                    ImGui::DragFloat("##thk", &m_edges[i].width, 0.5f, 0.f, 100.f, "%.1f");
                    ImGui::NextColumn();

                    ImGui::ColorEdit4("##col", m_edges[i].color,
                        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
                    ImGui::NextColumn();

                    ImGui::PopID();
                }

                ImGui::Columns(1);
                ImGui::EndChild();
            }

            ImGui::Separator();

            if (ImGui::Button(u8"Создать", ImVec2(120, 0))) {
                auto fig = createConfiguredFigure();
                if (fig) {
                    if (scene.customOriginActive) {
                        fig->parentOrigin = scene.customOriginPos;
                        fig->anchor = m_createPos - scene.customOriginPos;
                    }
                    else {
                        fig->parentOrigin = sf::Vector2f(0.f, 0.f);
                        fig->anchor = m_createPos;
                    }

                    scene.setSelectedFigure(fig.get());
                    scene.addFigure(std::move(fig));
                }

                m_open = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();

            if (ImGui::Button(u8"Отмена", ImVec2(120, 0))) {
                m_open = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

} // namespace ui
