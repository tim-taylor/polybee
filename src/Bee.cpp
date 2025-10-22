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
}

void Bee::move() {
    // record current position
    path.emplace_back(x, y);
    if (path.size() > Params::beePathRecordLen) {
        path.erase(path.begin());
    }

    bool isInTunnel = m_pEnv->inTunnel(x, y);

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
    float wallThickness = m_pEnv->getTunnel().thickness();

    bool newPosInTunnel = m_pEnv->inTunnel(newx, newy);
    if ((isInTunnel && newPosInTunnel) || (!isInTunnel && !newPosInTunnel)) {
        // valid move: update position
        x = newx;
        y = newy;
        // TODO - if bee is outside tunnel, check it is not within tunnel wall thickness of the tunnel...
    }
    else {
        // bee has crossed tunnel boundary, so we need to figure out if it can enter/exit the tunnel at this point
        // For now, just prevent the move
        // TODO - implement proper tunnel entrance/exit checking
        if (isInTunnel) {
            // bee is currently in tunnel
            // TODO
        }
        else {
            // bee is currently outside tunnel
            // TODO
        }

    }

}

