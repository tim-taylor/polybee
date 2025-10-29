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


Bee::Bee(fPos pos, Environment* pEnv) :
    x(pos.x), y(pos.y), angle(0.0), m_pEnv(pEnv)
{
    commonInit();
}

Bee::Bee(fPos pos, float angle, Environment* pEnv) :
    x(pos.x), y(pos.y), angle(angle), m_pEnv(pEnv)
{
    commonInit();
}

void Bee::commonInit() {
    assert(Params::initialised());
    m_distDir.param(std::uniform_real_distribution<float>::param_type(-Params::beeMaxDirDelta, Params::beeMaxDirDelta));
    colorHue = PolyBeeCore::m_sUniformProbDistrib(PolyBeeCore::m_sRngEngine) * 360.0f;
    inTunnel = m_pEnv->inTunnel(x, y);
}

void Bee::move() {
    // record current position
    path.emplace_back(x, y);
    if (path.size() > Params::beePathRecordLen) {
        path.erase(path.begin());
    }

    //bool isInTunnel = m_pEnv->inTunnel(x, y);

    // decide on a direction in which to try to move
    angle += m_distDir(PolyBeeCore::m_sRngEngine);

    // work out where that would take us
    float newx = x + Params::beeStepLength * std::cos(angle);
    float newy = y + Params::beeStepLength * std::sin(angle);

    // keep within bounds of environment
    if (newx < 0.0f) newx = 0.0f;
    if (newy < 0.0f) newy = 0.0f;
    if (newx > Params::envW) newx = Params::envW;
    if (newy > Params::envH) newy = Params::envH;

    // check if new position is valid
    bool newPosInTunnel = m_pEnv->inTunnel(newx, newy);
    if ((inTunnel && newPosInTunnel) || (!inTunnel && !newPosInTunnel)) {
        // valid move: update position
        x = newx;
        y = newy;
    }
    else {
        // bee has crossed tunnel boundary, so we need to figure out if it can enter/exit the tunnel at this point
        auto intersectInfo = m_pEnv->getTunnel().intersectsEntrance(x, y, newx, newy);

        if (!intersectInfo.intersects) {
            assert(intersectInfo.intersects); // should always be true here
            pb::msg_error_and_exit("Bee::move(): logic error: expected intersection when crossing tunnel boundary.");
        }
        else if (intersectInfo.withinLimits) {
            // the bee passed through an entrance, so it can move to the new position
            x = newx;
            y = newy;
            inTunnel = !inTunnel;
        }
        else {
            // the bee collided with the tunnel wall, so it cannot move where it wanted to go.
            // Instead, set its position to the point where it hit the wall
            x = intersectInfo.point.x;
            y = intersectInfo.point.y;
        }
    }

    // nudge bee away from tunnel walls if too close, to avoid any numerical issues
    nudgeAwayFromTunnelWalls();
}


void Bee::nudgeAwayFromTunnelWalls()
{
    auto& tunnel = m_pEnv->getTunnel();
    float wallBuffer = 0.5f; // minimum distance to keep from tunnel walls

    if (inTunnel) {
        // check left/right walls
        if (x <= tunnel.x() + wallBuffer) {
            x = tunnel.x() + wallBuffer;
        }
        else if (x >= tunnel.x() + tunnel.width() - wallBuffer) {
            x = tunnel.x() + tunnel.width() - wallBuffer;
        }
        // check top/bottom walls
        if (y <= tunnel.y() + wallBuffer) {
            y = tunnel.y() + wallBuffer;
        }
        else if (y >= tunnel.y() + tunnel.height() - wallBuffer) {
            y = tunnel.y() + tunnel.height() - wallBuffer;
        }
    }
    else {
        if (y >= tunnel.y() - wallBuffer && y <= tunnel.y() + tunnel.height() + wallBuffer) {
            // bee is outside the tunnel but within the wall buffer zone, so nudge it out further
            if (x < tunnel.x() + tunnel.width() / 2.0f && x >= tunnel.x() - wallBuffer) {
                x = tunnel.x() - wallBuffer;
            }
            else if (x >= tunnel.x() + tunnel.width() / 2.0f && x <= tunnel.x() + tunnel.width() + wallBuffer) {
                x = tunnel.x() + tunnel.width() + wallBuffer;
            }
        }

        if (x >= tunnel.x() - wallBuffer && x <= tunnel.x() + tunnel.width() + wallBuffer) {
            // bee is outside the tunnel but within the wall buffer zone, so nudge it out further
            if (y < tunnel.y() + tunnel.height() / 2.0f && y >= tunnel.y() - wallBuffer) {
                y = tunnel.y() - wallBuffer;
            }
            else if (y >= tunnel.y() + tunnel.height() / 2.0f && y <= tunnel.y() + tunnel.height() + wallBuffer) {
                y = tunnel.y() + tunnel.height() + wallBuffer;
            }
        }
    }
}

