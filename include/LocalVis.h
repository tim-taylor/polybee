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
    PolyBeeCore* m_pPolyBeeCore;
};

#endif /* _LOCALVIS_H */
