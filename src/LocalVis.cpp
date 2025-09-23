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
//#include "resource_dir.h" // utility header for SearchAndSetResourceDir
#include "Params.h"
#include "PolyBeeCore.h"
#include <format>
#include <chrono>
#include <thread>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#include "LocalVis.h"

const int ENV_MARGIN = 50;
const int HIVE_SIZE = 15;
const std::vector<Vector2> BEE_SHAPE = { {10, 0}, {-6, -6}, {-6, 6} }; // triangle shape for drawing bees

LocalVis::LocalVis(PolyBeeCore* pPolyBeeCore) : m_pPolyBeeCore(pPolyBeeCore)
{
    SetTraceLogLevel(4); // Level 4 suppresses INFO msgs from RayLib

    // Create the window and OpenGL context
    InitWindow(Params::envW * Params::visCellSize + 2 * ENV_MARGIN,
        Params::envH * Params::visCellSize + 2 * ENV_MARGIN, "polybee");

    // Utility function from resource_dir.h to find the resources folder and set it as the current
    // working directory so we can load from it
    // SearchAndSetResourceDir("resources");

    // Load a texture from the resources directory
    // Texture myTexture = LoadTexture("texture.png");

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    // Tell the window to use vsync and work on high DPI displays
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);

    //SetTargetFPS(60.0); // Set our game to run at 60 frames-per-second

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
    // cleanup
    // unload our textures so it can be cleaned up
    // UnloadTexture(myTexture);

    // destroy the window and cleanup the OpenGL context
    CloseWindow();
}


void LocalVis::updateDrawFrame()
{

    if (WindowShouldClose()) //(IsKeyPressed(KEY_ESCAPE))
    {
        m_pPolyBeeCore->earlyExit();
    }

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

    // Setup the back buffer for drawing (clear color and depth buffers)
    ClearBackground(BLACK);

    // consider using the following functions if implementing zoom so we can
    // figure out which cells need to be drawn:
    //   Vector2 GetWorldToScreen2D(Vector2 position, Camera2D camera);    // Get the screen space position for a 2d camera world space position
    //   Vector2 GetScreenToWorld2D(Vector2 position, Camera2D camera);    // Get the world space position for a 2d camera screen space position
    //   Matrix GetCameraMatrix(Camera camera);                            // Get camera transform matrix (view matrix)
    //   Matrix GetCameraMatrix2D(Camera2D camera);                        // Get camera 2d transform matrix

    // draw environment boundary
    DrawRectangleLines(ENV_MARGIN, ENV_MARGIN,
        Params::envW * Params::visCellSize, Params::envH * Params::visCellSize, WHITE);

    // draw hives
    for (const HiveSpec& hiveSpec : Params::hiveSpecs) {
        DrawRectangleLines(ENV_MARGIN + static_cast<int>(hiveSpec.x * Params::visCellSize) - HIVE_SIZE / 2,
            ENV_MARGIN + static_cast<int>(hiveSpec.y * Params::visCellSize) - HIVE_SIZE / 2,
            HIVE_SIZE, HIVE_SIZE, GOLD);
    }

    // draw bees
    for (const Bee& bee : m_pPolyBeeCore->m_bees) {
        std::vector<Vector2> BeeShapeAbs = BEE_SHAPE;
        for (Vector2& v : BeeShapeAbs) {
            v = Vector2Rotate(v, bee.angle);
            v.x += ENV_MARGIN + bee.x * Params::visCellSize;
            v.y += ENV_MARGIN + bee.y * Params::visCellSize;
        }

        //DrawTriangle(BeeShapeAbs[0], BeeShapeAbs[1], BeeShapeAbs[2], LIME);
        DrawTriangle(BeeShapeAbs[0], BeeShapeAbs[1], BeeShapeAbs[2], ColorFromHSV(bee.colorHue, 0.7f, 0.9f));

        // draw path
        size_t pathIdxMax = bee.path.size()-1;
        int drawCount = 0;
        for (size_t i = pathIdxMax; i >= 1 && drawCount < Params::visBeePathDrawLen; --i) {
            Vector2 p1 = { ENV_MARGIN + bee.path[i - 1].x * Params::visCellSize,
                           ENV_MARGIN + bee.path[i - 1].y * Params::visCellSize };
            Vector2 p2 = { ENV_MARGIN + bee.path[i].x * Params::visCellSize,
                           ENV_MARGIN + bee.path[i].y * Params::visCellSize };
            float alpha = 1.0f - ((pathIdxMax - static_cast<float>(i)) / Params::visBeePathDrawLen); // fade out older parts of path
            DrawLineEx(p1, p2, Params::visBeePathThickness, ColorAlpha(ColorFromHSV(bee.colorHue, 0.3f, 0.7f), alpha));
            ++drawCount;
        }
    }

    std::string msg;
    if (m_bWaitingForUserToClose) {
        msg = std::format("Finished {} iterations. Press ESC to exit", m_pPolyBeeCore->m_iIteration);
    }
    else {
        msg = std::format("Iteration target {}. Current iteration {}", Params::numIterations, m_pPolyBeeCore->m_iIteration);
    }
    DrawText(msg.c_str(), 10, 10, 20, RAYWHITE);

    // end the frame and get ready for the next one  (display frame, poll input, etc...)
    EndDrawing();

    if (IsKeyPressed(KEY_H)) {
        m_bDrawHistogram = !m_bDrawHistogram;
    }
    if (m_bDrawHistogram) {
        drawHistogram();
    }

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


void LocalVis::drawHistogram()
{
    if (!m_pPolyBeeCore->m_heatmap.isNormalisedCalculated()) {
        DrawText("Normalised heatmap not available!", 100, 100, 20, RAYWHITE);
        return;
    }

    int numCellsX = m_pPolyBeeCore->m_heatmap.size_x();
    int numCellsY = m_pPolyBeeCore->m_heatmap.size_y();
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
    const auto& cellsNormalised = m_pPolyBeeCore->m_heatmap.cellsNormalised();
    for (int x = 0; x < numCellsX; ++x) {
        for (int y = 0; y < numCellsY; ++y) {
            float normalizedValue = cellsNormalised[x][y];
            float valueToPlot = normalizedValue * (numCells / 3.0); // scale for better visibility
            Color color = getHeatmapColor(valueToPlot);

            DrawRectangle(ENV_MARGIN + x * cellW, ENV_MARGIN + y * cellH, cellW, cellH, color);

            // Optional: Draw cell borders for better visibility
            DrawRectangleLines(ENV_MARGIN + x * cellW, ENV_MARGIN + y * cellH, cellW, cellH, DARKGRAY);
        }
    }

    // TEMP CODE TO SHOW EMD VALUE TO UNIFORM TARGET
    static std::vector<std::vector<int>> targetHeatmap;
    static std::vector<std::vector<float>> targetHeatmapNormalised;
    if (targetHeatmap.empty()) {
        int cellVal = (m_pPolyBeeCore->m_iIteration * Params::numBees) / numCells;
        float cellValNormalised = 1.0f / numCells;
        targetHeatmap.resize(numCellsX);
        targetHeatmapNormalised.resize(numCellsX);
        for (int x = 0; x < numCellsX; ++x) {
            targetHeatmap[x].resize(numCellsY, cellVal);
            targetHeatmapNormalised[x].resize(numCellsY, cellValNormalised);
        }
    }

    float emdLemonVal = m_pPolyBeeCore->m_heatmap.emd_lemon(targetHeatmap);
    float emdFullVal = m_pPolyBeeCore->m_heatmap.emd_full(targetHeatmapNormalised);
    float emdApproxVal = m_pPolyBeeCore->m_heatmap.emd_approx(targetHeatmapNormalised);
    DrawText(std::format("EMD (lemon) to uniform target: {:.4f}", emdLemonVal).c_str(), 10, 830, 20, RAYWHITE);
    DrawText(std::format("EMD (full) to uniform target: {:.4f}", emdFullVal).c_str(), 10, 850, 20, RAYWHITE);
    DrawText(std::format("EMD (approx) to uniform target: {:.4f}", emdApproxVal).c_str(), 10, 870, 20, RAYWHITE);

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