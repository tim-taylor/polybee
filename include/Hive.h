/**
 * @file
 *
 * Declaration of the Hive class
 */

#ifndef _HIVE_H
#define _HIVE_H

#include "utils.h"

class Environment;

/**
 * The Hive class ...
 */
class Hive {

public:
    //Hive() : m_x(0.0f), m_y(0.0f), m_direction(4) {}
    Hive(float x, float y, int direction, const Environment* pEnv);
    ~Hive() {}

    float x() const { return m_pos.x; }
    float y() const { return m_pos.y; }
    const pb::Pos2D& pos() const { return m_pos; }
    int direction() const { return m_direction; }
    bool inTunnel() const { return m_inTunnel; }

private:
    pb::Pos2D m_pos; // position of hive in environment coordinates
    int m_direction; // 0=North, 1=East, 2=South, 3=West, 4=Random

    const Environment* m_pEnv { nullptr };
    bool m_inTunnel { false };
};

#endif /* _HIVE_H */
