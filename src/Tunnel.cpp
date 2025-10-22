/**
 * @file
 *
 * Implementation of the Tunnel class
 */

#include "Tunnel.h"

Tunnel::Tunnel() {
}

void Tunnel::initialise(float x, float y, float width, float height) {
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
    TunnelEntranceSpec specCopy = spec;
    // ensure e1 <= e2
    if (specCopy.e2 < specCopy.e1) {
        float tmp = specCopy.e1;
        specCopy.e1 = specCopy.e2;
        specCopy.e2 = tmp;
    }
    m_entrances.push_back(specCopy);
}
