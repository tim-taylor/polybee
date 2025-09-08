/**
 * @file
 *
 * Declaration of the LocalVis class
 */

#ifndef _LOCALVIS_H
#define _LOCALVIS_H

class PolyBee;

/**
 * The LocalVis class ...
 */
class LocalVis {

public:
    LocalVis(PolyBee* PolyBee);
    ~LocalVis();

    void run();

    void updateDrawFrame();

private:
    PolyBee* m_pPolyBee;
};

#endif /* _LOCALVIS_H */
