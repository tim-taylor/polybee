/**
 * @file
 *
 * Declaration of the Bee class
 */

#ifndef _BEE_H
#define _BEE_H

#include "utils.h"
#include "Params.h"
#include <random>
#include <vector>

class Environment;
class Plant;

/**
 * The Bee class ...
 */
class Bee {

public:
    Bee(pb::Pos2D pos, Environment* pEnv);
    Bee(pb::Pos2D pos, float angle, Environment* pEnv);
    ~Bee() {}

    void move();

    // Getters
    float x() const { return m_x; }
    float y() const { return m_y; }
    float angle() const { return m_angle; }
    static float visualRange() { return Params::beeVisualRange; }
    float colorHue() const { return m_colorHue; }
    bool inTunnel() const { return m_inTunnel; }
    const std::vector<pb::Pos2D>& path() const { return m_path; }

private:
    void commonInit();
    void nudgeAwayFromTunnelWalls();
    pb::PosAndDir2D forageNearestFlower();
    void addToRecentlyVisitedPlants(Plant* pPlant);

    float m_x;        // position of bee in environment coordinates
    float m_y;        // position of bee in environment coordinates
    float m_angle;    // direction of travel in radians
    float m_colorHue; // hue value for coloring the bee in visualisation (between 0.0 and 360.0)
    bool  m_inTunnel; // is the bee currently in the tunnel

    std::vector<Plant*> m_recentlyVisitedPlants; // the last N plants visited by the bee
    std::vector<pb::Pos2D> m_path; // record of the path taken by the bee

    Environment* m_pEnv;
    std::uniform_real_distribution<float> m_distDir;

    static const float m_sTunnelWallBuffer; // minimum distance to keep from tunnel walls
};

#endif /* _BEE_H */
