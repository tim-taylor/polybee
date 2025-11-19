/**
 * @file
 *
 * Declaration of the Hive class
 */

#ifndef _HIVE_H
#define _HIVE_H

class Environment;

/**
 * The Hive class ...
 */
class Hive {

public:
    //Hive() : m_x(0.0f), m_y(0.0f), m_direction(4) {}
    Hive(float x, float y, int direction, const Environment* pEnv);
    ~Hive() {}

    float x() const { return m_x; }
    float y() const { return m_y; }
    int direction() const { return m_direction; }
    bool inTunnel() const { return m_inTunnel; }

private:
    float m_x; // position of hive in enrironment coordinates
    float m_y; // position of hive in enrironment coordinates
    int m_direction; // 0=North, 1=East, 2=South, 3=West, 4=Random

    const Environment* m_pEnv { nullptr };
    bool m_inTunnel { false };
};

#endif /* _HIVE_H */
