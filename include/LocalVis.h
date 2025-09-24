/**
 * @file
 *
 * Declaration of the LocalVis class
 */

#ifndef _LOCALVIS_H
#define _LOCALVIS_H

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

private:
    void drawBees();
    void drawHistogram();
    void rotateDrawState();

    PolyBeeCore* m_pPolyBeeCore;
    DrawState m_drawState {DrawState::BEES_AND_HEATMAP};

    bool m_bWaitingForUserToClose {false};
    bool m_bPaused {false};
    bool m_bShowTrails {true};
};

#endif /* _LOCALVIS_H */
