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

const int DISPLAY_MARGIN_TOP = 50;
const int DISPLAY_MARGIN_BOTTOM = 50;
const int DISPLAY_MARGIN_LEFT = 50;
const int DISPLAY_MARGIN_RIGHT = 50;
const Color ENV_BACKGROUND_COLOR = { 40, 120, 40, 255 };
const Color TUNNEL_BACKGROUND_COLOR = { 70, 70, 70, 255 };

const int HIVE_SIZE = 20;
const float HALF_HIVE_SIZE = static_cast<float>(HIVE_SIZE) / 2.0f;
const std::vector<Vector2> BEE_SHAPE = { {10, 0}, {-6, -6}, {-6, 6} }; // triangle shape for drawing bees

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
    return envX * Params::visCellSize + DISPLAY_MARGIN_LEFT;
}

// ... convert a Y coordinate
float LocalVis::envToDisplayY(float envY) const {
    return envY * Params::visCellSize + DISPLAY_MARGIN_TOP;
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
                envToDisplayRect({0, 0, static_cast<float>(Params::envW), static_cast<float>(Params::envH)}),
                ENV_BACKGROUND_COLOR);
        }
        DrawRectangleLinesEx(
            envToDisplayRect({0, 0, static_cast<float>(Params::envW), static_cast<float>(Params::envH)}),
            5.0, WHITE);


        // draw tunnel rectangle and boundary
        if (!showHeatmap()) {
            DrawRectangleRec(
                envToDisplayRect({static_cast<float>(Params::tunnelX), static_cast<float>(Params::tunnelY),
                    static_cast<float>(Params::tunnelW), static_cast<float>(Params::tunnelH)}),
                TUNNEL_BACKGROUND_COLOR);
        }
        DrawRectangleLinesEx(
            envToDisplayRect({static_cast<float>(Params::tunnelX), static_cast<float>(Params::tunnelY),
                    static_cast<float>(Params::tunnelW), static_cast<float>(Params::tunnelH)}),
            5.0, WHITE);

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
    std::string msg;
    if (m_bWaitingForUserToClose) {
        msg = std::format("Finished {} iterations. Press ESC to exit", m_pPolyBeeCore->m_iIteration);
    }
    else {
        msg = std::format("Iteration target {}. Current iteration {}", Params::numIterations, m_pPolyBeeCore->m_iIteration);
    }
    DrawText(msg.c_str(), 10, 10, 20, RAYWHITE);

    if (m_bPaused) {
        DrawText("PAUSED", 10, 40, 40, RAYWHITE);
    }

    if (showHeatmap()) {
        DrawText(std::format("EMD (OpenCV) to uniform target: {:.4f} :: {} microseconds", m_currentEMD, m_currentEMDTime).c_str(),
            10, GetScreenHeight() - 30, 20, RAYWHITE);
    }

    // end the frame and get ready for the next one  (display frame, poll input, etc...)
    EndDrawing();

    // handle input
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

    // Camera zoom controls
    // Uses log scaling to provide consistent zoom speed
    m_camera.zoom = expf(logf(m_camera.zoom) + ((float)GetMouseWheelMove()*0.1f));

    if (m_camera.zoom > 3.0f) m_camera.zoom = 3.0f;
    else if (m_camera.zoom < 0.1f) m_camera.zoom = 0.1f;

    // Camera reset
    if (IsKeyPressed(KEY_R)) {
        m_camera.zoom = 1.0f;
    }

    // sleep for a short time to control frame rate if requested
    if (Params::visDelayPerStep > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(Params::visDelayPerStep));
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
    // TODO convert this method to use the envToDisplay...() methods

    // draw hives
    for (const HiveSpec& hiveSpec : Params::hiveSpecs) {
        DrawRectangleLinesEx(
            { static_cast<float>(DISPLAY_MARGIN_LEFT) + (hiveSpec.x * Params::visCellSize) - HALF_HIVE_SIZE,
              static_cast<float>(DISPLAY_MARGIN_TOP) + (hiveSpec.y * Params::visCellSize) - HALF_HIVE_SIZE,
              HIVE_SIZE, HIVE_SIZE },
            4.0,  // line thickness
            GOLD);
    }

    // draw bees
    for (const Bee& bee : m_pPolyBeeCore->getBees()) {
        std::vector<Vector2> BeeShapeAbs = BEE_SHAPE;
        for (Vector2& v : BeeShapeAbs) {
            v = Vector2Rotate(v, bee.angle);
            v.x += DISPLAY_MARGIN_LEFT + bee.x * Params::visCellSize;
            v.y += DISPLAY_MARGIN_TOP + bee.y * Params::visCellSize;
        }

        //DrawTriangle(BeeShapeAbs[0], BeeShapeAbs[1], BeeShapeAbs[2], LIME);
        DrawTriangle(BeeShapeAbs[0], BeeShapeAbs[1], BeeShapeAbs[2], ColorFromHSV(bee.colorHue, 0.7f, 0.9f));

        if (m_bShowTrails) {
            size_t pathIdxMax = bee.path.size()-1;
            int drawCount = 0;
            for (size_t i = pathIdxMax; i >= 1 && drawCount < Params::visBeePathDrawLen; --i) {
                Vector2 p1 = { DISPLAY_MARGIN_LEFT + bee.path[i - 1].x * Params::visCellSize,
                            DISPLAY_MARGIN_TOP + bee.path[i - 1].y * Params::visCellSize };
                Vector2 p2 = { DISPLAY_MARGIN_LEFT + bee.path[i].x * Params::visCellSize,
                            DISPLAY_MARGIN_TOP + bee.path[i].y * Params::visCellSize };
                float alpha = 1.0f - ((pathIdxMax - static_cast<float>(i)) / Params::visBeePathDrawLen); // fade out older parts of path
                DrawLineEx(p1, p2, Params::visBeePathThickness, ColorAlpha(ColorFromHSV(bee.colorHue, 0.3f, 0.7f), alpha));
                ++drawCount;
            }
        }
    }
}


void LocalVis::drawHeatmap()
{
    // TODO convert this method to use the envToDisplay...() methods

    const Heatmap& heatmap = m_pPolyBeeCore->getHeatmap();
    if (!heatmap.isNormalisedCalculated()) {
        DrawText("Normalised heatmap not available!", 100, 100, 20, RAYWHITE);
        return;
    }

    int numCellsX = heatmap.size_x();
    int numCellsY = heatmap.size_y();
    int numCells = numCellsX * numCellsY;
    int cellW = (Params::envW * Params::visCellSize) / numCellsX;
    int cellH = (Params::envH * Params::visCellSize) / numCellsY;

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

    if (!m_bPaused && !m_bWaitingForUserToClose)
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