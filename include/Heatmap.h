/**
 * @file
 *
 * Declaration of the Heatmap class
 */

#ifndef _HEATMAP_H
#define _HEATMAP_H

#include <vector>
class Bee;

/**
 * The Heatmap class ...
 */

#include <ostream>

class Heatmap {

public:
    Heatmap();
    ~Heatmap() {}

    void initialise(std::vector<Bee>* bees);
    void update(); // update counts with current positions of bees
    void print(std::ostream& os); // print heatmap to output stream

private:
    std::vector<Bee>* m_pBees;

    int m_numCellsX;
    int m_numCellsY;
    int m_cellSize;

    std::vector<std::vector<int>> m_cells; // 2D array of cell counts
};

#endif /* _HEATMAP_H */
