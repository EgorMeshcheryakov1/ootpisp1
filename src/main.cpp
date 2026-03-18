#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include "core/Figures.hpp"
#include "core/PolylineShape.hpp"
#include "core/CompositeFigure.hpp"
#include "core/Scene.hpp"
#include "core/Viewport.hpp"
#include "core/MathUtils.hpp"
#include "ui/CreateFigureModal.hpp"
#include "ui/PropertiesPanel.hpp"
#include "ui/Toolbar.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <cmath>
#include <imgui-SFML.h>
#include <imgui.h>
#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>
#include <cstdio>