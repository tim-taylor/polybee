/**
 * @file
 *
 * Declaration of the Plant class
 */

#ifndef _PLANT_H
#define _PLANT_H

/**
 * The Plant class ...
 */
class Plant {

public:
    Plant() : m_x(0.0f), m_y(0.0f), m_speciesID(0) {}
    Plant(float x, float y, int speciesID) : m_x(x), m_y(y), m_speciesID(speciesID) {}
    ~Plant() {}

    float x() const { return m_x; }
    float y() const { return m_y; }
    int speciesID() const { return m_speciesID; }


private:
    float m_x {0.0f};
    float m_y {0.0f};
    int m_speciesID {0};
};

#endif /* _PLANT_H */
