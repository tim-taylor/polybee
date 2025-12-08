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
    Plant(float x, float y, int speciesID);
    ~Plant() {}

    float x() const { return m_x; }
    float y() const { return m_y; }
    int speciesID() const { return m_speciesID; }
    bool visited() const { return m_visited; }
    void setVisited(bool visited = true) { m_visited = visited; }
    float extractNectar(float amountWanted);

private:
    float m_x {0.0f};
    float m_y {0.0f};
    int m_speciesID {0};
    bool m_visited {false};
    float m_nectarAmount {0.0f}; // amount of nectar available in the flower
};

#endif /* _PLANT_H */
