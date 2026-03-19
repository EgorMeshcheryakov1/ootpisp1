#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include "core/CompositeFigure.hpp"
#include "core/Figures.hpp"
#include "core/MathUtils.hpp"
#include "core/PolylineShape.hpp"
#include "core/Scene.hpp"
#include "core/Viewport.hpp"
#include "ui/CreateFigureModal.hpp"
#include "ui/PropertiesPanel.hpp"
#include "ui/Toolbar.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <imgui-SFML.h>
#include <imgui.h>
#include <iostream>
#include <memory>
#include <vector>

using namespace core;

static void applyLightTheme() {
    ImGui::StyleColorsLight();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.95f, 0.95f, 0.97f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.98f, 0.98f, 0.99f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.98f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.90f, 0.91f, 0.93f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.82f, 0.86f, 0.95f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.75f, 0.82f, 0.96f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.86f, 0.89f, 0.95f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.76f, 0.82f, 0.96f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.66f, 0.75f, 0.95f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.80f, 0.85f, 0.95f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.72f, 0.79f, 0.95f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.64f, 0.73f, 0.94f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.75f, 0.78f, 0.84f, 1.00f);
    colors[ImGuiCol_Text] = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
}

static bool loadRussianFont(float fontSize = 18.0f) {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    const ImWchar* ranges = io.Fonts->GetGlyphRangesCyrillic();
    ImFont* font = nullptr;
    font = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeui.ttf", fontSize, nullptr, ranges);
    if (!font) font = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/arial.ttf", fontSize, nullptr, ranges);
    if (!font) font = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/tahoma.ttf", fontSize, nullptr, ranges);
    if (!font) {
        std::cerr << "Failed to load Cyrillic font" << std::endl;
        return false;
    }
    io.FontDefault = font;
    ImGui::SFML::UpdateFontTexture();
    return true;
}

static void recreateWindow(sf::RenderWindow& window, bool fullscreen) {
    ImGui::SFML::Shutdown();
    if (fullscreen)
        window.create(sf::VideoMode::getDesktopMode(), "GraphEditor", sf::Style::Fullscreen);
    else
        window.create(sf::VideoMode(1280, 720), "GraphEditor", sf::Style::Default);
    window.setFramerateLimit(60);
    if (!ImGui::SFML::Init(window))
        std::cerr << "Failed to reinitialize ImGui-SFML" << std::endl;
    applyLightTheme();
    loadRussianFont(18.0f);
}

static void drawAnchorMarker(sf::RenderTarget& target, sf::Vector2f pos, float markerScale) {
    float r = 5.f * markerScale;
    sf::CircleShape circle(r);
    circle.setFillColor(sf::Color::White);
    circle.setOutlineColor(sf::Color(0, 120, 215));
    circle.setOutlineThickness(1.5f * markerScale);
    circle.setOrigin(r, r);
    circle.setPosition(pos);

    sf::VertexArray lines(sf::Lines, 4);
    sf::Color lineColor(0, 120, 215);
    lines[0] = sf::Vertex(pos - sf::Vector2f(8.f * markerScale, 0.f), lineColor);
    lines[1] = sf::Vertex(pos + sf::Vector2f(8.f * markerScale, 0.f), lineColor);
    lines[2] = sf::Vertex(pos - sf::Vector2f(0.f, 8.f * markerScale), lineColor);
    lines[3] = sf::Vertex(pos + sf::Vector2f(0.f, 8.f * markerScale), lineColor);
    target.draw(circle);
    target.draw(lines);
}

struct PolylineCreationState {
    bool active = false;
    std::vector<sf::Vector2f> points;
};

int main() {
    sf::RenderWindow window(sf::VideoMode(1280, 720), "GraphEditor");
    window.setFramerateLimit(60);

    if (!ImGui::SFML::Init(window)) {
        std::cerr << "Failed to initialize ImGui-SFML" << std::endl;
        return -1;
    }

    applyLightTheme();
    loadRussianFont(18.0f);

    bool isFullscreen = false;
    sf::Clock deltaClock;

    core::Scene scene;

    ui::Toolbar toolbar;
    ui::PropertiesPanel propertiesPanel;
    ui::CreateFigureModal createModal;
    ui::Tool currentTool = ui::Tool::Select;

    bool isDragging = false;
    bool isDraggingAnchor = false;
    bool isNodeEditMode = false;
    bool isRotating = false;
    float rotationStartAngle = 0.f;
    float initialRotation = 0.f;

    enum class ScaleHandle { None, TL, TC, TR, CR, BR, BC, BL, CL };
    ScaleHandle hoveringScaleHandle = ScaleHandle::None;
    ScaleHandle draggingScaleHandle = ScaleHandle::None;
    sf::Vector2f scaleStartMouse;
    sf::Vector2f scaleStartValue;
    sf::Vector2f scaleStartAnchor;

    int draggingVertexIndex = -1;
    bool isCreating = false;
    int creatingStep = 0;
    sf::Vector2f dragOffset;

    core::Viewport viewport;
    bool isPanning = false;
    sf::Vector2f panStartOrigin;
    sf::Vector2f panStartMouse;

    bool isDraggingCustomOrigin = false;

    sf::Clock clickClock;
    bool wasClicked = false;
    sf::Vector2f createStartPos;

    bool showGrid = false;
    bool showOriginAxes = true;

    PolylineCreationState polylineState;

    sf::Cursor cursorArrow; cursorArrow.loadFromSystem(sf::Cursor::Arrow);
    sf::Cursor cursorHand; cursorHand.loadFromSystem(sf::Cursor::Hand);
    sf::Cursor cursorCross; cursorCross.loadFromSystem(sf::Cursor::Cross);
    sf::Cursor cursorSizeAll; cursorSizeAll.loadFromSystem(sf::Cursor::SizeAll);
    sf::Cursor cursorSizeNWSE; cursorSizeNWSE.loadFromSystem(sf::Cursor::SizeTopLeftBottomRight);
    sf::Cursor cursorSizeNESW; cursorSizeNESW.loadFromSystem(sf::Cursor::SizeBottomLeftTopRight);
    sf::Cursor cursorSizeWE; cursorSizeWE.loadFromSystem(sf::Cursor::SizeHorizontal);
    sf::Cursor cursorSizeNS; cursorSizeNS.loadFromSystem(sf::Cursor::SizeVertical);

    auto createFigure = [&](ui::Tool tool, float width, float height) -> std::unique_ptr<core::Figure> {
        width = std::max(width, 50.f);
        height = std::max(height, 50.f);

        std::unique_ptr<core::Figure> fig;
        if (tool == ui::Tool::Rectangle)
            fig = std::make_unique<core::Rectangle>(width, height);
        else if (tool == ui::Tool::Triangle)
            fig = std::make_unique<core::PolylineShape>(core::PolylineShape::makeTriangle(width, height, width));
        else if (tool == ui::Tool::Hexagon)
            fig = std::make_unique<core::Hexagon>(width, height);
        else if (tool == ui::Tool::Rhombus)
            fig = std::make_unique<core::Rhombus>(width, height);
        else if (tool == ui::Tool::Trapezoid)
            fig = std::make_unique<core::PolylineShape>(core::PolylineShape::makeTrapezoid(width * 0.6f, width, height));
        else if (tool == ui::Tool::Circle)
            fig = std::make_unique<core::Circle>(width / 2.f, height / 2.f);

        if (fig) {
            fig->fillColor = sf::Color(150, 150, 150);
            for (auto& edge : fig->edges) {
                edge.width = 2.f;
                edge.color = sf::Color::Black;
            }
        }
        return fig;
    };

    {
        std::vector<sf::Color> colors = {
            sf::Color(255, 100, 100), sf::Color(100, 255, 100), sf::Color(100, 100, 255),
            sf::Color(255, 255, 100), sf::Color(255, 100, 255), sf::Color(100, 255, 255)};
        std::vector<ui::Tool> tempTools = {
            ui::Tool::Rectangle, ui::Tool::Triangle, ui::Tool::Hexagon,
            ui::Tool::Rhombus, ui::Tool::Trapezoid, ui::Tool::Circle};
        for (size_t i = 0; i < tempTools.size(); ++i) {
            auto fig = createFigure(tempTools[i], 150.f, 150.f);
            if (!fig) continue;
            fig->fillColor = colors[i];
            for (size_t j = 0; j < fig->edges.size(); ++j) {
                fig->edges[j].color = colors[(i + j + 1) % colors.size()];
                fig->edges[j].width = 4.f;
            }
            fig->anchor = sf::Vector2f(250.f + (i % 3) * 250.f, 250.f + (i / 3) * 250.f);
            scene.addFigure(std::move(fig));
        }
    }

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(window, event);

            if (event.type == sf::Event::Closed)
                window.close();

            ImGuiIO& io = ImGui::GetIO();

            if (event.type == sf::Event::KeyPressed && !io.WantCaptureKeyboard) {
                if (event.key.code == sf::Keyboard::F11) {
                    isFullscreen = !isFullscreen;
                    recreateWindow(window, isFullscreen);
                    deltaClock.restart();
                    continue;
                }

                bool ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) ||
                            sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);

                if (ctrl) {
                    if (event.key.code == sf::Keyboard::Equal || event.key.code == sf::Keyboard::Add) {
                        sf::Vector2f center(window.getSize().x / 2.f, window.getSize().y / 2.f);
                        viewport.zoomAt(center, 1.1f);
                    } else if (event.key.code == sf::Keyboard::Hyphen || event.key.code == sf::Keyboard::Subtract) {
                        sf::Vector2f center(window.getSize().x / 2.f, window.getSize().y / 2.f);
                        viewport.zoomAt(center, 1.f / 1.1f);
                    } else if (event.key.code == sf::Keyboard::Num0 || event.key.code == sf::Keyboard::Numpad0) {
                        sf::Vector2f center(window.getSize().x / 2.f, window.getSize().y / 2.f);
                        viewport.zoomAt(center, 1.f / viewport.zoom);
                    }
                } else if (event.key.code == sf::Keyboard::V)
                    currentTool = ui::Tool::Select;
                else if (event.key.code == sf::Keyboard::R)
                    currentTool = ui::Tool::Rectangle;
                else if (event.key.code == sf::Keyboard::T)
                    currentTool = ui::Tool::Triangle;
                else if (event.key.code == sf::Keyboard::H)
                    currentTool = ui::Tool::Hexagon;
                else if (event.key.code == sf::Keyboard::D)
                    currentTool = ui::Tool::Rhombus;
                else if (event.key.code == sf::Keyboard::Z)
                    currentTool = ui::Tool::Trapezoid;
                else if (event.key.code == sf::Keyboard::C)
                    currentTool = ui::Tool::Circle;
                else if (event.key.code == sf::Keyboard::P)
                    currentTool = ui::Tool::Polyline;
                else if (event.key.code == sf::Keyboard::Escape) {
                    if (polylineState.active) {
                        polylineState.active = false;
                        polylineState.points.clear();
                    } else if (isNodeEditMode) {
                        isNodeEditMode = false;
                    } else {
                        scene.clearSelection();
                        if (isCreating) {
                            isCreating = false;
                            creatingStep = 0;
                        }
                    }
                } else if (event.key.code == sf::Keyboard::N) {
                    if (scene.getSelectedFigure())
                        isNodeEditMode = !isNodeEditMode;
                } else if (event.key.code == sf::Keyboard::G)
                    showGrid = !showGrid;
                else if (event.key.code == sf::Keyboard::A)
                    showOriginAxes = !showOriginAxes;
                else if (event.key.code == sf::Keyboard::Delete || event.key.code == sf::Keyboard::Backspace) {
                    auto sel = scene.getSelection();
                    for (Figure* f : sel) {
                        scene.removeFigure(f);
                    }
                    scene.clearSelection();
                } else if (event.key.code == sf::Keyboard::Up || event.key.code == sf::Keyboard::Down ||
                           event.key.code == sf::Keyboard::Left || event.key.code == sf::Keyboard::Right) {
                    if (core::Figure* fig = scene.getSelectedFigure()) {
                        float moveAmt = (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ||
                                         sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) ? 10.f : 1.f;
                        if (event.key.code == sf::Keyboard::Up) fig->move(sf::Vector2f(0.f, -moveAmt));
                        else if (event.key.code == sf::Keyboard::Down) fig->move(sf::Vector2f(0.f, moveAmt));
                        else if (event.key.code == sf::Keyboard::Left) fig->move(sf::Vector2f(-moveAmt, 0.f));
                        else if (event.key.code == sf::Keyboard::Right) fig->move(sf::Vector2f(moveAmt, 0.f));
                    }
                }
            }

            if (event.type == sf::Event::MouseWheelScrolled && !io.WantCaptureMouse) {
                if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
                    float zoomFactor = std::pow(1.1f, event.mouseWheelScroll.delta);
                    sf::Vector2f mousePosScreen(event.mouseWheelScroll.x, event.mouseWheelScroll.y);
                    viewport.zoomAt(mousePosScreen, zoomFactor);
                }
            }

            if (!io.WantCaptureMouse) {
                if (event.type == sf::Event::MouseButtonPressed) {
                    if (event.mouseButton.button == sf::Mouse::Middle ||
                        (event.mouseButton.button == sf::Mouse::Left && sf::Keyboard::isKeyPressed(sf::Keyboard::Space))) {
                        isPanning = true;
                        panStartMouse = sf::Vector2f(event.mouseButton.x, event.mouseButton.y);
                        panStartOrigin = viewport.worldOrigin;
                    } else if (event.mouseButton.button == sf::Mouse::Right) {
                        sf::Vector2f mousePosScreen(event.mouseButton.x, event.mouseButton.y);
                        sf::Vector2f mousePos = viewport.screenToWorld(mousePosScreen);
                        if (currentTool == ui::Tool::Polyline && polylineState.active && polylineState.points.size() >= 2) {
                            std::vector<core::Segment> segs;
                            const auto& pts = polylineState.points;
                            float runningAbsAngle = 0.f;
                            for (size_t i = 0; i + 1 < pts.size(); ++i) {
                                sf::Vector2f dir = pts[i + 1] - pts[i];
                                float len = std::hypot(dir.x, dir.y);
                                if (len < 1.f)
                                    continue;
                                float absAngle = std::atan2(dir.y, dir.x) * 180.f / math::PI;
                                if (segs.empty()) {
                                    segs.push_back({len, absAngle});
                                } else {
                                    float turn = absAngle - runningAbsAngle;
                                    while (turn > 180.f) turn -= 360.f;
                                    while (turn <= -180.f) turn += 360.f;
                                    segs.push_back({len, turn});
                                }
                                runningAbsAngle = absAngle;
                            }

                            if (!segs.empty()) {
                                auto fig = std::make_unique<core::PolylineShape>(segs, false);
                                fig->fillColor = sf::Color(150, 150, 150, 0);
                                for (auto& e : fig->edges) {
                                    e.width = 2.f;
                                    e.color = sf::Color::Black;
                                }
                                sf::Vector2f cen(0.f, 0.f);
                                for (const auto& p : pts)
                                    cen += p;
                                cen /= static_cast<float>(pts.size());
                                if (scene.customOriginActive) {
                                    fig->parentOrigin = scene.customOriginPos;
                                    fig->anchor = cen - scene.customOriginPos;
                                } else {
                                    fig->anchor = cen;
                                }
                                Figure* raw = fig.get();
                                scene.addFigure(std::move(fig));
                                scene.setSelectedFigure(raw);
                            }

                            polylineState.active = false;
                            polylineState.points.clear();
                            currentTool = ui::Tool::Select;
                        } else if (currentTool != ui::Tool::Polyline) {
                            createModal.open(mousePos);
                        }
                    } else if (event.mouseButton.button == sf::Mouse::Left) {
                        sf::Vector2f mousePosScreen(event.mouseButton.x, event.mouseButton.y);
                        sf::Vector2f mousePos = viewport.screenToWorld(mousePosScreen);

                        if (scene.customOriginActive) {
                            sf::Vector2f customScreen = viewport.worldToScreen(scene.customOriginPos);
                            if (std::hypot(mousePosScreen.x - customScreen.x, mousePosScreen.y - customScreen.y) <= 15.f) {
                                bool doubleClickedOrigin = false;
                                if (wasClicked && clickClock.getElapsedTime().asSeconds() < 0.3f) {
                                    doubleClickedOrigin = true;
                                    wasClicked = false;
                                } else {
                                    wasClicked = true;
                                    clickClock.restart();
                                }
                                if (doubleClickedOrigin)
                                    scene.resetCustomOrigin();
                                else
                                    isDraggingCustomOrigin = true;
                                continue;
                            }
                        }

                        if (currentTool == ui::Tool::Polyline) {
                            polylineState.active = true;
                            polylineState.points.push_back(mousePos);
                            continue;
                        }

                        if (currentTool == ui::Tool::Select) {
                            bool doubleClicked = false;
                            if (wasClicked && clickClock.getElapsedTime().asSeconds() < 0.3f) {
                                doubleClicked = true;
                                wasClicked = false;
                            } else {
                                wasClicked = true;
                                clickClock.restart();
                            }

                            if (doubleClicked && !scene.hitTest(mousePos)) {
                                scene.setCustomOrigin(mousePos);
                                continue;
                            }

                            bool ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) ||
                                        sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);
                            bool altPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) ||
                                              sf::Keyboard::isKeyPressed(sf::Keyboard::RAlt);

                            core::Figure* selFig = scene.getSelectedFigure();
                            bool hitAnchor = false;
                            bool hitRotationMarker = false;
                            int hoveredVertex = -1;

                            if (selFig && !ctrl) {
                                float markerScale = 1.f / viewport.zoom;
                                sf::Vector2f absAnchor = selFig->parentOrigin + selFig->anchor;
                                float dist = std::hypot(mousePos.x - absAnchor.x, mousePos.y - absAnchor.y);
                                if (dist <= 10.f * markerScale)
                                    hitAnchor = true;

                                sf::FloatRect localBounds = selFig->getLocalBoundingBox();
                                sf::Vector2f tl(localBounds.left, localBounds.top);
                                sf::Vector2f tr(localBounds.left + localBounds.width, localBounds.top);
                                sf::Vector2f tc = (tl + tr) / 2.f;
                                sf::Vector2f absTc = selFig->getAbsoluteVertex(tc);
                                float rotRad = selFig->rotationAngle * math::PI / 180.f;
                                sf::Vector2f rotOffset(std::sin(rotRad) * 20.f * markerScale,
                                                       -std::cos(rotRad) * 20.f * markerScale);
                                sf::Vector2f rotMarker = absTc + rotOffset;
                                if (std::hypot(mousePos.x - rotMarker.x, mousePos.y - rotMarker.y) <= 8.f * markerScale)
                                    hitRotationMarker = true;

                                if (isNodeEditMode) {
                                    const auto& verts = selFig->getVertices();
                                    for (size_t i = 0; i < verts.size(); ++i) {
                                        sf::Vector2f absV = selFig->getAbsoluteVertex(verts[i]);
                                        if (std::abs(mousePos.x - absV.x) <= 6.f * markerScale &&
                                            std::abs(mousePos.y - absV.y) <= 6.f * markerScale) {
                                            hoveredVertex = static_cast<int>(i);
                                            break;
                                        }
                                    }
                                }
                            }

                            if (ctrl) {
                                core::Figure* hit = scene.hitTest(mousePos);
                                if (hit) {
                                    if (scene.isSelected(hit))
                                        scene.removeFromSelection(hit);
                                    else
                                        scene.addToSelection(hit);
                                }
                            } else if (hoveringScaleHandle != ScaleHandle::None && selFig) {
                                draggingScaleHandle = hoveringScaleHandle;
                                scaleStartMouse = mousePos;
                                scaleStartValue = selFig->scale;
                                scaleStartAnchor = selFig->anchor;
                            } else if (hitRotationMarker && selFig) {
                                isRotating = true;
                                sf::Vector2f absoluteAnchor = selFig->parentOrigin + selFig->anchor;
                                rotationStartAngle = std::atan2(mousePos.y - absoluteAnchor.y, mousePos.x - absoluteAnchor.x) * 180.f / math::PI;
                                initialRotation = selFig->rotationAngle;
                            } else if (isNodeEditMode && hoveredVertex != -1) {
                                draggingVertexIndex = hoveredVertex;
                            } else if (hitAnchor && altPressed && selFig) {
                                isDraggingAnchor = true;
                                sf::Vector2f absoluteAnchor = selFig->parentOrigin + selFig->anchor;
                                dragOffset = mousePos - absoluteAnchor;
                            } else {
                                core::Figure* hit = scene.hitTest(mousePos);
                                scene.setSelectedFigure(hit);
                                if (selFig != hit)
                                    isNodeEditMode = false;
                                if (hit && doubleClicked) {
                                    isNodeEditMode = true;
                                } else if (hit) {
                                    isDragging = true;
                                    sf::Vector2f absoluteAnchor = hit->parentOrigin + hit->anchor;
                                    dragOffset = mousePos - absoluteAnchor;
                                }
                            }
                        } else {
                            if (creatingStep == 0) {
                                isCreating = true;
                                creatingStep = 1;
                                createStartPos = mousePos;
                                scene.setSelectedFigure(nullptr);
                            } else if (creatingStep == 1) {
                                creatingStep = 2;
                            }
                        }
                    }
                } else if (event.type == sf::Event::MouseButtonReleased) {
                    if (event.mouseButton.button == sf::Mouse::Middle ||
                        (event.mouseButton.button == sf::Mouse::Left && isPanning))
                        isPanning = false;

                    if (event.mouseButton.button == sf::Mouse::Left) {
                        if (isDraggingCustomOrigin) isDraggingCustomOrigin = false;
                        if (draggingScaleHandle != ScaleHandle::None) draggingScaleHandle = ScaleHandle::None;
                        if (isDragging) isDragging = false;
                        if (isDraggingAnchor) isDraggingAnchor = false;
                        if (isRotating) isRotating = false;
                        if (draggingVertexIndex != -1) draggingVertexIndex = -1;

                        if (isCreating && creatingStep == 2) {
                            isCreating = false;
                            creatingStep = 0;
                            sf::Vector2f mousePos = viewport.screenToWorld(sf::Vector2f(event.mouseButton.x, event.mouseButton.y));
                            float dx = mousePos.x - createStartPos.x;
                            float dy = mousePos.y - createStartPos.y;
                            float width = std::abs(dx);
                            float height = std::abs(dy);
                            if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ||
                                sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) {
                                float maxDim = std::max(width, height);
                                width = maxDim;
                                height = maxDim;
                                mousePos.x = createStartPos.x + ((dx >= 0) ? maxDim : -maxDim);
                                mousePos.y = createStartPos.y + ((dy >= 0) ? maxDim : -maxDim);
                            }
                            if (width < 5 && height < 5) {
                                width = 100.f;
                                height = 100.f;
                            }
                            sf::Vector2f center = (createStartPos + mousePos) / 2.f;
                            auto fig = createFigure(currentTool, width, height);
                            if (fig) {
                                if (scene.customOriginActive) {
                                    fig->parentOrigin = scene.customOriginPos;
                                    fig->anchor = center - scene.customOriginPos;
                                } else {
                                    fig->anchor = center;
                                    fig->parentOrigin = sf::Vector2f(0.f, 0.f);
                                }
                                Figure* raw = fig.get();
                                scene.addFigure(std::move(fig));
                                scene.setSelectedFigure(raw);
                            }
                            currentTool = ui::Tool::Select;
                        }
                    }
                } else if (event.type == sf::Event::MouseMoved) {
                    sf::Vector2f mousePos = viewport.screenToWorld(sf::Vector2f(event.mouseMove.x, event.mouseMove.y));

                    if (isDraggingCustomOrigin) {
                        scene.setCustomOrigin(mousePos);
                    } else if (isPanning) {
                        sf::Vector2f currentMouse(event.mouseMove.x, event.mouseMove.y);
                        sf::Vector2f delta = currentMouse - panStartMouse;
                        viewport.worldOrigin = panStartOrigin + delta;
                    } else if (draggingScaleHandle != ScaleHandle::None && scene.getSelectedFigure()) {
                        core::Figure* selFig = scene.getSelectedFigure();
                        sf::Vector2f delta = mousePos - scaleStartMouse;
                        float rad = -selFig->rotationAngle * math::PI / 180.f;
                        float dx = delta.x * std::cos(rad) - delta.y * std::sin(rad);
                        float dy = delta.x * std::sin(rad) + delta.y * std::cos(rad);

                        sf::FloatRect localBounds = selFig->getLocalBoundingBox();
                        sf::Vector2f newScale = scaleStartValue;
                        bool shift = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ||
                                     sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);

                        float curAbsWidth = localBounds.width * std::abs(scaleStartValue.x);
                        float curAbsHeight = localBounds.height * std::abs(scaleStartValue.y);
                        if (curAbsWidth < 0.001f) curAbsWidth = 0.001f;
                        if (curAbsHeight < 0.001f) curAbsHeight = 0.001f;

                        float scaleX_mult = 1.0f, scaleY_mult = 1.0f;
                        if (draggingScaleHandle == ScaleHandle::TR || draggingScaleHandle == ScaleHandle::CR || draggingScaleHandle == ScaleHandle::BR)
                            scaleX_mult = (curAbsWidth + dx) / curAbsWidth;
                        else if (draggingScaleHandle == ScaleHandle::TL || draggingScaleHandle == ScaleHandle::CL || draggingScaleHandle == ScaleHandle::BL)
                            scaleX_mult = (curAbsWidth - dx) / curAbsWidth;
                        if (draggingScaleHandle == ScaleHandle::TR || draggingScaleHandle == ScaleHandle::TC || draggingScaleHandle == ScaleHandle::TL)
                            scaleY_mult = (curAbsHeight - dy) / curAbsHeight;
                        else if (draggingScaleHandle == ScaleHandle::BR || draggingScaleHandle == ScaleHandle::BC || draggingScaleHandle == ScaleHandle::BL)
                            scaleY_mult = (curAbsHeight + dy) / curAbsHeight;

                        bool isCorner = (draggingScaleHandle == ScaleHandle::TL || draggingScaleHandle == ScaleHandle::TR ||
                                         draggingScaleHandle == ScaleHandle::BL || draggingScaleHandle == ScaleHandle::BR);
                        if (shift && isCorner) {
                            float maxMult = std::max(std::abs(scaleX_mult), std::abs(scaleY_mult));
                            scaleX_mult = maxMult * (scaleX_mult < 0 ? -1.f : 1.f);
                            scaleY_mult = maxMult * (scaleY_mult < 0 ? -1.f : 1.f);
                        }

                        newScale.x = scaleStartValue.x * scaleX_mult;
                        newScale.y = scaleStartValue.y * scaleY_mult;
                        if (std::abs(newScale.x) < 0.01f) newScale.x = 0.01f * (newScale.x < 0 ? -1.f : 1.f);
                        if (std::abs(newScale.y) < 0.01f) newScale.y = 0.01f * (newScale.y < 0 ? -1.f : 1.f);
                        selFig->scale = newScale;

                        sf::Vector2f v_inv(0.f, 0.f);
                        if (draggingScaleHandle == ScaleHandle::TL || draggingScaleHandle == ScaleHandle::CL || draggingScaleHandle == ScaleHandle::BL)
                            v_inv.x = localBounds.left + localBounds.width;
                        else if (draggingScaleHandle == ScaleHandle::TR || draggingScaleHandle == ScaleHandle::CR || draggingScaleHandle == ScaleHandle::BR)
                            v_inv.x = localBounds.left;
                        if (draggingScaleHandle == ScaleHandle::TL || draggingScaleHandle == ScaleHandle::TC || draggingScaleHandle == ScaleHandle::TR)
                            v_inv.y = localBounds.top + localBounds.height;
                        else if (draggingScaleHandle == ScaleHandle::BL || draggingScaleHandle == ScaleHandle::BC || draggingScaleHandle == ScaleHandle::BR)
                            v_inv.y = localBounds.top;

                        sf::Vector2f V_old(v_inv.x * scaleStartValue.x, v_inv.y * scaleStartValue.y);
                        sf::Vector2f V_new(v_inv.x * newScale.x, v_inv.y * newScale.y);
                        sf::Vector2f delta_V = V_old - V_new;
                        float rad2 = selFig->rotationAngle * math::PI / 180.f;
                        float anchor_dx = delta_V.x * std::cos(rad2) - delta_V.y * std::sin(rad2);
                        float anchor_dy = delta_V.x * std::sin(rad2) + delta_V.y * std::cos(rad2);
                        selFig->anchor = scaleStartAnchor + sf::Vector2f(anchor_dx, anchor_dy);
                    } else if (isRotating && scene.getSelectedFigure()) {
                        sf::Vector2f absoluteAnchor = scene.getSelectedFigure()->parentOrigin + scene.getSelectedFigure()->anchor;
                        float currentAngle = std::atan2(mousePos.y - absoluteAnchor.y, mousePos.x - absoluteAnchor.x) * 180.f / math::PI;
                        float delta = currentAngle - rotationStartAngle;
                        float newRot = initialRotation + delta;
                        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift))
                            newRot = std::round(newRot / 15.f) * 15.f;
                        scene.getSelectedFigure()->rotationAngle = newRot;
                    } else if (draggingVertexIndex != -1 && scene.getSelectedFigure()) {
                        auto& verts = scene.getSelectedFigure()->getVerticesMutable();
                        if (draggingVertexIndex >= 0 && draggingVertexIndex < static_cast<int>(verts.size())) {
                            sf::Vector2f absoluteAnchor = scene.getSelectedFigure()->parentOrigin + scene.getSelectedFigure()->anchor;
                            sf::Vector2f deltaAbs = mousePos - absoluteAnchor;
                            float invRad = -scene.getSelectedFigure()->rotationAngle * math::PI / 180.f;
                            float rx = deltaAbs.x * std::cos(invRad) - deltaAbs.y * std::sin(invRad);
                            float ry = deltaAbs.x * std::sin(invRad) + deltaAbs.y * std::cos(invRad);
                            float scaledX = rx / scene.getSelectedFigure()->scale.x;
                            float scaledY = ry / scene.getSelectedFigure()->scale.y;
                            verts[draggingVertexIndex] = sf::Vector2f(scaledX, scaledY);
                        }
                    } else if (isDraggingAnchor && scene.getSelectedFigure()) {
                        sf::Vector2f newAbsoluteAnchor = mousePos - dragOffset;
                        sf::Vector2f newAnchor = newAbsoluteAnchor - scene.getSelectedFigure()->parentOrigin;
                        bool selectedIsComposite = dynamic_cast<core::CompositeFigure*>(scene.getSelectedFigure()) != nullptr;
                        if (propertiesPanel.m_lockAnchor || selectedIsComposite) {
                            scene.getSelectedFigure()->move(newAnchor - scene.getSelectedFigure()->anchor);
                        } else {
                            scene.getSelectedFigure()->setAnchorKeepAbsolute(newAnchor);
                        }
                    } else if (isDragging && scene.getSelectedFigure()) {
                        sf::Vector2f newAbsoluteAnchor = mousePos - dragOffset;
                        sf::Vector2f newAnchor = newAbsoluteAnchor - scene.getSelectedFigure()->parentOrigin;
                        scene.getSelectedFigure()->move(newAnchor - scene.getSelectedFigure()->anchor);
                    }
                }
            }
        }

        bool groupClicked = false;
        bool ungroupClicked = false;
        bool multiSel = scene.getSelection().size() >= 2;
        bool selIsComposite = dynamic_cast<core::CompositeFigure*>(scene.getSelectedFigure()) != nullptr;

        ScaleHandle newHoverHandle = ScaleHandle::None;
        if (!isNodeEditMode && scene.getSelectedFigure()) {
            auto selFig = scene.getSelectedFigure();
            sf::Vector2f mousePos = viewport.screenToWorld(sf::Vector2f(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y));
            sf::FloatRect localBounds = selFig->getLocalBoundingBox();
            sf::Vector2f tl(localBounds.left, localBounds.top);
            sf::Vector2f tr(localBounds.left + localBounds.width, localBounds.top);
            sf::Vector2f bl(localBounds.left, localBounds.top + localBounds.height);
            sf::Vector2f br(localBounds.left + localBounds.width, localBounds.top + localBounds.height);
            sf::Vector2f tc = (tl + tr) / 2.f;
            sf::Vector2f bc = (bl + br) / 2.f;
            sf::Vector2f cl = (tl + bl) / 2.f;
            sf::Vector2f cr = (tr + br) / 2.f;
            std::vector<sf::Vector2f> handles = {tl, tc, tr, cr, br, bc, bl, cl};
            float markerScale = 1.f / viewport.zoom;
            for (int i = 0; i < 8; ++i) {
                sf::Vector2f absH = selFig->getAbsoluteVertex(handles[i]);
                if (std::hypot(mousePos.x - absH.x, mousePos.y - absH.y) <= 8.f * markerScale) {
                    newHoverHandle = static_cast<ScaleHandle>(i + 1);
                    break;
                }
            }
        }
        hoveringScaleHandle = newHoverHandle;

        ImGuiIO& io = ImGui::GetIO();
        if (!io.WantCaptureMouse) {
            sf::Vector2f mousePos = viewport.screenToWorld(sf::Vector2f(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y));
            ScaleHandle activeHandle = draggingScaleHandle != ScaleHandle::None ? draggingScaleHandle : hoveringScaleHandle;
            if (isDraggingCustomOrigin || isPanning || (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && currentTool == ui::Tool::Select))
                window.setMouseCursor(cursorHand);
            else if (scene.customOriginActive &&
                     std::hypot(sf::Mouse::getPosition(window).x - viewport.worldToScreen(scene.customOriginPos).x,
                                sf::Mouse::getPosition(window).y - viewport.worldToScreen(scene.customOriginPos).y) <= 15.f)
                window.setMouseCursor(cursorHand);
            else if (activeHandle != ScaleHandle::None) {
                if (activeHandle == ScaleHandle::TL || activeHandle == ScaleHandle::BR)
                    window.setMouseCursor(cursorSizeNWSE);
                else if (activeHandle == ScaleHandle::TR || activeHandle == ScaleHandle::BL)
                    window.setMouseCursor(cursorSizeNESW);
                else if (activeHandle == ScaleHandle::TC || activeHandle == ScaleHandle::BC)
                    window.setMouseCursor(cursorSizeNS);
                else if (activeHandle == ScaleHandle::CL || activeHandle == ScaleHandle::CR)
                    window.setMouseCursor(cursorSizeWE);
            } else if (currentTool != ui::Tool::Select)
                window.setMouseCursor(cursorCross);
            else if (isDragging)
                window.setMouseCursor(cursorSizeAll);
            else if (scene.hitTest(mousePos) != nullptr)
                window.setMouseCursor(cursorHand);
            else
                window.setMouseCursor(cursorArrow);
        } else {
            window.setMouseCursor(cursorArrow);
        }

        sf::Time dt = deltaClock.restart();
        for (const auto& fig : scene.getFigures())
            fig->updateEdgeEffects(dt.asSeconds());

        ImGui::SFML::Update(window, dt);

        toolbar.render(currentTool, multiSel, &groupClicked, selIsComposite, &ungroupClicked);
        bool fitRequested = propertiesPanel.render(scene, viewport);
        createModal.render(scene);

        if (groupClicked && multiSel) {
            auto composite = std::make_unique<core::CompositeFigure>();
            auto selection = scene.getSelection();
            sf::Vector2f centroid(0.f, 0.f);
            for (Figure* f : selection)
                centroid += f->parentOrigin + f->anchor;
            centroid /= static_cast<float>(selection.size());

            for (Figure* f : selection) {
                auto owned = scene.takeFigure(f);
                if (owned)
                    composite->addChild(std::move(owned));
            }

            if (scene.customOriginActive) {
                composite->parentOrigin = scene.customOriginPos;
                composite->anchor = centroid - scene.customOriginPos;
            } else {
                composite->anchor = centroid;
            }
            Figure* raw = composite.get();
            scene.addFigure(std::move(composite));
            scene.setSelectedFigure(raw);
        }

        if (ungroupClicked && selIsComposite) {
            auto* comp = dynamic_cast<core::CompositeFigure*>(scene.getSelectedFigure());
            if (comp) {
                std::vector<std::unique_ptr<Figure>> children;
                while (!comp->getChildren().empty()) {
                    auto child = comp->removeChild(comp->getChildren().front().get());
                    if (child)
                        children.push_back(std::move(child));
                }
                scene.removeFigure(comp);
                scene.clearSelection();
                for (auto& child : children) {
                    Figure* raw = child.get();
                    scene.addFigure(std::move(child));
                    scene.addToSelection(raw);
                }
            }
        }

        if (fitRequested && !scene.getFigures().empty()) {
            const auto& figs = scene.getFigures();
            sf::FloatRect first = figs.front()->getBoundingBox();
            float minX = first.left, minY = first.top;
            float maxX = first.left + first.width, maxY = first.top + first.height;
            for (size_t i = 1; i < figs.size(); ++i) {
                sf::FloatRect b = figs[i]->getBoundingBox();
                minX = std::min(minX, b.left);
                minY = std::min(minY, b.top);
                maxX = std::max(maxX, b.left + b.width);
                maxY = std::max(maxY, b.top + b.height);
            }
            float margin = 50.f;
            float fwidth = maxX - minX + margin * 2.f;
            float fheight = maxY - minY + margin * 2.f;
            float zoomX = window.getSize().x / fwidth;
            float zoomY = window.getSize().y / fheight;
            viewport.zoom = std::clamp(std::min(zoomX, zoomY), 0.05f, 50.f);
            sf::Vector2f centerWorld(minX + (maxX - minX) / 2.f, minY + (maxY - minY) / 2.f);
            sf::Vector2f centerScreen(window.getSize().x / 2.f, window.getSize().y / 2.f);
            viewport.worldOrigin = centerScreen - centerWorld * viewport.zoom;
        }

        ImGui::SetNextWindowPos(ImVec2(0, window.getSize().y - 30), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(window.getSize().x - 320, 30), ImGuiCond_Always);
        ImGuiWindowFlags statusFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 5));
        ImGui::Begin("Status Bar", nullptr, statusFlags);
        sf::Vector2f mPos = viewport.screenToWorld(sf::Vector2f(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y));
        char statusText[256];
        if (currentTool == ui::Tool::Polyline && polylineState.active)
            std::snprintf(statusText, sizeof(statusText),
                u8"Полилиния: %zu точек | ПКМ — завершить | Zoom: %.0f%% | (%.1f, %.1f)",
                polylineState.points.size(), viewport.zoom * 100.f, mPos.x, mPos.y);
        else
            std::snprintf(statusText, sizeof(statusText),
                "Zoom: %.0f%% | Cursor: (%.1f, %.1f)", viewport.zoom * 100.f, mPos.x, mPos.y);
        float textWidth = ImGui::CalcTextSize(statusText).x;
        float windowWidth = ImGui::GetWindowSize().x;
        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
        ImGui::TextUnformatted(statusText);
        ImGui::PopStyleColor();
        ImGui::End();
        ImGui::PopStyleVar();

        window.clear(sf::Color(240, 240, 240));
        sf::View oldView = window.getView();
        window.setView(viewport.getView(sf::Vector2f(window.getSize())));

        if (showGrid) {
            sf::VertexArray grid(sf::Lines);
            sf::Color gridColor(200, 200, 200);
            int gridSize = 50;
            sf::Vector2f viewCenter = window.getView().getCenter();
            sf::Vector2f viewSize = window.getView().getSize();
            float startX = std::floor((viewCenter.x - viewSize.x / 2.f) / gridSize) * gridSize;
            float endX = std::floor((viewCenter.x + viewSize.x / 2.f) / gridSize) * gridSize + gridSize;
            float startY = std::floor((viewCenter.y - viewSize.y / 2.f) / gridSize) * gridSize;
            float endY = std::floor((viewCenter.y + viewSize.y / 2.f) / gridSize) * gridSize + gridSize;
            for (float x = startX; x <= endX; x += gridSize) {
                grid.append(sf::Vertex(sf::Vector2f(x, startY), gridColor));
                grid.append(sf::Vertex(sf::Vector2f(x, endY), gridColor));
            }
            for (float y = startY; y <= endY; y += gridSize) {
                grid.append(sf::Vertex(sf::Vector2f(startX, y), gridColor));
                grid.append(sf::Vertex(sf::Vector2f(endX, y), gridColor));
            }
            window.draw(grid);
        }

        auto drawOrigins = [&]() {
            if (showOriginAxes) {
                sf::Vector2f activeOrigin = scene.customOriginActive ? scene.customOriginPos : sf::Vector2f(0.f, 0.f);
                sf::VertexArray originAxes(sf::Lines, 4);
                sf::Color axisColor(100, 100, 100, 150);
                sf::Vector2f boundsMin = viewport.screenToWorld(sf::Vector2f(0.f, 0.f));
                sf::Vector2f boundsMax = viewport.screenToWorld(sf::Vector2f(window.getSize().x, window.getSize().y));
                originAxes[0] = sf::Vertex(sf::Vector2f(boundsMin.x, activeOrigin.y), axisColor);
                originAxes[1] = sf::Vertex(sf::Vector2f(boundsMax.x, activeOrigin.y), axisColor);
                originAxes[2] = sf::Vertex(sf::Vector2f(activeOrigin.x, boundsMin.y), axisColor);
                originAxes[3] = sf::Vertex(sf::Vector2f(activeOrigin.x, boundsMax.y), axisColor);
                window.draw(originAxes);
            }
            float markerScale = 1.f / viewport.zoom;
            sf::Color crossCol(80, 80, 80);
            float length = 24.f * markerScale, thickness = 2.f * markerScale;
            sf::RectangleShape hLine(sf::Vector2f(length, thickness));
            hLine.setOrigin(length / 2.f, thickness / 2.f);
            hLine.setPosition(0.f, 0.f);
            hLine.setFillColor(crossCol);
            sf::RectangleShape vLine(sf::Vector2f(thickness, length));
            vLine.setOrigin(thickness / 2.f, length / 2.f);
            vLine.setPosition(0.f, 0.f);
            vLine.setFillColor(crossCol);
            window.draw(hLine);
            window.draw(vLine);

            if (scene.customOriginActive) {
                sf::Vector2f cPos = scene.customOriginPos;
                sf::CircleShape circ(6.f * markerScale);
                circ.setOrigin(6.f * markerScale, 6.f * markerScale);
                circ.setPosition(cPos);
                circ.setFillColor(sf::Color::Transparent);
                circ.setOutlineColor(sf::Color(255, 50, 50));
                circ.setOutlineThickness(1.5f * markerScale);
                window.draw(circ);
                sf::VertexArray cross(sf::Lines, 4);
                sf::Color crossColor(255, 50, 50);
                cross[0] = sf::Vertex(sf::Vector2f(cPos.x - 12.f * markerScale, cPos.y), crossColor);
                cross[1] = sf::Vertex(sf::Vector2f(cPos.x + 12.f * markerScale, cPos.y), crossColor);
                cross[2] = sf::Vertex(sf::Vector2f(cPos.x, cPos.y - 12.f * markerScale), crossColor);
                cross[3] = sf::Vertex(sf::Vector2f(cPos.x, cPos.y + 12.f * markerScale), crossColor);
                window.draw(cross);
            }
        };

        if (!propertiesPanel.m_drawOriginsOverFigures)
            drawOrigins();
        scene.drawAll(window, 1.f / viewport.zoom);

        if (scene.getSelectedFigure() && propertiesPanel.m_selectedSegmentIndex >= 0) {
            auto* selected = scene.getSelectedFigure();
            const auto& verts = selected->getVertices();
            auto* polyline = dynamic_cast<core::PolylineShape*>(selected);
            bool isOpenPolyline = polyline && !polyline->isClosed();

            auto drawHighlightedEdge = [&](int edgeIndex, sf::Color color) {
                if (edgeIndex < 0) return;
                size_t idx = static_cast<size_t>(edgeIndex);
                if (verts.size() < 2) return;
                if (isOpenPolyline) {
                    if (idx + 1 >= verts.size()) return;
                } else {
                    if (idx >= verts.size()) return;
                }

                sf::Vector2f a = selected->getAbsoluteVertex(verts[idx]);
                sf::Vector2f b = selected->getAbsoluteVertex(verts[isOpenPolyline ? idx + 1 : (idx + 1) % verts.size()]);
                sf::Vector2f delta = b - a;
                float len = std::hypot(delta.x, delta.y);
                if (len <= 0.0001f) return;

                sf::Vector2f dir = delta / len;
                sf::Vector2f normal(-dir.y, dir.x);
                float width = 6.f / viewport.zoom;

                sf::ConvexShape quad(4);
                quad.setPoint(0, a - normal * width);
                quad.setPoint(1, a + normal * width);
                quad.setPoint(2, b + normal * width);
                quad.setPoint(3, b - normal * width);
                quad.setFillColor(color);
                window.draw(quad);
            };

            if (propertiesPanel.m_selectedSegmentIsAngle && polyline) {
                drawHighlightedEdge(propertiesPanel.m_selectedSegmentIndex, sf::Color(255, 170, 0, 180));
                if (propertiesPanel.m_selectedSegmentIndex > 0) {
                    drawHighlightedEdge(propertiesPanel.m_selectedSegmentIndex - 1, sf::Color(255, 220, 0, 180));
                }
            } else {
                drawHighlightedEdge(propertiesPanel.m_selectedSegmentIndex, sf::Color(255, 235, 59, 190));
            }
        }

        if (propertiesPanel.m_drawOriginsOverFigures)
            drawOrigins();

        if (currentTool == ui::Tool::Polyline && polylineState.active && !polylineState.points.empty()) {
            sf::Vector2f curMouse = viewport.screenToWorld(sf::Vector2f(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y));
            sf::VertexArray lines(sf::LinesStrip);
            sf::Color previewColor(0, 120, 215, 200);
            for (const auto& p : polylineState.points)
                lines.append(sf::Vertex(p, previewColor));
            lines.append(sf::Vertex(curMouse, previewColor));
            window.draw(lines);

            float ms = 5.f / viewport.zoom;
            for (const auto& p : polylineState.points) {
                sf::CircleShape marker(ms);
                marker.setOrigin(ms, ms);
                marker.setPosition(p);
                marker.setFillColor(sf::Color::White);
                marker.setOutlineColor(sf::Color(0, 120, 215));
                marker.setOutlineThickness(1.5f / viewport.zoom);
                window.draw(marker);
            }
        }

        if (scene.getSelectedFigure()) {
            float markerScale = 1.f / viewport.zoom;
            drawAnchorMarker(window, scene.getSelectedFigure()->parentOrigin + scene.getSelectedFigure()->anchor, markerScale);

            sf::FloatRect localBounds = scene.getSelectedFigure()->getLocalBoundingBox();
            sf::Vector2f tl(localBounds.left, localBounds.top);
            sf::Vector2f tr(localBounds.left + localBounds.width, localBounds.top);
            sf::Vector2f bl(localBounds.left, localBounds.top + localBounds.height);
            sf::Vector2f br(localBounds.left + localBounds.width, localBounds.top + localBounds.height);
            sf::Vector2f tc = (tl + tr) / 2.f;
            sf::Vector2f bc = (bl + br) / 2.f;
            sf::Vector2f cl = (tl + bl) / 2.f;
            sf::Vector2f cr = (tr + br) / 2.f;
            std::vector<sf::Vector2f> handles = {tl, tc, tr, cr, br, bc, bl, cl};

            if (!isNodeEditMode) {
                for (const auto& h : handles) {
                    sf::RectangleShape handle(sf::Vector2f(8.f * markerScale, 8.f * markerScale));
                    handle.setOrigin(4.f * markerScale, 4.f * markerScale);
                    handle.setPosition(scene.getSelectedFigure()->getAbsoluteVertex(h));
                    handle.setRotation(scene.getSelectedFigure()->rotationAngle);
                    handle.setFillColor(sf::Color::White);
                    handle.setOutlineColor(sf::Color(0, 120, 215));
                    handle.setOutlineThickness(1.5f * markerScale);
                    window.draw(handle);
                }
            }

            sf::Vector2f absTc = scene.getSelectedFigure()->getAbsoluteVertex(tc);
            float rotRad = scene.getSelectedFigure()->rotationAngle * math::PI / 180.f;
            sf::Vector2f rotOffset(std::sin(rotRad) * 20.f * markerScale, -std::cos(rotRad) * 20.f * markerScale);
            sf::Vector2f rotPos = absTc + rotOffset;
            sf::CircleShape rotMarker(5.f * markerScale);
            rotMarker.setOrigin(5.f * markerScale, 5.f * markerScale);
            rotMarker.setPosition(rotPos);
            rotMarker.setFillColor(sf::Color::White);
            rotMarker.setOutlineColor(sf::Color(0, 120, 215));
            rotMarker.setOutlineThickness(1.5f * markerScale);
            window.draw(rotMarker);
            sf::VertexArray rotLine(sf::Lines, 2);
            rotLine[0] = sf::Vertex(absTc, sf::Color(0, 120, 215));
            rotLine[1] = sf::Vertex(rotPos, sf::Color(0, 120, 215));
            window.draw(rotLine);

            if (isNodeEditMode) {
                const auto& verts = scene.getSelectedFigure()->getVertices();
                for (const auto& v : verts) {
                    sf::RectangleShape handle(sf::Vector2f(8.f * markerScale, 8.f * markerScale));
                    handle.setOrigin(4.f * markerScale, 4.f * markerScale);
                    handle.setPosition(scene.getSelectedFigure()->getAbsoluteVertex(v));
                    handle.setFillColor(sf::Color::White);
                    handle.setOutlineColor(sf::Color(0, 120, 215));
                    handle.setOutlineThickness(1.5f * markerScale);
                    window.draw(handle);
                }
            }
        }

        if (isCreating && creatingStep == 1 && currentTool != ui::Tool::Select) {
            sf::Vector2f mousePos = viewport.screenToWorld(sf::Vector2f(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y));
            float dx = mousePos.x - createStartPos.x;
            float dy = mousePos.y - createStartPos.y;
            float width = std::abs(dx), height = std::abs(dy);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) {
                float maxDim = std::max(width, height);
                width = maxDim;
                height = maxDim;
                mousePos.x = createStartPos.x + ((dx >= 0) ? maxDim : -maxDim);
                mousePos.y = createStartPos.y + ((dy >= 0) ? maxDim : -maxDim);
            }
            sf::RectangleShape preview;
            preview.setPosition(std::min(createStartPos.x, mousePos.x), std::min(createStartPos.y, mousePos.y));
            preview.setSize(sf::Vector2f(width, height));
            preview.setFillColor(sf::Color(150, 150, 150, 100));
            preview.setOutlineColor(sf::Color(100, 100, 100, 200));
            preview.setOutlineThickness(1.f);
            window.draw(preview);
        }

        window.setView(oldView);
        window.resetGLStates();
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}
