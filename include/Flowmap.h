/**
 * @file
 *
 * Declaration of the Flowmap class
 */

#ifndef _FLOWMAP_H
#define _FLOWMAP_H

#include <vector>

class Bee;

struct FlowmapCell {
    float axis {0.0f};          // predominant movement axis for this cell
    float strength {0.0f};      // strength of alignment to the predominant axis (between 0 and 1)
    int count {0};              // number of bee movements recorded in this cell
    std::vector<float> thetas;  // record of all movement angles for bees in this cell

    void reset() {
        axis = 0.0f;
        strength = 0.0f;
        count = 0;
        thetas.clear();
    }
};

/**
 * The Flowmap class ...
 */
class Flowmap {

public:
    Flowmap();
    ~Flowmap() {}

    void initialise(std::vector<Bee>* bees);
    void reset();
    void update();          // update counts with current positions of bees
    void calculateFlow();   // calculate predominant movement axis and strength for each cell

    int size_x() const { return m_numCellsX; }
    int size_y() const { return m_numCellsY; }
    int cell_size() const { return m_cellSize; }
    int max_count() const; // return the maximum count of any cell (for scaling visualisation)
    const std::vector<std::vector<FlowmapCell>>& cells() const { return m_cells; }

private:
    int m_numCellsX;
    int m_numCellsY;
    int m_cellSize;
    std::vector<std::vector<FlowmapCell>> m_cells;  // 2D array of local flow information
    std::vector<Bee>* m_pAllBees;                   // pointer to the vector of all bees in the environment
};

#endif /* _FLOWMAP_H */
