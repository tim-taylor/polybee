/**
 * @file
 *
 * Declaration of the Environment class
 */

#ifndef _ENVIRONMENT_H
#define _ENVIRONMENT_H

#include "Bee.h"
#include "Hive.h"
#include "Tunnel.h"
#include "Heatmap.h"
#include <vector>

/**
 * The Environment class ...
 */
class Environment {

public:
    Environment();
    ~Environment() {}

    void initialise();
    void update();
    void reset(); // reset environment to initial state

    const Heatmap& getHeatmap() const { return m_heatmap; }
    const std::vector<Bee>& getBees() const { return m_bees; }

private:
    void initialiseTunnel();
    void initialiseHives();
    void initialiseBees();
    void initialiseHeatmap();
    void resetBees();

    std::vector<Bee> m_bees;
    std::vector<Hive> m_hives;
    Tunnel m_tunnel;
    Heatmap m_heatmap;
};

#endif /* _ENVIRONMENT_H */
