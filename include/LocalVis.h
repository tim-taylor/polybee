/**
 * @file
 *
 * Declaration of the LocalVis class
 */

#ifndef _LOCALVIS_H
#define _LOCALVIS_H

class PolyBeeCore;

/**
 * The LocalVis class ...
 */
class LocalVis {

public:
    LocalVis(PolyBeeCore* PolyBeeCore);
    ~LocalVis();

    void updateDrawFrame();
    void continueUntilClosed();

private:
    void drawHistogram();

    PolyBeeCore* m_pPolyBeeCore;
    bool m_bWaitingForUserToClose{false};
    bool m_bDrawHistogram{false};
    bool m_bPaused{false};
};

#endif /* _LOCALVIS_H */
