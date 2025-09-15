/**
 * @file
 *
 * Declaration of the Bee class
 */

#ifndef _BEE_H
#define _BEE_H

#include "utils.h"
#include <random>
#include <vector>

class Environment;

/**
 * The Bee class ...
 */
class Bee {

public:
    Bee(fPos pos, Environment* pEnv);
    Bee(fPos pos, float angle, Environment* pEnv);
    ~Bee() {}

    void move();

    float x;
    float y;
    float angle; // direction of travel in radians
    float colorHue; // hue value for coloring the bee in visualisation (between 0.0 and 360.0)

    std::vector<fPos> path; // record of the path taken by the bee

private:
    void commonInit();

    Environment* m_pEnv;
    std::uniform_real_distribution<float> m_distDir;
};

#endif /* _BEE_H */
