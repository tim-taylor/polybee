/**
 * @file
 *
 * Implementation of the Bee class
 */

#include "Bee.h"
#include "Environment.h"
#include "PolyBeeCore.h"
#include "Params.h"
#include <numbers>
#include <random>
#include <cassert>
#include <format>
#include <algorithm>

// initialise static members
const float Bee::m_sTunnelWallBuffer {0.1f};


// Bee methods

Bee::Bee(pb::Pos2D pos, Environment* pEnv) :
    m_x(pos.x), m_y(pos.y), m_angle(0.0), m_pEnv(pEnv)
{
    commonInit();
}

Bee::Bee(pb::Pos2D pos, float angle, Environment* pEnv) :
    m_x(pos.x), m_y(pos.y), m_angle(angle), m_pEnv(pEnv)
{
    commonInit();
}

void Bee::commonInit() {
    assert(Params::initialised());
    m_distDir.param(std::uniform_real_distribution<float>::param_type(-Params::beeMaxDirDelta, Params::beeMaxDirDelta));
    m_colorHue = PolyBeeCore::m_sUniformProbDistrib(PolyBeeCore::m_sRngEngine) * 360.0f;
    m_inTunnel = m_pEnv->inTunnel(m_x, m_y);
}

void Bee::move() {
    // Temp code just for sanity check of final position, tested at end of method
    /*
    float oldx = x;
    float oldy = y;
    */

    // record current position
    m_path.emplace_back(m_x, m_y);
    if (m_path.size() > Params::beePathRecordLen) {
        m_path.erase(m_path.begin());
    }

    // work out where bee would like to go next, following forage for the nearest flower strategy
    // or step in random direction if no nearby flowers found
    // TODO - remove the if..else scaffold and old code when flower foraging properly implemented
    pb::PosAndDir2D desiredMove;
    if (true) {
        desiredMove = forageNearestFlower();
    }
    else {
        // decide on a direction in which to try to move
        //m_angle += m_distDir(PolyBeeCore::m_sRngEngine);
        desiredMove.angle = m_angle + m_distDir(PolyBeeCore::m_sRngEngine);

        // work out where that would take us
        desiredMove.x = m_x + Params::beeStepLength * std::cos(desiredMove.angle);
        desiredMove.y = m_y + Params::beeStepLength * std::sin(desiredMove.angle);
    }

     // keep within bounds of environment
    if (desiredMove.x < 0.0f) desiredMove.x = 0.0f;
    if (desiredMove.y < 0.0f) desiredMove.y = 0.0f;
    if (desiredMove.x > Params::envW) desiredMove.x = Params::envW;
    if (desiredMove.y > Params::envH) desiredMove.y = Params::envH;

    // check if new position is valid
    bool newPosInTunnel = m_pEnv->inTunnel(desiredMove.x, desiredMove.y);
    if ((m_inTunnel && newPosInTunnel) || (!m_inTunnel && !newPosInTunnel)) {
        // valid move: update position
        m_angle = desiredMove.angle;
        m_x = desiredMove.x;
        m_y = desiredMove.y;
    }
    else {
        // bee has crossed tunnel boundary, so we need to figure out if it can enter/exit the tunnel at this point
        auto intersectInfo = m_pEnv->getTunnel().intersectsEntrance(m_x, m_y, desiredMove.x, desiredMove.y);

        if (!intersectInfo.intersects) {
            pb::msg_error_and_exit(
                std::format("Bee::move(): logic error: expected intersection when crossing tunnel boundary, from ({}, {}) to ({}, {})",
                    m_x, m_y, desiredMove.x, desiredMove.y));
        }
        else if (intersectInfo.withinLimits) {
            // the bee passed through an entrance, so it can move to the new position
            m_angle = desiredMove.angle;
            m_x = desiredMove.x;
            m_y = desiredMove.y;
            m_inTunnel = !m_inTunnel;
        }
        else {
            // the bee collided with the tunnel wall, so it cannot move where it wanted to go.
            // Instead, set its position to the point where it hit the wall
            m_angle = desiredMove.angle; // TODO should maybe recalculate actual angle here?
            m_x = intersectInfo.point.x;
            m_y = intersectInfo.point.y;
        }
    }

    // Temp code for sanity check of final position, tested at end of method
    /*
    float newx_prenudge = x;
    float newy_prenudge = y;
    */

    // nudge bee away from tunnel walls if too close, to avoid any numerical issues
    nudgeAwayFromTunnelWalls();

    // Temp code - sanity check
    /*
    float distanceMovedSq = (x - oldx) * (x - oldx) + (y - oldy) * (y - oldy);
    static bool maxStepLengthCalculated = false;
    static float maxStepLengthSq = 0.0f;
    if (!maxStepLengthCalculated) {
        maxStepLengthSq = (Params::beeStepLength * 1.2f) * (Params::beeStepLength * 1.2f);
        maxStepLengthCalculated = true;
    }
    if (distanceMovedSq > maxStepLengthSq) {
        pb::msg_error_and_exit(
            std::format("Bee::move(): logic error: bee moved further than allowed step length, from "
                "({}, {}) to ({}, {}) nudged to ({}, {})",
                oldx, oldy, newx_prenudge, newy_prenudge, x, y));
    }
    */
}


void Bee::nudgeAwayFromTunnelWalls()
{
    auto& tunnel = m_pEnv->getTunnel();

    if (m_inTunnel) {
        // bee is inside the tunnel: ensure it stays a minimum distance away from the walls

        // check left/right walls
        if (m_x <= tunnel.x() + m_sTunnelWallBuffer) {
            m_x = tunnel.x() + m_sTunnelWallBuffer;
        }
        else if (m_x >= tunnel.x() + tunnel.width() - m_sTunnelWallBuffer) {
            m_x = tunnel.x() + tunnel.width() - m_sTunnelWallBuffer;
        }
        // check top/bottom walls
        if (m_y <= tunnel.y() + m_sTunnelWallBuffer) {
            m_y = tunnel.y() + m_sTunnelWallBuffer;
        }
        else if (m_y >= tunnel.y() + tunnel.height() - m_sTunnelWallBuffer) {
            m_y = tunnel.y() + tunnel.height() - m_sTunnelWallBuffer;
        }
    }
    else {
        // bee is outside the tunnel: check if it's within the wall buffer zone, and nudge it out further if so

        // check left/right walls
        if ((m_y >= tunnel.y()) && (m_y <= tunnel.y() + tunnel.height())) {
            // bee is within the vertical range of the tunnel, so check horizontal distance
            if ((m_x < tunnel.x() + tunnel.width() / 2.0f) && (m_x >= tunnel.x() - m_sTunnelWallBuffer)) {
                m_x = tunnel.x() - m_sTunnelWallBuffer;
            }
            else if ((m_x >= tunnel.x() + tunnel.width() / 2.0f) && (m_x <= tunnel.x() + tunnel.width() + m_sTunnelWallBuffer)) {
                m_x = tunnel.x() + tunnel.width() + m_sTunnelWallBuffer;
            }
        }

        // check top/bottom walls
        if ((m_x >= tunnel.x()) && (m_x <= tunnel.x() + tunnel.width())) {
            // bee is within the horizontal range of the tunnel, so check vertical distance
            if ((m_y < tunnel.y() + tunnel.height() / 2.0f) && (m_y >= tunnel.y() - m_sTunnelWallBuffer)) {
                m_y = tunnel.y() - m_sTunnelWallBuffer;
            }
            else if ((m_y >= tunnel.y() + tunnel.height() / 2.0f) && (m_y <= tunnel.y() + tunnel.height() + m_sTunnelWallBuffer)) {
                m_y = tunnel.y() + tunnel.height() + m_sTunnelWallBuffer;
            }
        }
    }
}


// Calculate where the bee would like to go next, following forage for nearest flower strategy
// or step in random direction if no nearby flowers found.
// Returns the new (x,y) position as a Pos2D struct, but does not actually update the bee's position.
// The calling method is responsible for checking if the new position is valid (e.g. that it is
// within the bounds of the environment and that it doesn't cross a tunnel wall except at defined
// entrance/exit points), and updating the bee's position if appropriate.
//
// Note, this method is not const because it uses m_distDir which updates its internal state
// each time it is called.
//
pb::PosAndDir2D Bee::forageNearestFlower()
{
    pb::PosAndDir2D result;

    auto plantInfo = m_pEnv->getNearestUnvisitedPlant(m_x, m_y, m_recentlyVisitedPlants);
    if (plantInfo.has_value()) {
        Plant* pPlant = plantInfo.value();
        // found a nearby plant to forage from
        float dx = pPlant->x() - m_x;
        float dy = pPlant->y() - m_y;
        float angleToPlant = std::atan2(dy, dx); // angle from bee to plant

        result.angle = angleToPlant;

        float distToPlantSq = (dx * dx + dy * dy);
        if (distToPlantSq <= (Params::beeStepLength * Params::beeStepLength)) {
            // plant is within one step length, so just go directly to it
            result.x = pPlant->x();
            result.y = pPlant->y();

            // record that we've visited this plant (and remove oldest visited plant if necessary
            // to keep memory length within limit)
            pPlant->setVisited();
            addToRecentlyVisitedPlants(pPlant);
        }
        else {
            // plant is further away than one step length, so just head in its direction
            result.x = m_x + Params::beeStepLength * std::cos(angleToPlant);
            result.y = m_y + Params::beeStepLength * std::sin(angleToPlant);
        }
    }
    else {
        // no nearby plants found, so just move in random direction
        result.angle = m_angle + m_distDir(PolyBeeCore::m_sRngEngine);
        result.x = m_x + Params::beeStepLength * std::cos(result.angle);
        result.y = m_y + Params::beeStepLength * std::sin(result.angle);
    }

    return result;
}


void Bee::addToRecentlyVisitedPlants(Plant* pPlant)
{
    // check if plant is already in recently visited list
    auto it = std::find(m_recentlyVisitedPlants.begin(), m_recentlyVisitedPlants.end(), pPlant);
    if (it != m_recentlyVisitedPlants.end()) {
        // TODO - this situation should not happen! - we should be able to remove this
        // test and just have the else clause below
        pb::msg_error_and_exit(
            std::format("Bee::addToRecentlyVisitedPlants(): logic error: plant at ({}, {}) already in recently visited list",
                pPlant->x(), pPlant->y()));
        // plant already in list, so move it to the back (most recently visited)
        m_recentlyVisitedPlants.erase(it);
        m_recentlyVisitedPlants.push_back(pPlant);
    }
    else {
        // plant not in list, so just add it to the back
        m_recentlyVisitedPlants.push_back(pPlant);
        if (m_recentlyVisitedPlants.size() > Params::beeVisitMemoryLength) {
            m_recentlyVisitedPlants.erase(m_recentlyVisitedPlants.begin());
        }
    }
}
