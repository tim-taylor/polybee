/**
 * @file
 *
 * Implementation of the LocalVis class
 */

 /*
  * Some sections of this file are based upon the Raylib example file, which has the
  * following copyright notice attached.
  *
 -- Copyright (c) 2020-2024 Jeffery Myers
 --
 --This software is provided "as-is", without any express or implied warranty. In no event
 --will the authors be held liable for any damages arising from the use of this software.

 --Permission is granted to anyone to use this software for any purpose, including commercial
 --applications, and to alter it and redistribute it freely, subject to the following restrictions:

 --  1. The origin of this software must not be misrepresented; you must not claim that you
 --  wrote the original software. If you use this software in a product, an acknowledgment
 --  in the product documentation would be appreciated but is not required.
 --
 --  2. Altered source versions must be plainly marked as such, and must not be misrepresented
 --  as being the original software.
 --
 --  3. This notice may not be removed or altered from any source distribution.
  *
  */

#include "raylib.h"
#include "raymath.h"
#include "Params.h"
#include "PolyBeeCore.h"
#include <format>
#include <chrono>
#include <thread>
#include <cmath>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#include "LocalVis.h"

const int DISPLAY_MARGIN_TOP = 60;
const int DISPLAY_MARGIN_BOTTOM = 50;
const int DISPLAY_MARGIN_LEFT = 50;
const int DISPLAY_MARGIN_RIGHT = 50;

const float HIVE_SIZE = 20.0f;
const float HALF_HIVE_SIZE = HIVE_SIZE / 2.0f;
const float PLANT_SIZE = 5.0f;
const float HALF_PLANT_SIZE = PLANT_SIZE / 2.0f;
const std::vector<Vector2> BEE_SHAPE = { {10, 0}, {-6, -6}, {-6, 6} }; // triangle shape for drawing bees
const float BEE_SCALING_FACTOR = 0.75f; // scale down bee shape for drawing
const float BEE_PATH_THICKNESS = 3.0f;
const float TUNNEL_WALL_VISUAL_THICKNESS = 10.0f;

const Color ENV_BACKGROUND_COLOR { 203, 189, 147, 255 };
const Color ENV_BORDER_COLOR = WHITE;
const Color TUNNEL_BACKGROUND_COLOR = BROWN;
const Color TUNNEL_ENTRANCE_COLOR = WHITE;
const Color TUNNEL_BORDER_COLOR = WHITE;

const int FONT_SIZE_REG = 20;
const int FONT_SIZE_LARGE = 40;

const int MAX_DELAY_PER_STEP = 100;

LocalVis::LocalVis(PolyBeeCore* pPolyBeeCore) :
    m_pPolyBeeCore{pPolyBeeCore}, m_camera{0}, m_currentEMD{0.0f}, m_currentEMDTime{0}
{
    SetTraceLogLevel(4); // Level 4 suppresses INFO msgs from RayLib

    // Create the window and OpenGL context
    InitWindow(Params::envW * Params::visCellSize + DISPLAY_MARGIN_LEFT + DISPLAY_MARGIN_RIGHT,
        Params::envH * Params::visCellSize + DISPLAY_MARGIN_TOP + DISPLAY_MARGIN_BOTTOM, "polybee");

    // set up the 2D camera
    Vector2 center = { (Params::envW * Params::visCellSize) / 2.0f + DISPLAY_MARGIN_LEFT,
                       (Params::envH * Params::visCellSize) / 2.0f + DISPLAY_MARGIN_TOP };
    m_camera.target = (Vector2){ center.x, center.y };
    m_camera.offset = (Vector2){ center.x, center.y };
    m_camera.rotation = 0.0f;
    m_camera.zoom = 1.0f;

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    // Tell the window to use vsync and work on high DPI displays
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);

    if (true && !IsWindowFullscreen())
    {
        // ToggleFullscreen();
        int mon = GetCurrentMonitor();
        int mw = GetMonitorWidth(mon);
        int mh = GetMonitorHeight(mon);
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();
        std::cout << "-----> Monitor " << mon << ", mw " << mw << ", mh " << mh
            << ", sw " << sw << ", sh " << sh << std::endl;
        // SetWindowSize(, );
    }
#endif
}


LocalVis::~LocalVis()
{
    // destroy the window and cleanup the OpenGL context
    CloseWindow();
}


// Convert environment coordinates to display coordinates

// ... convert an X coordinate
float LocalVis::envToDisplayX(float envX) const {
    return ((envX + m_displayOffset.x) * Params::visCellSize) + DISPLAY_MARGIN_LEFT;
}

// ... convert a Y coordinate
float LocalVis::envToDisplayY(float envY) const {
    return ((envY + m_displayOffset.y) * Params::visCellSize) + DISPLAY_MARGIN_TOP;
}

// ... convert a length/size N
float LocalVis::envToDisplayN(float displayN) const {
    return displayN * Params::visCellSize;
}

// ... convert a Rectangle
Rectangle LocalVis::envToDisplayRect(const Rectangle& rect) const {
    return { envToDisplayX(rect.x), envToDisplayY(rect.y), envToDisplayN(rect.width), envToDisplayN(rect.height) };
}


void LocalVis::updateDrawFrame()
{

    if (WindowShouldClose()) { // Detect window close button or ESC key{
        m_pPolyBeeCore->earlyExit();
    }

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

    BeginMode2D(m_camera);

        // Setup the back buffer for drawing (clear color and depth buffers)
        ClearBackground(BLACK);

        // consider using the following functions if implementing zoom so we can
        // figure out which cells need to be drawn:
        //   Vector2 GetWorldToScreen2D(Vector2 position, Camera2D camera);    // Get the screen space position for a 2d camera world space position
        //   Vector2 GetScreenToWorld2D(Vector2 position, Camera2D camera);    // Get the world space position for a 2d camera screen space position
        //   Matrix GetCameraMatrix(Camera camera);                            // Get camera transform matrix (view matrix)
        //   Matrix GetCameraMatrix2D(Camera2D camera);                        // Get camera 2d transform matrix

        // draw environment rectangle and boundary
        if (!showHeatmap()) {
            DrawRectangleRec(
                envToDisplayRect({0.0f, 0.0f, Params::envW, Params::envH}), ENV_BACKGROUND_COLOR);
        }
        DrawRectangleLinesEx(
            envToDisplayRect({0.0f, 0.0f, Params::envW, Params::envH}), 5.0f, ENV_BORDER_COLOR);


        // draw tunnel rectangle and boundary
        drawTunnel();

        // draw plant patches
        drawPatches();

        // draw plants
        drawPlants();

        // draw bees
        if (showBees()) {
            drawBees();
        }

        // draw heatmap
        if (showHeatmap()) {
            drawHeatmap();
        }

    EndMode2D();

    // draw status text
    drawStatusText();

    // end the frame and get ready for the next one  (display frame, poll input, etc...)
    EndDrawing();

    // handle input
    processKeyboardInput();

    // sleep for a short time to control frame rate if requested
    if (Params::visDelayPerStep > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(Params::visDelayPerStep));
    }
}


void LocalVis::drawTunnel()
{
    auto& tunnel = m_pPolyBeeCore->m_env.getTunnel();

    // draw tunnel rectangle and boundary
    if (!showHeatmap()) {
        DrawRectangleRec(
            envToDisplayRect({tunnel.x(), tunnel.y(), tunnel.width(), tunnel.height()}), TUNNEL_BACKGROUND_COLOR);
    }
    DrawRectangleLinesEx(
        envToDisplayRect({tunnel.x(), tunnel.y(), tunnel.width(), tunnel.height()}), 5.0, TUNNEL_BORDER_COLOR);

    auto& entrances = m_pPolyBeeCore->m_env.getTunnel().getEntrances();
    for (const TunnelEntranceInfo& entrance : entrances) {
        Rectangle entranceRect;
        switch (entrance.side) {
        case 0: { // North
            entranceRect = { entrance.x1, entrance.y1 - TUNNEL_WALL_VISUAL_THICKNESS,
                             entrance.x2 - entrance.x1, TUNNEL_WALL_VISUAL_THICKNESS };
            break;
        }
        case 1: { // East
            entranceRect = { entrance.x1, entrance.y1,
                             TUNNEL_WALL_VISUAL_THICKNESS, entrance.y2 - entrance.y1 };
            break;
        }
        case 2: { // South
            entranceRect = { entrance.x1, entrance.y1,
                             entrance.x2 - entrance.x1, TUNNEL_WALL_VISUAL_THICKNESS };
            break;
        }
        case 3: { // West
            entranceRect = { entrance.x1 - TUNNEL_WALL_VISUAL_THICKNESS, entrance.y1,
                             TUNNEL_WALL_VISUAL_THICKNESS, entrance.y2 - entrance.y1 };
            break;
        }
        default:
            pb::msg_error_and_exit("LocalVis::drawTunnel(): invalid entrance side");
        }

        DrawRectangleRec(envToDisplayRect(entranceRect), TUNNEL_ENTRANCE_COLOR);
    }
}


void LocalVis::drawPatches()
{
    for (const PatchSpec& patchSpec : Params::patchSpecs) {
        Rectangle patchRect = { patchSpec.x,  patchSpec.y, patchSpec.w, patchSpec.h };
        for (int p=0; p<patchSpec.numRepeats; ++p) {
            DrawRectangleRec(envToDisplayRect(patchRect), GRAY);
            patchRect.x += patchSpec.dx;
            patchRect.y += patchSpec.dy;
        }
    }
}


void LocalVis::drawPlants()
{
    for (const Plant* pPlant : m_pPolyBeeCore->m_env.getPlantPtrs()) {
        float displayX = envToDisplayX(pPlant->x());
        float displayY = envToDisplayY(pPlant->y());
        float displaySize = envToDisplayN(HALF_PLANT_SIZE);
        DrawCircleV({ displayX, displayY }, displaySize, GREEN);
    }
}


void LocalVis::drawStatusText()
{
    std::string msg;
    if (m_bWaitingForUserToClose) {
        msg = std::format("Finished {} iterations. Press ESC to exit", m_pPolyBeeCore->m_iIteration);
    }
    else {
        msg = std::format("Iteration target {}. Current iteration {}\nSim speed {}",
            Params::numIterations, m_pPolyBeeCore->m_iIteration, MAX_DELAY_PER_STEP - Params::visDelayPerStep);
    }
    DrawText(msg.c_str(), 10, 10, FONT_SIZE_REG, RAYWHITE);

    if (m_bPaused) {
        DrawText("PAUSED", 10, 40, FONT_SIZE_LARGE, RAYWHITE);
    }

    if (showHeatmap() && m_bShowEMD) {
        DrawText(std::format("EMD (OpenCV) to uniform target: {:.4f} :: {} microseconds", m_currentEMD, m_currentEMDTime).c_str(),
            10, GetScreenHeight() - 30, FONT_SIZE_REG, RAYWHITE);
    }
}


void LocalVis::processKeyboardInput()
{
   if (IsKeyPressed(KEY_H)) {
        rotateDrawState();
    }

    if (IsKeyPressed(KEY_P)) {
        m_bPaused = !m_bPaused;
        m_pPolyBeeCore->pauseSimulation(m_bPaused);
    }

    if (IsKeyPressed(KEY_T)) {
        m_bShowTrails = !m_bShowTrails;
    }

    if (IsKeyPressed(KEY_E)) {
        m_bShowEMD = !m_bShowEMD;
    }

    if (IsKeyDown(KEY_MINUS) || IsKeyDown(KEY_KP_SUBTRACT)) {
        Params::visDelayPerStep = std::min(MAX_DELAY_PER_STEP, Params::visDelayPerStep+5);
    }

    if (IsKeyDown(KEY_EQUAL) || IsKeyDown(KEY_KP_ADD)) {
        Params::visDelayPerStep = std::max(0, Params::visDelayPerStep-5);
    }

    // Camera zoom controls
    // Uses log scaling to provide consistent zoom speed
    m_camera.zoom = expf(logf(m_camera.zoom) + ((float)GetMouseWheelMove()*0.1f));

    if (m_camera.zoom > 3.0f) m_camera.zoom = 3.0f;
    else if (m_camera.zoom < 0.1f) m_camera.zoom = 0.1f;

    // Camera reset
    if (IsKeyPressed(KEY_R)) {
        m_camera.zoom = 1.0f;
        m_displayOffset = {0.0f, 0.0f};
    }

    if (IsKeyDown(KEY_UP)) {
        m_displayOffset.y += 10.0f / m_camera.zoom;
    }
    if (IsKeyDown(KEY_DOWN)) {
        m_displayOffset.y -= 10.0f / m_camera.zoom;
    }
    if (IsKeyDown(KEY_LEFT)) {
        m_displayOffset.x -= 10.0f / m_camera.zoom;
    }
    if (IsKeyDown(KEY_RIGHT)) {
        m_displayOffset.x += 10.0f / m_camera.zoom;
    }
}


void LocalVis::continueUntilClosed()
{
    m_bWaitingForUserToClose = true;
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        updateDrawFrame();
    }
}


void LocalVis::rotateDrawState()
{
    switch (m_drawState) {
    case DrawState::BEES:
        m_drawState = DrawState::BEES_AND_HEATMAP;
        break;
    case DrawState::BEES_AND_HEATMAP:
        m_drawState = DrawState::HEATMAP;
        break;
    case DrawState::HEATMAP:
        m_drawState = DrawState::BEES;
        break;
    default:
        pb::msg_error_and_exit("LocalVis::rotateDrawState(): invalid draw state");
    }
}


void LocalVis::drawBees()
{
    // draw hives
    for (const HiveSpec& hiveSpec : Params::hiveSpecs) {
        DrawRectangleLinesEx(
            envToDisplayRect({ hiveSpec.x-HALF_HIVE_SIZE, hiveSpec.y-HALF_HIVE_SIZE, HIVE_SIZE, HIVE_SIZE }),
            4.0f, GOLD);
    }

    // draw bees
    for (const Bee& bee : m_pPolyBeeCore->getBees()) {
        std::vector<Vector2> BeeShapeAbs = BEE_SHAPE;
        for (Vector2& v : BeeShapeAbs) {
            v = Vector2Scale(Vector2Rotate(v, bee.angle), BEE_SCALING_FACTOR);
            v.x += envToDisplayX(bee.x);
            v.y += envToDisplayY(bee.y);
        }

        //DrawTriangle(BeeShapeAbs[0], BeeShapeAbs[1], BeeShapeAbs[2], LIME);
        DrawTriangle(BeeShapeAbs[0], BeeShapeAbs[1], BeeShapeAbs[2], ColorFromHSV(bee.colorHue, 0.7f, 0.9f));

        if (m_bShowTrails) {
            size_t pathIdxMax = bee.path.size()-1;
            int drawCount = 0;

            size_t i = pathIdxMax;
            Vector2 p1 = { envToDisplayX(bee.path[i].x), envToDisplayY(bee.path[i].y) };
            Vector2 p2 = { envToDisplayX(bee.x), envToDisplayY(bee.y) };
            DrawLineEx(p1, p2, BEE_PATH_THICKNESS, ColorAlpha(ColorFromHSV(bee.colorHue, 0.3f, 0.7f), 1.0f));
            ++drawCount;

            for (; i >= 1 && drawCount < Params::visBeePathDrawLen; --i) {
                Vector2 p1 = { envToDisplayX(bee.path[i-1].x), envToDisplayY(bee.path[i-1].y) };
                Vector2 p2 = { envToDisplayX(bee.path[i].x), envToDisplayY(bee.path[i].y) };
                float alpha = 1.0f - ((pathIdxMax - static_cast<float>(i)) / Params::visBeePathDrawLen); // fade out older parts of path
                DrawLineEx(p1, p2, BEE_PATH_THICKNESS, ColorAlpha(ColorFromHSV(bee.colorHue, 0.3f, 0.7f), alpha));
                ++drawCount;
            }
        }
    }
}


void LocalVis::drawHeatmap()
{
    const Heatmap& heatmap = m_pPolyBeeCore->getHeatmap();
    if (!heatmap.isNormalisedCalculated()) {
        DrawText("Normalised heatmap not available!", 100, 100, 20, RAYWHITE);
        return;
    }

    int numCellsX = heatmap.size_x();
    int numCellsY = heatmap.size_y();
    int numCells = numCellsX * numCellsY;
    int cellW = envToDisplayN(Params::envW) / numCellsX;
    int cellH = envToDisplayN(Params::envH) / numCellsY;

    // Helper lambda to convert normalized value [0,1] to blue-red heatmap color
    auto getHeatmapColor = [](float normalized) -> Color {
        // Clamp to [0, 1] range
        if (normalized < 0.0f) normalized = 0.0f;
        if (normalized > 1.0f) normalized = 1.0f;

        // Blue to red color mapping through cyan, green, yellow
        // Blue (0): RGB(0, 0, 255)
        // Cyan (0.25): RGB(0, 255, 255)
        // Green (0.5): RGB(0, 255, 0)
        // Yellow (0.75): RGB(255, 255, 0)
        // Red (1): RGB(255, 0, 0)

        unsigned char r, g, b;

        if (normalized < 0.25f) {
            // Blue to Cyan
            float t = normalized / 0.25f;
            r = 0;
            g = static_cast<unsigned char>(255 * t);
            b = 255;
        } else if (normalized < 0.5f) {
            // Cyan to Green
            float t = (normalized - 0.25f) / 0.25f;
            r = 0;
            g = 255;
            b = static_cast<unsigned char>(255 * (1.0f - t));
        } else if (normalized < 0.75f) {
            // Green to Yellow
            float t = (normalized - 0.5f) / 0.25f;
            r = static_cast<unsigned char>(255 * t);
            g = 255;
            b = 0;
        } else {
            // Yellow to Red
            float t = (normalized - 0.75f) / 0.25f;
            r = 255;
            g = static_cast<unsigned char>(255 * (1.0f - t));
            b = 0;
        }

        return Color{r, g, b, 128}; // Semi-transparent for overlay
    };

    // Draw heatmap cells using normalized values
    const auto& cellsNormalised = heatmap.cellsNormalised();
    for (int x = 0; x < numCellsX; ++x) {
        for (int y = 0; y < numCellsY; ++y) {
            float normalizedValue = cellsNormalised[x][y];
            float valueToPlot = normalizedValue * (numCells / 3.0); // scale for better visibility
            Color color = getHeatmapColor(valueToPlot);

            DrawRectangle(DISPLAY_MARGIN_LEFT + x * cellW, DISPLAY_MARGIN_TOP + y * cellH, cellW, cellH, color);

            // Draw cell borders for better visibility
            DrawRectangleLines(DISPLAY_MARGIN_LEFT + x * cellW, DISPLAY_MARGIN_TOP + y * cellH, cellW, cellH, DARKGRAY);
        }
    }

    if (m_bShowEMD && !m_bPaused && !m_bWaitingForUserToClose)
    {
        auto start = std::chrono::high_resolution_clock::now();
        m_currentEMD = heatmap.emd(heatmap.uniformTargetNormalised());
        auto end = std::chrono::high_resolution_clock::now();
        m_currentEMDTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }

    /*
    // Draw color scale legend
    int legendX = 20;
    int legendY = 50;
    int legendWidth = 200;
    int legendHeight = 20;

    DrawText("Normalized Heatmap Scale:", legendX, legendY - 25, 16, RAYWHITE);

    // Draw gradient legend
    for (int i = 0; i < legendWidth; ++i) {
        float t = static_cast<float>(i) / legendWidth;
        Color color = getHeatmapColor(t);
        DrawRectangle(legendX + i, legendY, 1, legendHeight, color);
    }

    // Legend labels
    DrawText("0.0", legendX, legendY + legendHeight + 5, 12, RAYWHITE);
    DrawText("1.0", legendX + legendWidth - 20, legendY + legendHeight + 5, 12, RAYWHITE);
    */
}