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
#include <cmath>

// initialise static members
const float Bee::m_sTunnelWallBuffer {0.1f};


// Bee methods

Bee::Bee(Hive* pHive, Environment* pEnv) :
    m_pHive(pHive), m_pEnv(pEnv)
{
    assert(Params::initialised());

    m_x = m_pHive->x();
    m_y = m_pHive->y();
    m_distDir.param(std::uniform_real_distribution<float>::param_type(-Params::beeMaxDirDelta, Params::beeMaxDirDelta));
    m_colorHue = PolyBeeCore::m_sUniformProbDistrib(PolyBeeCore::m_sRngEngine) * 360.0f;
    m_inTunnel = m_pEnv->inTunnel(m_x, m_y);
    m_currentBoutDuration = 0;
    m_currentHiveDuration = 0;
    m_state = BeeState::FORAGING;
    setDirAccordingToHive();
    m_energy = Params::beeInitialEnergy;
}


void Bee::setDirAccordingToHive()
{
    assert(m_pHive != nullptr);
    switch (m_pHive->direction()) {
    case 0: m_angle = -std::numbers::pi_v<float> / 2.0f; break; // North
    case 1: m_angle = 0.0f; break; // East
    case 2: m_angle = std::numbers::pi_v<float> / 2.0f; break; // South
    case 3: m_angle = std::numbers::pi_v<float>; break; // West
    case 4: m_angle = PolyBeeCore::m_sAngle2PiDistrib(PolyBeeCore::m_sRngEngine);; break; // Random
    default:
        pb::msg_error_and_exit(std::format("Invalid hive direction {} specified for hive at ({},{}). Must be 0=North, 1=East, 2=South, 3=West, or 4=Random.",
            m_pHive->direction(), m_pHive->x(), m_pHive->y()));
    }
}


void Bee::update() {
    switch (m_state)
    {
    case BeeState::FORAGING:
        forage();
        break;
    case BeeState::ON_FLOWER:
        stayOnFlower();
        break;
    case BeeState::RETURN_TO_HIVE_INSIDE_TUNNEL:
        returnToHiveInsideTunnel();
        break;
    case BeeState::RETURN_TO_HIVE_OUTSIDE_TUNNEL:
        returnToHiveOutsideTunnel();
        break;
    case BeeState::IN_HIVE:
        stayInHive();
        break;
    default:
        break;
    }
}


void Bee::forage()
{
    // update bee's path record with current position
    updatePathHistory();

    pb::PosAndDir2D desiredMove;
    // work out where bee would like to go next, following "forage nearest flower" strategy
    // or step in random direction if no nearby flowers found
    auto desiredMoveOpt = forageNearestFlower();
    if (desiredMoveOpt.has_value()) {
        desiredMove = desiredMoveOpt.value();
    }
    else {
        // no nearby unvisited flowers found, so move in random direction
        desiredMove = moveInRandomDirection();
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
        if (m_state == BeeState::ON_FLOWER) {
            // bee has just landed on its target flower. Its state has already been updated in forageNearestFlower(),
            // so our work here is done
            return;
        }
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

            // NB store pointer to last tunnel entrance used. Alternatively, we might want to record the
            // FIRST entrance used when entering the tunnel, so that the bee can try to exit via the same entrance later?
            // TODO - discuss with Alan and Hazel
            m_pLastTunnelEntrance = intersectInfo.pEntranceUsed;
        }
        else {
            // the bee collided with the tunnel wall, so it cannot move where it wanted to go.
            // Instead, set its position to the point where it hit the wall
            m_angle = desiredMove.angle; // TODO should maybe recalculate actual angle here?
            m_x = intersectInfo.point.x;
            m_y = intersectInfo.point.y;
        }
    }

    // nudge bee away from tunnel walls if too close, to avoid any numerical issues
    nudgeAwayFromTunnelWalls();

    m_currentBoutDuration++;

    // TODO - is this bit now obsolete?
    // do we still want a fixed max foraging bout duration, or just have bees return to hive when they run out of energy?
    if (m_currentBoutDuration >= Params::beeForageDuration) {
        // maximum foraging bout duration reached, so return to hive
        switchToReturnToHive();
    }

    m_energy -= Params::beeEnergyDepletionPerStep;
    if (m_energy <= 0.0f) {
        // bee has run out of energy, so return to hive
        switchToReturnToHive();
    }
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


// Search for the bee's next target flower following a "forage nearest flower" strategy.
// If a nearby unvisited flower is found, return a new position moving towards that flower,
// otherwise, return std::nullopt.
//
// NB Returns the new (x,y) position as a Pos2D struct, but does not actually update the bee's position.
// The calling method is responsible for checking if the new position is valid (e.g. that it is
// within the bounds of the environment and that it doesn't cross a tunnel wall except at defined
// entrance/exit points), and updating the bee's position if appropriate.
//
// Note, this method is not const because it uses m_distDir which updates its internal state
// each time it is called.
//
std::optional<pb::PosAndDir2D> Bee::forageNearestFlower()
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

            // and switch to ON_FLOWER state
            switchToOnFlower(pPlant);
        }
        else {
            // plant is further away than one step length, so just head in its direction
            result.x = m_x + Params::beeStepLength * std::cos(angleToPlant);
            result.y = m_y + Params::beeStepLength * std::sin(angleToPlant);
        }

        return result;
    }
    else {
        // no unvisited nearby plants found
        return std::nullopt;
    }
}


// Calculate a new position by moving in a random direction from the bee's current position.
pb::PosAndDir2D Bee::moveInRandomDirection()
{
    pb::PosAndDir2D result;

    result.angle = m_angle + m_distDir(PolyBeeCore::m_sRngEngine);
    result.x = m_x + Params::beeStepLength * std::cos(result.angle);
    result.y = m_y + Params::beeStepLength * std::sin(result.angle);

    return result;
}


// record current position in path history, trimming to maximum length if necessary
void Bee::updatePathHistory()
{
    // add current position to path
    m_path.emplace_back(m_x, m_y);

    // trim path to maximum length
    if (m_path.size() > static_cast<size_t>(Params::beePathRecordLen)) {
        m_path.erase(m_path.begin());
    }
}


// Add the given plant to the bee's recently visited plants list.
// If the list exceeds the maximum length, remove the oldest entry.
//
void Bee::addToRecentlyVisitedPlants(Plant* pPlant)
{
    m_recentlyVisitedPlants.push_back(pPlant);
    if (m_recentlyVisitedPlants.size() > Params::beeVisitMemoryLength) {
        m_recentlyVisitedPlants.erase(m_recentlyVisitedPlants.begin());
    }
}


void Bee::switchToOnFlower(Plant* pPlant)
{
    m_state = BeeState::ON_FLOWER;
    m_energy += pPlant->extractNectar(Params::beeEnergyBoostPerFlower); // boost bee's energy on visiting a flower
    m_currentFlowerDuration = 0;
    // we don't reset bout duration here, as the bee is still in the same foraging bout
}


void Bee::stayOnFlower()
{
    m_currentFlowerDuration++;
    updatePathHistory();
    if (m_currentFlowerDuration >= Params::beeNumStepsOnFlower) {
        // finished resting in hive, so restart foraging
        m_state = BeeState::FORAGING;
        m_currentFlowerDuration = 0;
    }
}


void Bee::switchToReturnToHive()
{
    // clear recently visited plants list so bee can visit them again on next foraging bout
    m_recentlyVisitedPlants.clear();
    m_currentBoutDuration = 0;

    if (m_inTunnel) {
        // bee is inside tunnel, so set state to return to hive inside tunnel
        m_state = BeeState::RETURN_TO_HIVE_INSIDE_TUNNEL;
        calculateWaypointsInsideTunnel();
    }
    else {
        // bee is outside tunnel, so set state to return to hive outside tunnel
        m_state = BeeState::RETURN_TO_HIVE_OUTSIDE_TUNNEL;
        calculateWaypointsAroundTunnel();
    }
}


// Follow waypoints to return to end point while inside tunnel.
// End point is this hive is it is inside the tunnel, or the last tunnel entrance used
// to exit the tunnel if hive is outside tunnel.
void Bee::returnToHiveInsideTunnel()
{
    // update bee's path record with current position
    updatePathHistory();

    // move towards next waypoint
    bool reachedWaypoint = headToNextWaypoint();

    if (reachedWaypoint) {
        m_homingWaypoints.pop_front();
        if (m_homingWaypoints.empty()) {
            // reached final waypoint
            if (m_pHive->inTunnel()) {
                // bee is now at hive
                m_state = BeeState::IN_HIVE;
                m_inTunnel = true;
                m_currentHiveDuration = 0;
                setDirAccordingToHive();
            }
            else {
                // bee is outside tunnel, so calculate waypoints around tunnel to get to hive
                m_state = BeeState::RETURN_TO_HIVE_OUTSIDE_TUNNEL;
                calculateWaypointsAroundTunnel();
            }
        }
    }
}


// Follow waypoints to return to end point while outside tunnel.
// End point is hive if it is outside tunnel, or last tunnel entrance used to
// enter tunnel if hive is inside tunnel.
void Bee::returnToHiveOutsideTunnel()
{
    // update bee's path record with current position
    updatePathHistory();

    // move towards next waypoint
    bool reachedWaypoint = headToNextWaypoint();

    if (reachedWaypoint) {
        m_homingWaypoints.pop_front();
        if (m_homingWaypoints.empty()) {
            // reached final waypoint
            if (m_pHive->inTunnel()) {
                // we've been following waypoints outside the tunnel, but the hive is inside the tunnel,
                // so now calculate waypoints inside tunnel to get to hive
                m_state = BeeState::RETURN_TO_HIVE_INSIDE_TUNNEL;
                calculateWaypointsInsideTunnel();
            }
            else {
                // we've been following waypoints outside the tunnel, and the hive is also outside the tunnel,
                // so we're now at the hive
                m_state = BeeState::IN_HIVE;
                m_inTunnel = false;
                m_currentHiveDuration = 0;
                setDirAccordingToHive();
            }
        }
    }
}


// Head to next waypoint in m_homingWaypoints (the front of the deque).
// If we're not at the final waypoint in the list, then add a little bit of jitter to each step.
// Returns true if the waypoint has been reached, false otherwise.
bool Bee::headToNextWaypoint()
{
    bool reachedWaypoint = false;
    float stepLength = 1.0f;

    const pb::Pos2D& nextWaypoint = m_homingWaypoints.front();
    pb::Pos2D moveVector = pb::Pos2D(nextWaypoint.x - m_x, nextWaypoint.y - m_y);
    float distToWaypoint = moveVector.length();

    if (distToWaypoint <= Params::beeStepLength) {
        reachedWaypoint = true;
        stepLength = distToWaypoint;
    }
    else {
        stepLength = Params::beeStepLength;
    }

    moveVector.resize(stepLength);

    if (!reachedWaypoint) {
        std::normal_distribution<float> distJitter(0.0f, 0.1f * stepLength);
        moveVector.x += distJitter(PolyBeeCore::m_sRngEngine);
        moveVector.y += distJitter(PolyBeeCore::m_sRngEngine);
    }

    m_x += moveVector.x;
    m_y += moveVector.y;

    return reachedWaypoint;
}


// Stay in hive for required duration, then switch to foraging state
void Bee::stayInHive()
{
    m_currentHiveDuration++;
    updatePathHistory();
    if (m_currentHiveDuration >= Params::beeInHiveDuration) {
        // finished resting in hive, so start a new foraging bout
        m_state = BeeState::FORAGING;
        m_currentHiveDuration = 0;
        m_currentBoutDuration = 0;
        m_energy = Params::beeInitialEnergy;
    }
}


// Calculate waypoints to end point when bee is inside tunnel.
// The end point is the hive if it is inside the tunnel, or the last tunnel entrance used
// to enter the tunnel if the hive is outside the tunnel.
void Bee::calculateWaypointsInsideTunnel()
{
    m_homingWaypoints.clear();

    if (m_pHive->inTunnel()) {
        // hive is inside tunnel - just set single waypoint straight to hive
        m_homingWaypoints.push_back(pb::Pos2D(m_pHive->x(), m_pHive->y()));
        return;
    }

    // If e get to this point, the hive is outside the tunnel.
    // Set a single waypoint at the last tunnel entrance used to exit the tunnel
    assert(m_pLastTunnelEntrance != nullptr);

    pb::Pos2D entranceCentre(
        ((m_pLastTunnelEntrance->x1 + m_pLastTunnelEntrance->x2) / 2.0f),
        ((m_pLastTunnelEntrance->y1 + m_pLastTunnelEntrance->y2) / 2.0f)
    );

    pb::Pos2D vecToEntrance { entranceCentre.x - m_x, entranceCentre.y - m_y };

    float distToEntrance = vecToEntrance.length();
    float bufferDist = Bee::m_sTunnelWallBuffer ; // distance to stay outside tunnel entrance when exiting
    float relDist = (distToEntrance + bufferDist) / distToEntrance;

    pb::Pos2D waypoint {m_x + vecToEntrance.x * relDist, m_y + vecToEntrance.y * relDist};

    m_homingWaypoints.push_back(waypoint);
}


// Calculate waypoints to navigate around the tunnel from current position to end point.
// The end point is the hive if it is outside the tunnel, or the last tunnel entrance used
// to enter the tunnel if the hive is inside the tunnel.
void Bee::calculateWaypointsAroundTunnel()
{
    m_homingWaypoints.clear();

    float endPointX, endPointY;

    if (m_pHive->inTunnel()) {
        // TODO should we nudge the end point a little inside the tunnel entrance?
        endPointX = m_pLastTunnelEntrance->x1 + (m_pLastTunnelEntrance->x2 - m_pLastTunnelEntrance->x1) / 2.0f;
        endPointY = m_pLastTunnelEntrance->y1 + (m_pLastTunnelEntrance->y2 - m_pLastTunnelEntrance->y1) / 2.0f;
    }
    else {
        endPointX = m_pHive->x();
        endPointY = m_pHive->y();
    }

    auto& tunnel = m_pEnv->getTunnel();
    float tx = tunnel.x();
    float ty = tunnel.y();
    float tw = tunnel.width();
    float th = tunnel.height();
    float buffer = m_sTunnelWallBuffer;

    // Check if direct path to end point intersects tunnel
    if (!lineIntersectsTunnel(m_x, m_y, endPointX, endPointY)) {
        // Direct path is clear, just go straight to end point
        m_homingWaypoints.push_back(pb::Pos2D(endPointX, endPointY));
        return;
    }

    // Direct path blocked, need to navigate around tunnel
    // Define the four corner waypoints (with buffer distance applied outward)
    pb::Pos2D topLeft(tx - buffer, ty - buffer);
    pb::Pos2D topRight(tx + tw + buffer, ty - buffer);
    pb::Pos2D bottomLeft(tx - buffer, ty + th + buffer);
    pb::Pos2D bottomRight(tx + tw + buffer, ty + th + buffer);

    // Determine which quadrant the bee is in relative to tunnel center
    float centerX = tx + tw / 2.0f;
    float centerY = ty + th / 2.0f;

    // Calculate which corners to use based on bee and hive positions
    // We'll evaluate different paths and choose the shortest

    struct PathOption {
        std::deque<pb::Pos2D> waypoints;
        float totalDistance;
    };

    std::vector<PathOption> pathOptions;

    // Helper to calculate distance between two points
    auto distance = [](float x1, float y1, float x2, float y2) -> float {
        float dx = x2 - x1;
        float dy = y2 - y1;
        return std::sqrt(dx * dx + dy * dy);
    };

    // Helper to calculate total path distance
    auto calculatePathDistance = [&](const std::deque<pb::Pos2D>& waypoints) -> float {
        float total = 0.0f;
        float prevX = m_x;
        float prevY = m_y;
        for (const auto& wp : waypoints) {
            total += distance(prevX, prevY, wp.x, wp.y);
            prevX = wp.x;
            prevY = wp.y;
        }
        return total;
    };

    // Try different routing options:
    // 1. Via top-left corner
    pathOptions.push_back({{topLeft, pb::Pos2D(endPointX, endPointY)}, 0.0f});

    // 2. Via top-right corner
    pathOptions.push_back({{topRight, pb::Pos2D(endPointX, endPointY)}, 0.0f});

    // 3. Via bottom-left corner
    pathOptions.push_back({{bottomLeft, pb::Pos2D(endPointX, endPointY)}, 0.0f});

    // 4. Via bottom-right corner
    pathOptions.push_back({{bottomRight, pb::Pos2D(endPointX, endPointY)}, 0.0f});

    // 5. Via top-left then top-right
    pathOptions.push_back({{topLeft, topRight, pb::Pos2D(endPointX, endPointY)}, 0.0f});

    // 6. Via top-right then bottom-right
    pathOptions.push_back({{topRight, bottomRight, pb::Pos2D(endPointX, endPointY)}, 0.0f});

    // 7. Via bottom-right then bottom-left
    pathOptions.push_back({{bottomRight, bottomLeft, pb::Pos2D(endPointX, endPointY)}, 0.0f});

    // 8. Via bottom-left then top-left
    pathOptions.push_back({{bottomLeft, topLeft, pb::Pos2D(endPointX, endPointY)}, 0.0f});

    // Calculate distances and filter out paths that still intersect the tunnel
    std::vector<PathOption> validPaths;
    for (auto& option : pathOptions) {
        bool valid = true;
        float prevX = m_x;
        float prevY = m_y;

        // Check each segment of the path
        for (const auto& wp : option.waypoints) {
            if (lineIntersectsTunnel(prevX, prevY, wp.x, wp.y)) {
                valid = false;
                break;
            }
            prevX = wp.x;
            prevY = wp.y;
        }

        if (valid) {
            option.totalDistance = calculatePathDistance(option.waypoints);
            validPaths.push_back(option);
        }
    }

    // Choose the shortest valid path
    if (!validPaths.empty()) {
        auto shortest = std::min_element(validPaths.begin(), validPaths.end(),
            [](const PathOption& a, const PathOption& b) {
                return a.totalDistance < b.totalDistance;
            });

        m_homingWaypoints = shortest->waypoints;
    }
    else {
        // Fallback: if no valid path found (shouldn't happen), just add end point as waypoint
        // and let the collision detection handle it
        m_homingWaypoints.push_back(pb::Pos2D(endPointX, endPointY));
    }
}


// Check if a line segment from (x1, y1) to (x2, y2) intersects the tunnel rectangle
// (excluding entrances - this is a simple geometric check)
bool Bee::lineIntersectsTunnel(float x1, float y1, float x2, float y2) const
{
    auto& tunnel = m_pEnv->getTunnel();
    float tx = tunnel.x();
    float ty = tunnel.y();
    float tw = tunnel.width();
    float th = tunnel.height();

    // Check if either endpoint is inside the tunnel
    if ((x1 >= tx && x1 <= tx + tw && y1 >= ty && y1 <= ty + th) ||
        (x2 >= tx && x2 <= tx + tw && y2 >= ty && y2 <= ty + th)) {
        return true;
    }

    // Check if line segment intersects any of the four tunnel edges
    // Using parametric line intersection test for each edge

    auto lineSegmentIntersect = [](float p1x, float p1y, float p2x, float p2y,
                                    float p3x, float p3y, float p4x, float p4y) -> bool {
        float d = (p2x - p1x) * (p4y - p3y) - (p2y - p1y) * (p4x - p3x);
        if (std::abs(d) < FLOAT_COMPARISON_EPSILON) return false; // parallel

        float t = ((p3x - p1x) * (p4y - p3y) - (p3y - p1y) * (p4x - p3x)) / d;
        float u = ((p3x - p1x) * (p2y - p1y) - (p3y - p1y) * (p2x - p1x)) / d;

        return (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f);
    };

    // Check against top edge
    if (lineSegmentIntersect(x1, y1, x2, y2, tx, ty, tx + tw, ty)) return true;
    // Check against bottom edge
    if (lineSegmentIntersect(x1, y1, x2, y2, tx, ty + th, tx + tw, ty + th)) return true;
    // Check against left edge
    if (lineSegmentIntersect(x1, y1, x2, y2, tx, ty, tx, ty + th)) return true;
    // Check against right edge
    if (lineSegmentIntersect(x1, y1, x2, y2, tx + tw, ty, tx + tw, ty + th)) return true;

    return false;
}

