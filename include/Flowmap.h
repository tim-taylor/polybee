/**
 * @file
 *
 * Declaration of the Flowmap class
 */

#ifndef _FLOWMAP_H
#define _FLOWMAP_H

#include <vector>
class Bee;

/**
 * The Flowmap class ...
 */
class Flowmap {

public:
    Flowmap();
    ~Flowmap() {}

    void initialise(std::vector<Bee>* bees);
    void reset();
    void update(); // update counts with current positions of bees

    int size_x() const { return m_numCellsX; }
    int size_y() const { return m_numCellsY; }

private:
    int m_numCellsX;
    int m_numCellsY;
    int m_cellSize;
    std::vector<std::vector<int>> m_cells; // 2D array of cell counts
    std::vector<Bee>* m_pBees;

};

#endif /* _FLOWMAP_H */
