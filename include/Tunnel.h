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

    void initialise(float x, float y, float width, float height);
    void addEntrance(const TunnelEntranceSpec& spec);

    float x() const { return m_x; }
    float y() const { return m_y; }
    float width() const { return m_width; }
    float height() const { return m_height; }
    float thickness() const { return m_thickness; }
    const std::vector<TunnelEntranceSpec>& getEntrances() const { return m_entrances; }

private:
    float m_x;                  // top-left x position of tunnel in environment coordinates
    float m_y;                  // top-left y position of tunnel in environment coordinates
    float m_width;              // width of the tunnel
    float m_height;             // height of the tunnel
    float m_thickness {0.5f};   // thickness of the tunnel walls
    std::vector<TunnelEntranceSpec> m_entrances;
};

#endif /* _TUNNEL_H */
