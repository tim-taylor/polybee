/**
 * @file
 *
 * Implementation of the Plant class
 */

#include "Plant.h"
#include "Params.h"
#include <cassert>


Plant::Plant(float x, float y, int speciesID)
    : m_x(x), m_y(y), m_speciesID(speciesID)
{
    assert(Params::initialised());
    m_nectarAmount = Params::flowerInitialNectar;
}


float Plant::extractNectar(float amountWanted) {
    float amountExtracted = 0.0f;
    if (m_nectarAmount >= amountWanted) {
        amountExtracted = amountWanted;
        m_nectarAmount -= amountWanted;
    }
    else {
        amountExtracted = m_nectarAmount;
        m_nectarAmount = 0.0f;
    }
    return amountExtracted;
}
