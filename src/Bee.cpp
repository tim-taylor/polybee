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

    m_pPolyBeeCore = m_pEnv->getPolyBeeCore();

    m_pos = m_pHive->pos();
    m_distDir.param(std::uniform_real_distribution<float>::param_type(-Params::beeMaxDirDelta, Params::beeMaxDirDelta));
    m_colorHue = m_pPolyBeeCore->m_uniformProbDistrib(m_pPolyBeeCore->m_rngEngine) * 360.0f;
    m_inTunnel = m_pEnv->inTunnel(m_pos.x, m_pos.y);
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
    case 4: m_angle = m_pPolyBeeCore->m_angle2PiDistrib(m_pPolyBeeCore->m_rngEngine); break; // Random
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

    if (m_tryingToCrossEntrance) {
        // bee is in processes of trying to cross a tunnel entrance and will keep trying until
        // it either succeeds or gives up
        continueTryingToCrossEntrance();
    }
    else {
        // normal foraging update
        bool stillForaging = normalForagingUpdate();
        if (!stillForaging) {
            return; // bee's state has changed, so we can exit right now
        }
    }

    m_currentBoutDuration++;

    /*
    // TODO - is this bit now obsolete?
    // do we still want a fixed max foraging bout duration, or just have bees return to hive when they run out of energy?
    if (m_currentBoutDuration >= Params::beeForageDuration) {
        // maximum foraging bout duration reached, so return to hive
        switchToReturnToHive();
    }
    */

    // deplete bee's energy level
    m_energy -= Params::beeEnergyDepletionPerStep;

    if (m_energy <= Params::beeEnergyMinThreshold || m_energy >= Params::beeEnergyMaxThreshold) {
        // bee has either run out of energy, or collected as much as it wants. Either way, return to hive!
        switchToReturnToHive();
    }
}


// Normal foraging update when not trying to cross tunnel entrance.
// Returns true if bee is still foraging, false if its state has changed (e.g. it has landed on a flower).
//
bool Bee::normalForagingUpdate()
{
    // Normal foraging update when not trying to cross tunnel entrance
    pb::PosAndDir2D desiredMove;

    // work out where bee would like to go next, following "forage nearest flower" strategy
    // or step in random direction if no nearby flowers found
    auto desiredMoveOpt = forageNearestFlower();

    if (desiredMoveOpt.has_value()) {
        float rnd = m_pPolyBeeCore->m_uniformProbDistrib(m_pPolyBeeCore->m_rngEngine);
        if (rnd < Params::beeProbVisitNearestFlower) {
            // move towards nearest unvisited flower
            desiredMove = desiredMoveOpt.value();
        }
        else {
            // move in random direction with a fixed step length
            // (N.B. we are not yet considering Levy flights here)
            desiredMove = moveInRandomDirection();
        }
    }
    else {
        // no nearby unvisited flowers found, so move in random direction with a fixed step length
        // (N.B. we are not yet considering Levy flights here)
        desiredMove = moveInRandomDirection();
    }

    // keep within bounds of environment
    keepMoveWithinEnvironment(desiredMove);

    // check if new position is valid
    bool newPosInTunnel = m_pEnv->inTunnel(desiredMove.x, desiredMove.y);
    if ((m_inTunnel && newPosInTunnel) || (!m_inTunnel && !newPosInTunnel)) {
        // valid move: update position
        m_angle = desiredMove.angle;
        m_pos.x = desiredMove.x;
        m_pos.y = desiredMove.y;
        if (m_state == BeeState::ON_FLOWER) {
            // bee has just landed on its target flower. Its state has already been updated in forageNearestFlower(),
            // so its bout has successfully finished and our work here is done
            return false; // bee is no longer foraging
        }
    }
    else {
        // desired move crosses tunnel boundary, so we need to figure out if bee can enter/exit the tunnel at this point
        attemptToCrossTunnelBoundaryWhileForaging(desiredMove);
    }

    // nudge bee away from tunnel walls if too close, to avoid any numerical issues
    nudgeAwayFromTunnelWalls();

    return true; // bee is still foraging
}


// Called when a bee's desired move while foraging crosses tunnel boundary. In this
// situation, we need to figure out if the bee can enter/exit the tunnel at this
// location and at this time, based upon whether there is a tunnel entrance at this
// location, and the net type at that entrance.
//
// * If it is trying to cross a tunnel entrance and is successful, we update its
//   position and state accordingly.
//
// * If it is trying to cross a tunnel entrance but fails (due to net), it remains
//   at its current position and we set m_tryingToCrossEntrance to true and
//   m_tryCrossState.failCount to 1. This will cause the bee to continue
//   trying to cross the entrance in subsequent updates until it either succeeds
//   or gives up after a maximum number of attempts. The logic for this is handled
//   in the forage() method.
//
// * If the bee collides with a tunnel wall, it cannot move to the desired position
//   so we ajust its position to be at the point of intersection with the tunnel
//   boundary, and align its direction along the wall.
//
void Bee::attemptToCrossTunnelBoundaryWhileForaging(pb::PosAndDir2D& desiredMove)
{
    assert(m_state == BeeState::FORAGING);

    auto intersectInfo = m_pEnv->getTunnel().intersectsTunnelBoundary(m_pos.x, m_pos.y, desiredMove.x, desiredMove.y);

    if (!intersectInfo.intersects) {
        pb::msg_error_and_exit(
            std::format("Bee::move(): logic error: expected intersection when crossing tunnel boundary, from ({}, {}) to ({}, {})",
                m_pos.x, m_pos.y, desiredMove.x, desiredMove.y));
    }
    else if (intersectInfo.crossesEntrance) {
        // bee wants to enter/exit via a tunnel entrance, so we need to determine whether it can
        // successfully do so based on the net type at this entrance

        float rnd = m_pPolyBeeCore->m_uniformProbDistrib(m_pPolyBeeCore->m_rngEngine);
        float probExit = intersectInfo.pEntranceUsed->probExit();
        if (rnd < probExit) {
            // the bee passed through an entrance, so it can move to the new position
            m_angle = desiredMove.angle;
            m_pos.x = desiredMove.x;
            m_pos.y = desiredMove.y;
            m_inTunnel = !m_inTunnel;
            // NB store pointer to last tunnel entrance used. Alternatively, we might want to record the
            // FIRST entrance used when entering the tunnel, so that the bee can try to exit via the same entrance later?
            // TODO - discuss with Alan and Hazel
            m_pLastTunnelEntrance = intersectInfo.pEntranceUsed;
        }
        else {
            // the bee failed to exit through the net, so we assume it rebounded to its current
            // position (i.e. it remains at its current position). We set m_tryingToCrossEntrance to true and
            // reinitialise m_tryCrossState, so that it will continue trying to cross the entrance
            // in subsequent updates until it either succeeds or gives up after a maximum number of attempts.
            m_tryingToCrossEntrance = true;
            m_tryCrossState.set(m_pos, intersectInfo, &(m_pEnv->getTunnel()));
        }
    }
    else {
        // the bee collided with the tunnel wall, so it cannot move where it wanted to go.
        // Instead, set its position to the point where it hit the wall

        m_pos = intersectInfo.point;

        // and align its direction to be along the wall, in the direction closest to its desired movement direction
        float dx = intersectInfo.intersectedLine.end.x - intersectInfo.intersectedLine.start.x;
        float dy = intersectInfo.intersectedLine.end.y - intersectInfo.intersectedLine.start.y;
        m_angle = alignAngleWithLine(desiredMove.angle, dx, dy);
    }
}


// This method is called when a bee has previously attempted to cross a tunnel entrance but failed
// (due to net), and is continuing to try to cross the entrance in subsequent updates until
// it either succeeds or gives up after a maximum number of attempts.
//
void Bee::continueTryingToCrossEntrance()
{
    assert(m_tryingToCrossEntrance);

    if (m_tryCrossState.currentlyAtNetPos) {
        // deal with case when we're just moving back from net to a rebound position calculated
        // based upon current position (i.e. we're in the "rebound" part of the logic)

        // First, we move from our current location at the net a little distance parallel to the wall
        // in a random direction (either way along the wall with equal probability), to give us a new starting point
        // for our rebound.
        float sideStepMax = Params::beeStepLength * 0.2f;
        float sideStep = sideStepMax - (m_pPolyBeeCore->m_uniformProbDistrib(m_pPolyBeeCore->m_rngEngine) * 2.0f * sideStepMax);
        pb::Pos2D newReboundStartPos = m_pos.moveAlongLine(*(m_tryCrossState.pWallLine), sideStep, true);

        // Next we move perpendicular to the wall by the rebound length. We need to make sure we move in the right
        // direction (i.e. towards the outside of the tunnel if we're trying to get in, and towards the inside of the
        // tunnel if we're trying to get out).
        pb::Pos2D reboundDir = m_tryCrossState.normalUnitVector.multiply(m_inTunnel ? -1.0f : 1.0f); // if we're trying to get out of the tunnel, we want

        // Now actually move the bee having calculated where it should go
        m_pos = newReboundStartPos.add(reboundDir.multiply(m_tryCrossState.reboundLen));

        // And update the state of the crossing attempts, and we're done
        m_tryCrossState.update();
    }
    else {
        // deal with case when we're back from net and moving towards it again
        // (i.e. we're in the "approach" part of the logic)

        // Calculate desired move towards and across boundary.
        // We do this by moving perpendicular to the wall by the cross length, which takes us directly
        // across the wall and little bit further.
        // We eed to make sure we move in the right direction (i.e. towards the net from the outside, and
        // away from the net from the inside). We can work this out based on whether the bee is currently
        // in the tunnel or not, and modify the normal vector accordingly.
        pb::Pos2D desiredMoveDir = m_tryCrossState.normalUnitVector.multiply(m_inTunnel ? 1.0 : -1.0f); // if we're in the tunnel, we want to move in the direction of the normal vector, to try to get out. If we're outside the tunnel, we want to move in the opposite direction of the normal vector, to try to get in.);
        pb::Pos2D desiredMove = m_pos.add(desiredMoveDir.multiply(m_tryCrossState.crossLen));

        // Now work out the consequences of the desired move, i.e. whether the bee would intersect with the
        // tunnel boundary as it tries to move there.
        auto intersectInfo = m_pEnv->getTunnel().intersectsTunnelBoundary(m_pos.x, m_pos.y, desiredMove.x, desiredMove.y);

        // And deal with the consequences depending on whether it intersects with the tunnel boundary, and whether
        // it crosses an entrance or just hits the wall
        if (!intersectInfo.intersects) {
            pb::msg_error_and_exit(
                std::format("Bee::continueTryingToCrossEntrance(): logic error: expected intersection when crossing tunnel boundary, from ({}, {}) to ({}, {})",
                    m_pos.x, m_pos.y, desiredMove.x, desiredMove.y));
        }
        else if (intersectInfo.crossesEntrance) {
            // bee wants to enter/exit via a tunnel entrance, so we need to determine whether it can
            // successfullly do so based on the net type at this entrance

            float rnd = m_pPolyBeeCore->m_uniformProbDistrib(m_pPolyBeeCore->m_rngEngine);
            float probExit = intersectInfo.pEntranceUsed->probExit();
            if (rnd < probExit) {
                // the bee passed through an entrance, so it can move to the new position
                m_angle = desiredMoveDir.angle();
                m_pos.x = desiredMove.x;
                m_pos.y = desiredMove.y;
                m_inTunnel = !m_inTunnel;
                m_pLastTunnelEntrance = intersectInfo.pEntranceUsed;
                // and we can now revert to normal foraging mode
                unsetTryingToCrossEntranceState();
            }
            else {
                // the bee failed to exit through the net, so we assume it rebounded to its current
                // position (i.e. it remains where it is). So we just need to update the state of the
                // crossing attempts, and we're done
                m_tryCrossState.update();
            }
        }
        else {
            // The bee collided with the tunnel wall, so it cannot move where it wanted to go.
            // So we keep the bee where it is and give up trying to cross the entrance and
            // revert to normal foraging mode
            unsetTryingToCrossEntranceState();
        }
    }

    // If we're still trying to cross the entrance, check if we've exceeded the maximum number of attempts
    if (m_tryingToCrossEntrance) {
        if (m_tryCrossState.moveCount >= m_tryCrossState.maxCount) {
            // Bee has exceeded maximum number of attempts to cross this entrance, so give up and
            // revert to normal foraging mode
            unsetTryingToCrossEntranceState();
        }
    }
}


// Calculate the angle of the line defined by (line_dx, line_dy) in the direction along
// the line that is closest to desiredAngle. This is used, for example, when a bee hits a
// tunnel wall and we want to align its direction along the wall.
//
float Bee::alignAngleWithLine(float desiredAngle, float line_dx, float line_dy) const
{
    float lineAngle = std::atan2(line_dy, line_dx);

    // Use dot product to determine which direction along the line is closer to desiredAngle
    float dotProduct = line_dx * std::cos(desiredAngle) + line_dy * std::sin(desiredAngle);

    if (dotProduct >= 0.0f) {
        return lineAngle;
    } else {
        return lineAngle + std::numbers::pi_v<float>;
    }
}


// Check whether the desired move would take the bee outside the environment bounds,
// and if so, adjust the position to be on the edge and align the angle along the edge.
//
// Note, this method modifies the desiredMove parameter in place.
//
void Bee::keepMoveWithinEnvironment(pb::PosAndDir2D& desiredMove)
{
    pb::Pos2D LR(0,1);
    pb::Pos2D TB(1,0);

    if (desiredMove.x < 0.0f) {
        // off left edge
        desiredMove.x = 0.0f;
        desiredMove.angle = alignAngleWithLine(desiredMove.angle, LR.x, LR.y);
    }
    else if (desiredMove.x > Params::envW) {
        // off right edge
        desiredMove.x = Params::envW;
        desiredMove.angle = alignAngleWithLine(desiredMove.angle, LR.x, LR.y);
    }

    if (desiredMove.y < 0.0f) {
        // off top edge
        desiredMove.y = 0.0f;
        desiredMove.angle = alignAngleWithLine(desiredMove.angle, TB.x, TB.y);
    }
    else if (desiredMove.y > Params::envH) {
        // off bottom edge
        desiredMove.y = Params::envH;
        desiredMove.angle = alignAngleWithLine(desiredMove.angle, TB.x, TB.y);
    }
}


// To avoid any problems with numerical precision when bees are close to tunnel walls,
// nudge them away from the walls if they are within a small buffer distance.
//
void Bee::nudgeAwayFromTunnelWalls()
{
    auto& tunnel = m_pEnv->getTunnel();

    if (m_inTunnel) {
        // bee is inside the tunnel: ensure it stays a minimum distance away from the walls

        // check left/right walls
        if (m_pos.x <= tunnel.x() + m_sTunnelWallBuffer) {
            m_pos.x = tunnel.x() + m_sTunnelWallBuffer;
        }
        else if (m_pos.x >= tunnel.x() + tunnel.width() - m_sTunnelWallBuffer) {
            m_pos.x = tunnel.x() + tunnel.width() - m_sTunnelWallBuffer;
        }
        // check top/bottom walls
        if (m_pos.y <= tunnel.y() + m_sTunnelWallBuffer) {
            m_pos.y = tunnel.y() + m_sTunnelWallBuffer;
        }
        else if (m_pos.y >= tunnel.y() + tunnel.height() - m_sTunnelWallBuffer) {
            m_pos.y = tunnel.y() + tunnel.height() - m_sTunnelWallBuffer;
        }
    }
    else {
        // bee is outside the tunnel: check if it's within the wall buffer zone, and nudge it out further if so

        // check left/right walls
        if ((m_pos.y >= tunnel.y()) && (m_pos.y <= tunnel.y() + tunnel.height())) {
            // bee is within the vertical range of the tunnel, so check horizontal distance
            if ((m_pos.x < tunnel.x() + tunnel.width() / 2.0f) && (m_pos.x >= tunnel.x() - m_sTunnelWallBuffer)) {
                m_pos.x = tunnel.x() - m_sTunnelWallBuffer;
            }
            else if ((m_pos.x >= tunnel.x() + tunnel.width() / 2.0f) && (m_pos.x <= tunnel.x() + tunnel.width() + m_sTunnelWallBuffer)) {
                m_pos.x = tunnel.x() + tunnel.width() + m_sTunnelWallBuffer;
            }
        }

        // check top/bottom walls
        if ((m_pos.x >= tunnel.x()) && (m_pos.x <= tunnel.x() + tunnel.width())) {
            // bee is within the horizontal range of the tunnel, so check vertical distance
            if ((m_pos.y < tunnel.y() + tunnel.height() / 2.0f) && (m_pos.y >= tunnel.y() - m_sTunnelWallBuffer)) {
                m_pos.y = tunnel.y() - m_sTunnelWallBuffer;
            }
            else if ((m_pos.y >= tunnel.y() + tunnel.height() / 2.0f) && (m_pos.y <= tunnel.y() + tunnel.height() + m_sTunnelWallBuffer)) {
                m_pos.y = tunnel.y() + tunnel.height() + m_sTunnelWallBuffer;
            }
        }
    }
}


// Search for the bee's next target flower following a "forage nearest flower" strategy.
// If a nearby unvisited flower is found, return a new position moving towards that flower,
// otherwise, return std::nullopt. It's up to the calling method in Bee::forage() to decide what to do
// if no flower is found -- it will move the bee in a random direction.
//
// NB Returns the new (x,y) position as a Pos2D struct, but does not actually update the bee's position.
// The calling method is responsible for checking if the new position is valid (e.g. that it is
// within the bounds of the environment and that it doesn't cross a tunnel wall except at defined
// entrance/exit points), and updating the bee's position if appropriate.
//
// Note, if the bee reaches the target flower, this method will also switch the bee's state to ON_FLOWER.
// The calling code can check the bee's state after calling this method to see if that happened.
//
// Note, this method is not const because it uses m_distDir which updates its internal state
// each time it is called.
//
std::optional<pb::PosAndDir2D> Bee::forageNearestFlower()
{
    pb::PosAndDir2D result;

    auto plantInfo = m_pEnv->selectNearbyUnvisitedPlant(m_pos.x, m_pos.y, m_recentlyVisitedPlants);

    if (plantInfo.has_value()) {
        Plant* pPlant = plantInfo.value();
        // found a nearby plant to forage from
        float dx = pPlant->x() - m_pos.x;
        float dy = pPlant->y() - m_pos.y;
        float angleToPlant = std::atan2(dy, dx); // angle from bee to plant

        result.angle = angleToPlant;

        float distToPlantSq = (dx * dx + dy * dy);
        if (distToPlantSq <= (Params::beeStepLength * Params::beeStepLength)) {
            // plant is within one step length, so just go directly to it
            result.x = pPlant->x();
            result.y = pPlant->y();

            // record that we've visited this plant (and remove oldest visited plant if necessary
            // to keep memory length within limit)
            pPlant->incrementVisitCount();
            addToRecentlyVisitedPlants(pPlant);

            // and switch to ON_FLOWER state
            switchToOnFlower(pPlant);
        }
        else {
            // plant is further away than one step length, so just head in its direction
            result.x = m_pos.x + Params::beeStepLength * std::cos(angleToPlant);
            result.y = m_pos.y + Params::beeStepLength * std::sin(angleToPlant);
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

    result.angle = m_angle + m_distDir(m_pPolyBeeCore->m_rngEngine);
    result.x = m_pos.x + Params::beeStepLength * std::cos(result.angle);
    result.y = m_pos.y + Params::beeStepLength * std::sin(result.angle);

    return result;
}


// record current position in path history, trimming to maximum length if necessary
void Bee::updatePathHistory()
{
    // add current position to path
    m_path.push_back(m_pos);

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
    if (m_currentFlowerDuration >= Params::beeOnFlowerDuration) {
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
    unsetTryingToCrossEntranceState();

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


void Bee::unsetTryingToCrossEntranceState()
{
    m_tryingToCrossEntrance = false;
    m_tryCrossState.reset();
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
// Does NOT remove the waypoint from the list even if it is reached - that is the caller's
// responsibility if this method returned true.
//
bool Bee::headToNextWaypoint()
{
    assert(!m_homingWaypoints.empty());

    bool reachedWaypoint = false;
    float stepLength = 1.0f;

    const pb::Pos2D& nextWaypoint = m_homingWaypoints.front();
    pb::Pos2D moveVector = pb::Pos2D(nextWaypoint.x - m_pos.x, nextWaypoint.y - m_pos.y);
    float distToWaypoint = moveVector.length();

    // Set orientation to face the waypoint
    if (distToWaypoint > FLOAT_COMPARISON_EPSILON) {
        m_angle = std::atan2(moveVector.y, moveVector.x);
    }

    if (distToWaypoint <= Params::beeStepLength) {
        // We've reached a waypoint.
        // We need to figure out it it's a tunnel entrance waypoint - if it is, we need to consider
        // the probability that the bee can pass through the net
        if (nextWaypointIsTunnelEntrance())
        {
            assert(m_pLastTunnelEntrance != nullptr);
            float rnd = m_pPolyBeeCore->m_uniformProbDistrib(m_pPolyBeeCore->m_rngEngine);
            // NB we assume that m_pLastTunnelEntrance refers to the same entrance as the waypoint being
            // targeted - this is ensured by the logic in calculateWaypointsAroundTunnel() and
            // calculateWaypointsInsideTunnel()
            float probExit = m_pLastTunnelEntrance->probExit();
            if (rnd < probExit) {
                // the bee passed through an entrance, so it can move to the waypoint
                reachedWaypoint = true;
                stepLength = distToWaypoint;
            }
            else {
                // the bee failed to exit through the net, so we assume it rebounded to its current
                // position (i.e. it remains at its current position)
                // (we could consider adding a time cost here for the failed exit attempt)
                stepLength = 0.0f;
            }
        }
        else {
            // The waypoint that we have reached is not a tunnel entrance, so we can just move
            // directly to it without jitter
            reachedWaypoint = true;
            stepLength = distToWaypoint;
        }
    }
    else {
        // We are not at a waypoint yet, so move towards the next one with full step length
        stepLength = Params::beeStepLength;
    }

    moveVector.resize(stepLength);

    if ((!reachedWaypoint) && (stepLength > FLOAT_COMPARISON_EPSILON)) {
        std::normal_distribution<float> distJitter(0.0f, 0.1f * stepLength);
        moveVector.x += distJitter(m_pPolyBeeCore->m_rngEngine);
        moveVector.y += distJitter(m_pPolyBeeCore->m_rngEngine);
    }

    m_pos.x += moveVector.x;
    m_pos.y += moveVector.y;

    return reachedWaypoint;
}


// Check whether the next waypoint in m_homingWaypoints is a tunnel entrance.
// Assumptions:
// - a tunnel entrance waypoint can only be the last waypoint in the list
// - if bee if in tunnel and hive is outside tunnel, the last waypoint in the current list
//   is always a tunnel entrance
// - if bee is outside tunnel and hive is inside tunnel, the last waypoint in the current list
//   is always a tunnel entrance
// - otherwise, the last waypoint is never a tunnel entrance
//
bool Bee::nextWaypointIsTunnelEntrance() const
{
    // Are we at the last waypoint in the list? If not, it can't be a tunnel entrance
    if (m_homingWaypoints.size() != 1) {
        return false;
    }

    // If bee is in tunnel and hive is outside, or vice versa, last waypoint must be tunnel entrance
    if (m_inTunnel != m_pHive->inTunnel()) {
        return true;
    }

    // Otherwise, last waypoint is not a tunnel entrance
    return false;
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
        m_homingWaypoints.push_back(m_pHive->pos());
        return;
    }

    // If we get to this point, the hive is outside the tunnel.
    // Set a single waypoint at the last tunnel entrance used to exit the tunnel
    assert(m_pLastTunnelEntrance != nullptr);

    pb::Pos2D entranceCentre(
        ((m_pLastTunnelEntrance->x1 + m_pLastTunnelEntrance->x2) / 2.0f),
        ((m_pLastTunnelEntrance->y1 + m_pLastTunnelEntrance->y2) / 2.0f)
    );

    pb::Pos2D vecToEntrance { entranceCentre.x - m_pos.x, entranceCentre.y - m_pos.y };

    float distToEntrance = vecToEntrance.length();
    float bufferDist = Bee::m_sTunnelWallBuffer ; // distance to stay outside tunnel entrance when exiting
    float relDist = (distToEntrance + bufferDist) / distToEntrance;

    pb::Pos2D waypoint {m_pos.x + vecToEntrance.x * relDist, m_pos.y + vecToEntrance.y * relDist};

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
    if (!lineIntersectsTunnel(m_pos.x, m_pos.y, endPointX, endPointY)) {
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
        float prevX = m_pos.x;
        float prevY = m_pos.y;
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
        float prevX = m_pos.x;
        float prevY = m_pos.y;

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

