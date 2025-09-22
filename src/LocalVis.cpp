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
    // TO DO
    DrawText("HISTOGRAM COMING SOON!", 100, 100, 20, RAYWHITE);

    int numCellsX = m_pPolyBeeCore->m_heatmap.size_x();
    int numCellsY = m_pPolyBeeCore->m_heatmap.size_y();
    int cellW = (Params::envW * Params::visCellSize) / numCellsX;
    int cellH = (Params::envH * Params::visCellSize) / numCellsY;

    int cellSize = Params::visCellSize; // for now, just use

    for (int x=0; x < m_pPolyBeeCore->m_heatmap.size_x(); ++x) {
        for (int y=0; y < m_pPolyBeeCore->m_heatmap.size_y(); ++y) {

            float value = m_pPolyBeeCore->m_heatmap.cells()[x][y];
            Color color = Color(value, value, value, 128);

            //float value = m_pPolyBeeCore->m_heatmap.cellsNormalised()[x][y];
            //Color color = ColorAlpha(ColorFromHSV(value * 360, 1.0f, 1.0f), 0.5f);

            DrawRectangle(ENV_MARGIN + x * cellW, ENV_MARGIN +  y * cellH, cellW, cellH, color);
        }
    }
}