/**
 * @file
 *
 * Implementation of the Tunnel class
 */

#include "Tunnel.h"

Tunnel::Tunnel() {
}

void Tunnel::initialise(int x, int y, int width, int height) {
    m_x = x;
    m_y = y;
    m_width = width;
    m_height = height;

    auto numEntrances = Params::tunnelEntranceSpecs.size();
    for (int i = 0; i < numEntrances; ++i) {
        const TunnelEntranceSpec& spec = Params::tunnelEntranceSpecs[i];
        addEntrance(spec);
    }
}

void Tunnel::addEntrance(const TunnelEntranceSpec& spec) {
    m_entrances.push_back(spec);
}
