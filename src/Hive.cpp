/**
 * @file
 *
 * Implementation of the Hive class
 */

#include "Hive.h"
#include "Environment.h"
#include <cassert>


Hive::Hive(float x, float y, int direction, const Environment* pEnv) :
    m_pos(x, y), m_direction(direction), m_pEnv(pEnv)
{
    assert(m_pEnv != nullptr);
    m_inTunnel = m_pEnv->inTunnel(m_pos.x, m_pos.y);
}