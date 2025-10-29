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

// initialise static members
const float Bee::m_sTunnelWallBuffer {0.1f};


// Bee methods

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
    // Temp code just for sanity check of final position, tested at end of method
    /*
    float oldx = x;
    float oldy = y;
    */

    // record current position
    path.emplace_back(x, y);
    if (path.size() > Params::beePathRecordLen) {
        path.erase(path.begin());
    }

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
            pb::msg_error_and_exit(
                std::format("Bee::move(): logic error: expected intersection when crossing tunnel boundary, from ({}, {}) to ({}, {})",
                    x, y, newx, newy));
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

    if (inTunnel) {
        // bee is inside the tunnel: ensure it stays a minimum distance away from the walls

        // check left/right walls
        if (x <= tunnel.x() + m_sTunnelWallBuffer) {
            x = tunnel.x() + m_sTunnelWallBuffer;
        }
        else if (x >= tunnel.x() + tunnel.width() - m_sTunnelWallBuffer) {
            x = tunnel.x() + tunnel.width() - m_sTunnelWallBuffer;
        }
        // check top/bottom walls
        if (y <= tunnel.y() + m_sTunnelWallBuffer) {
            y = tunnel.y() + m_sTunnelWallBuffer;
        }
        else if (y >= tunnel.y() + tunnel.height() - m_sTunnelWallBuffer) {
            y = tunnel.y() + tunnel.height() - m_sTunnelWallBuffer;
        }
    }
    else {
        // bee is outside the tunnel: check if it's within the wall buffer zone, and nudge it out further if so

        // check left/right walls
        if ((y >= tunnel.y()) && (y <= tunnel.y() + tunnel.height())) {
            // bee is within the vertical range of the tunnel, so check horizontal distance
            if ((x < tunnel.x() + tunnel.width() / 2.0f) && (x >= tunnel.x() - m_sTunnelWallBuffer)) {
                x = tunnel.x() - m_sTunnelWallBuffer;
            }
            else if ((x >= tunnel.x() + tunnel.width() / 2.0f) && (x <= tunnel.x() + tunnel.width() + m_sTunnelWallBuffer)) {
                x = tunnel.x() + tunnel.width() + m_sTunnelWallBuffer;
            }
        }

        // check top/bottom walls
        if ((x >= tunnel.x()) && (x <= tunnel.x() + tunnel.width())) {
            // bee is within the horizontal range of the tunnel, so check vertical distance
            if ((y < tunnel.y() + tunnel.height() / 2.0f) && (y >= tunnel.y() - m_sTunnelWallBuffer)) {
                y = tunnel.y() - m_sTunnelWallBuffer;
            }
            else if ((y >= tunnel.y() + tunnel.height() / 2.0f) && (y <= tunnel.y() + tunnel.height() + m_sTunnelWallBuffer)) {
                y = tunnel.y() + tunnel.height() + m_sTunnelWallBuffer;
            }
        }
    }
}

