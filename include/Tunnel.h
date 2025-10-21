/**
 * @file
 *
 * Declaration of the Tunnel class
 */

#ifndef _TUNNEL_H
#define _TUNNEL_H

#include "Params.h"

/**
 * The Tunnel class ...
 */
class Tunnel {

public:
    Tunnel();
    ~Tunnel() {}

    void initialise(int x, int y, int width, int height);
    void addEntrance(const TunnelEntranceSpec& spec);

private:
    int m_x;      // top-left x position of tunnel in environment coordinates
    int m_y;      // top-left y position of tunnel in environment coordinates
    int m_width;  // width of the tunnel
    int m_height; // height of the tunnel
    std::vector<TunnelEntranceSpec> m_entrances;
};

#endif /* _TUNNEL_H */
