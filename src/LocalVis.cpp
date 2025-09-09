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
//#include "resource_dir.h" // utility header for SearchAndSetResourceDir
#include "Params.h"
#include "PolyBee.h"

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#include "LocalVis.h"

LocalVis::LocalVis(PolyBee* pPolyBee) : m_pPolyBee(pPolyBee)
{
    SetTraceLogLevel(4); // Level 4 suppresses INFO msgs from RayLib

    // Create the window and OpenGL context
    InitWindow(Params::envW * Params::visCellSize, Params::envH * Params::visCellSize, "polybee");

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

    SetTargetFPS(Params::visTargetFPS); // Set our game to run at 60 frames-per-second

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

    //--------------------------------------------------------------------------------------

    // Main game loop
    //while (!WindowShouldClose()) // Detect window close button or ESC key
    //{
    //    updateDrawFrame();
    //}
#endif

    // cleanup
    // unload our textures so it can be cleaned up
    // UnloadTexture(myTexture);

    // destroy the window and cleanup the OpenGL context
    //CloseWindow(); - this is now done in destructor
    //return 0;
}

LocalVis::~LocalVis()
{
      // destroy the window and cleanup the OpenGL context
      CloseWindow();
}


void LocalVis::run()
{
    ////////////////////////////////////////////////////////////////////////////////////////////
    // TODO - this code borrowed from Raylib example YouTube guy

    // Initialization
	const int screenWidth = 1280;
    const int screenHeight = 768;
    const char *windowTitle = "polybee";
    const char *message = "polybee - coming very soon!";
    const int fontSize = 40;
    const float msgSpacing = 1.0f;

    //InitWindow(screenWidth, screenHeight, windowTitle);

    // NOTE: The following only works after calling InitWindow() (i.e,. RayLib is initialized)
    const Font font = GetFontDefault();
    const Vector2 msgSize = MeasureTextEx(font, message, fontSize, msgSpacing);
    const Vector2 msgPos = Vector2{(screenWidth - msgSize.x) / 2, (screenHeight - msgSize.y) / 2};

    //SetTargetFPS(60);
    ////////////////////////////////////////////////////////////////////////////////////////////


    // TODO
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        updateDrawFrame();
    }
}


//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------
void LocalVis::updateDrawFrame()
{
    // Update
    //----------------------------------------------------------------------------------
    // TODO: Update your variables here
    //----------------------------------------------------------------------------------

    if (IsKeyPressed(KEY_R))
    {
        //m_pPolyBee->initialiseCells();
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

    DrawFPS(10, 10); // display current FPS in corner of screen



    // Main loop
    //while(!WindowShouldClose()) {

        // Update the display
        //BeginDrawing();
            //ClearBackground(RAYWHITE);
            //DrawTextEx(font, message, msgPos, fontSize, msgSpacing, RED);
        DrawRectangle(100, 100, 100, 100, BLUE);
        DrawCircle(250, 250, 50, GREEN);
        DrawCircle(250, 250, 30, YELLOW);
        //EndDrawing();
    //}

    // Cleanup
    //CloseWindow();
    ////////////////////////////////////////////////////////////////////////////////////////////

    /*
    for (int y = 0; y < Params::envH; ++y)
    {
        for (int x = 0; x < Params::envW; ++x)
        {
            State state = (*(m_pPolyBee->m_pCurrentCells))[m_pPolyBee->cellIdx(x, y)];
            if (state > 0)
            {
                DrawRectangle(x * Params::visCellSize, y * Params::visCellSize, Params::visCellSize, Params::visCellSize,
                    (state == 1) ? YELLOW : GREEN);
            }
        }
    }
    */

    // end the frame and get ready for the next one  (display frame, poll input, etc...)
    EndDrawing();
    //----------------------------------------------------------------------------------

    // Post-draw update
    //----------------------------------------------------------------------------------
    // if (IsKeyPressed(KEY_ENTER)) {
    //m_pPolyBee->updateCells();
    //}
}

