/**
 * @file
 *
 * Implementation of the Bee class
 */

#include "Bee.h"
#include "Environment.h"
#include "PolyBeeCore.h"
#include <numbers>
#include <random>

const float BEE_STEP_SIZE = 20.0f; // how far a bee moves in one step

Bee::Bee(fPos pos, Environment* pEnv) :
    x(pos.x), y(pos.y), angle(0.0), m_pEnv(pEnv)
{}


Bee::Bee(fPos pos, float angle, Environment* pEnv) :
    x(pos.x), y(pos.y), angle(angle), m_pEnv(pEnv)
{}


void Bee::move() {
    //std::uniform_real_distribution<float> distDir(0.0, std::numbers::pi_v<float> * 2.0f);
    //angle = distDir(PolyBeeCore::m_sRngEngine);

    std::uniform_real_distribution<float> distDir(-0.4, 0.4);
    angle += distDir(PolyBeeCore::m_sRngEngine);

    x += BEE_STEP_SIZE * std::cos(angle);
    y += BEE_STEP_SIZE * std::sin(angle);
}

