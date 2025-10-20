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

    // change direction a bit
    angle += m_distDir(PolyBeeCore::m_sRngEngine);

    // move in that direction
    x += Params::stepLength * std::cos(angle);
    y += Params::stepLength * std::sin(angle);

    // keep within bounds of environment
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x > Params::envW) x = static_cast<float>(Params::envW);
    if (y > Params::envH) y = static_cast<float>(Params::envH);
}

