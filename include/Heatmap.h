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
    Heatmap(bool calcNormalised = true);
    ~Heatmap() {}

    void initialise(std::vector<Bee>* bees);
    void update(); // update counts with current positions of bees
    void print(std::ostream& os); // print heatmap to output stream
    void printNormalised(std::ostream& os); // print normalised heatmap to output stream

    bool isNormalisedCalculated() const { return m_bCalcNormalised; }
    int size_x() const { return m_numCellsX; }
    int size_y() const { return m_numCellsY; }

    // compute and return the "earth mover's distance" between this heatmap and the target heatmap
    float emd_approx(const std::vector<std::vector<float>>& target) const;
    float emd_full(const std::vector<std::vector<float>>& target) const;
    float emd_lemon(const std::vector<std::vector<int>>& target) const;

    const std::vector<std::vector<int>>& cells() const { return m_cells; }
    const std::vector<std::vector<float>>& cellsNormalised() const { return m_cellsNormalised; }

private:
    void calcNormalised(); // calculate the normalised version of the heatmap

    std::vector<Bee>* m_pBees;

    int m_numCellsX;
    int m_numCellsY;
    int m_cellSize;
    bool m_bCalcNormalised; // whether to calculate and store a normalised version of the heatmap

    std::vector<std::vector<int>> m_cells; // 2D array of cell counts
    std::vector<std::vector<float>> m_cellsNormalised; // 2D array of normalised cell counts
};

#endif /* _HEATMAP_H */
