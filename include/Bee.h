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
#include <cassert>

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


// Struct to keep track of the state of a bee that is trying to cross a tunnel entrance
struct TryingToCrossEntranceState
{
    void set(const pb::Pos2D& pos, const IntersectInfo& intersectInfo, const Tunnel* pTunnel_);
    void update();
    void reset();

    // variables
    int moveCount { 0 };                // count of the number of moves made while trying to cross the entrance
                                        //   (each attempted cross, and each subsequent rebound counts as a move)
    int maxCount { 0 };                 // maximum number of moves backwards and forwards to attempt before giving up and reverting to normal foraging
    bool currentlyAtNetPos { false };   // whether the bee is currently at the net (having failed to cross it and about to rebound)
    float reboundLen {0.0f};            // length of the bee's rebound path (i.e. how far it rebounds back from its position near the wall)
    float crossLen {0.0f};              // length of the bee's path across the entrance
                                        //   (i.e. how far it needs to move from rebound pos to successfully cross the entrance)
    const pb::Line2D* pWallLine { nullptr }; // pointer to the line representing the wall that the bee is trying to cross
    pb::Pos2D normalUnitVector;          // unit vector perpendicular to the wall line, pointing outwards from the tunnel

private:
    float reboundToNetLen {0.0f};       // perpendicular distance from the bee's rebound position to the net line
    const Tunnel* pTunnel { nullptr };  // pointer to the tunnel
};


// Struct to record info about a bee's attempt to cross a tunnel entrance, for calculating stats on crossing attempts
struct CrossingInfo
{
    bool success {false};
    int numRebounds {0};
    NetType netType {NetType::NONE};
    int entranceID {0};
    bool isEntry {false}; // whether the crossing attempt was an attempt to enter the tunnel (as opposed to exit)

    void reset() {
        success = false;
        numRebounds = 0;
        netType = NetType::NONE;
        entranceID = 0;
        isEntry = false;
    }

    void incrementReboundCount() { numRebounds++; }
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
    float x() const { return m_pos.x; }
    float y() const { return m_pos.y; }
    float angle() const { return m_angle; }
    static float visualRange() { return Params::beeVisualRange; }
    float colorHue() const { return m_colorHue; }
    bool inTunnel() const { return m_inTunnel; }
    const std::vector<pb::Pos2D>& path() const { return m_path; }
    BeeState state() const { return m_state; }
    const TunnelEntranceInfo* entranceUsed() const { return m_pLastTunnelEntrance; }
    //const std::vector<CrossingInfo>& tunnelExitRecords() const { return m_tunnelExitRecords; }
    //const std::vector<CrossingInfo>& tunnelEntryRecords() const { return m_tunnelEntryRecords; }
    const std::vector<CrossingInfo>& entranceCrossingRecords() const { return m_entranceCrossingRecords; }

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
    void unsetTryingToCrossEntranceState();
    void recordCurrentCrossingInfo(bool success);

    pb::Pos2D m_pos;    // position of bee in environment coordinates
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

    CrossingInfo m_currentCrossingInfo;
    std::vector<CrossingInfo> m_entranceCrossingRecords;  // record of the bee's attempts to enter or exit the tunnel, for calculating stats
    //std::vector<CrossingInfo> m_tunnelEntryRecords; // record of the bee's attempts to enter the tunnel, for calculating stats

    std::vector<Plant*> m_recentlyVisitedPlants; // the last N plants visited by the bee
    std::vector<pb::Pos2D> m_path;               // record of the path taken by the bee

    Hive* m_pHive { nullptr };                  // pointer to the hive the bee belongs to
    Environment* m_pEnv { nullptr };            // pointer to the environment the bee is in
    PolyBeeCore* m_pPolyBeeCore { nullptr };    // pointer to the PolyBeeCore instance
    std::uniform_real_distribution<float> m_distDir;

    static const float m_sTunnelWallBuffer;     // minimum distance to keep from tunnel walls
};

#endif /* _BEE_H */
