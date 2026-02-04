/**
 * @file
 *
 * Declaration of the Bee class
 */

#ifndef _BEE_H
#define _BEE_H

#include "Tunnel.h"
#include "utils.h"
#include "Params.h"
#include <random>
#include <vector>
#include <deque>
#include <optional>

class PolyBeeCore;
class Environment;
class Hive;
class TunnelEntranceInfo;
class Plant;

enum class BeeState
{
    FORAGING,
    ON_FLOWER,
    RETURN_TO_HIVE_OUTSIDE_TUNNEL,
    RETURN_TO_HIVE_INSIDE_TUNNEL,
    IN_HIVE
};


struct TryingToCrossEntranceState
{
    void set(int failCount_, float x, float y, const IntersectInfo& intersectInfo_, const pb::PosAndDir2D& desiredMove_) {
        failCount = failCount_;
        initialPos = pb::Pos2D(x, y);
        intersectInfo = intersectInfo_;
        desiredMove = desiredMove_;
        atNetPos.x = x + 0.9 * (intersectInfo_.point.x - x);
        atNetPos.y = y + 0.9 * (intersectInfo_.point.y - y);
    }

    void reset() {
        failCount = 0;
        initialPos.setToZero();
        intersectInfo.reset();
        desiredMove.setToZero();
        atNetPos.setToZero();
    }

    int failCount { 0 };
    pb::Pos2D initialPos;
    pb::Pos2D atNetPos;
    IntersectInfo intersectInfo;
    pb::PosAndDir2D desiredMove;
};


/**
 * The Bee class ...
 */
class Bee {

public:
    //Bee(pb::Pos2D pos, float angle, Hive* pHive, Environment* pEnv);
    Bee(Hive* pHive, Environment* pEnv);
    ~Bee() {}

    void update();

    // Getters
    float x() const { return m_x; }
    float y() const { return m_y; }
    float angle() const { return m_angle; }
    static float visualRange() { return Params::beeVisualRange; }
    float colorHue() const { return m_colorHue; }
    bool inTunnel() const { return m_inTunnel; }
    const std::vector<pb::Pos2D>& path() const { return m_path; }
    BeeState state() const { return m_state; }

    // Setters
    void setState(BeeState state) { m_state = state; }

private:
    void forage();
    bool normalForagingUpdate();
    void continueTryingToCrossEntrance();
    void attemptToCrossTunnelBoundaryWhileForaging(pb::PosAndDir2D& desiredMove);
    void switchToReturnToHive();
    void switchToOnFlower(Plant* pPlant);
    void returnToHiveInsideTunnel();
    void returnToHiveOutsideTunnel();
    void stayOnFlower();
    void stayInHive();
    void keepMoveWithinEnvironment(pb::PosAndDir2D& desiredMove);
    void nudgeAwayFromTunnelWalls();
    float alignAngleWithLine(float desiredAngle, float line_dx, float line_dy) const;
    std::optional<pb::PosAndDir2D> forageNearestFlower();
    pb::PosAndDir2D moveInRandomDirection();
    void addToRecentlyVisitedPlants(Plant* pPlant);
    bool lineIntersectsTunnel(float x1, float y1, float x2, float y2) const;
    void calculateWaypointsAroundTunnel();
    void calculateWaypointsInsideTunnel();
    bool headToNextWaypoint();
    bool nextWaypointIsTunnelEntrance() const;
    void updatePathHistory();
    void setDirAccordingToHive();
    void resetTryingToCrossEntranceState();

    float m_x;          // position of bee in environment coordinates
    float m_y;          // position of bee in environment coordinates
    float m_angle;      // direction of travel in radians
    float m_energy;     // energy level of the bee
    float m_colorHue;   // hue value for coloring the bee in visualisation (between 0.0 and 360.0)
    bool  m_inTunnel;   // is the bee currently in the tunnel

    const TunnelEntranceInfo* m_pLastTunnelEntrance { nullptr }; // info about the last tunnel entrance used by the bee
    std::deque<pb::Pos2D> m_homingWaypoints; // waypoints for returning to hive

    int   m_currentBoutDuration { 0 };  // duration (number of iterations) of the current foraging bout
    int   m_currentHiveDuration { 0 };  // duration (number of iterations) of the current stay in the hive
    int   m_currentFlowerDuration { 0 };// duration (number of iterations) on current flower

    BeeState m_state { BeeState::FORAGING };

    bool m_tryingToCrossEntrance { false };
    TryingToCrossEntranceState m_tryCrossState;

    std::vector<Plant*> m_recentlyVisitedPlants; // the last N plants visited by the bee
    std::vector<pb::Pos2D> m_path;               // record of the path taken by the bee

    Hive* m_pHive { nullptr };                  // pointer to the hive the bee belongs to
    Environment* m_pEnv { nullptr };            // pointer to the environment the bee is in
    PolyBeeCore* m_pPolyBeeCore { nullptr };    // pointer to the PolyBeeCore instance
    std::uniform_real_distribution<float> m_distDir;

    static const float m_sTunnelWallBuffer;     // minimum distance to keep from tunnel walls
};

#endif /* _BEE_H */
