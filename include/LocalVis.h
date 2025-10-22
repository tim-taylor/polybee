/**
 * @file
 *
 * Declaration of the LocalVis class
 */

#ifndef _LOCALVIS_H
#define _LOCALVIS_H

#include "raylib.h"

class PolyBeeCore;

enum class DrawState {
    BEES,
    BEES_AND_HEATMAP,
    HEATMAP
};

/**
 * The LocalVis class ...
 */
class LocalVis {

public:
    LocalVis(PolyBeeCore* PolyBeeCore);
    ~LocalVis();

    void updateDrawFrame();
    void continueUntilClosed();

    bool showBees() const { return m_drawState == DrawState::BEES || m_drawState == DrawState::BEES_AND_HEATMAP; }
    bool showHeatmap() const { return m_drawState == DrawState::HEATMAP || m_drawState == DrawState::BEES_AND_HEATMAP; }

    float envToDisplayX(float envX) const;
    float envToDisplayY(float envY) const;
    float envToDisplayN(float displayN) const;
    Rectangle envToDisplayRect(const Rectangle& rect) const;

private:
    void drawBees();
    void drawHeatmap();
    void drawTunnel();
    void drawStatusText();
    void processKeyboardInput();
    void rotateDrawState();

    PolyBeeCore* m_pPolyBeeCore;
    DrawState m_drawState {DrawState::BEES_AND_HEATMAP};

    bool m_bWaitingForUserToClose {false};
    bool m_bPaused {false};
    bool m_bShowTrails {true};
    bool m_bShowEMD {true};

    Camera2D m_camera;

    float m_currentEMD;
    int64_t m_currentEMDTime;
};

#endif /* _LOCALVIS_H */
