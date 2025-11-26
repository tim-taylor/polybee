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
    void reset();
    void update(); // update counts with current positions of bees
    void print(std::ostream& os) const; // print heatmap to output stream
    void printNormalised(std::ostream& os) const; // print normalised heatmap to output stream

    bool isNormalisedCalculated() const { return m_bCalcNormalised; }
    int size_x() const { return m_numCellsX; }
    int size_y() const { return m_numCellsY; }

    // compute and return the "earth mover's distance" between this heatmap and the target heatmap
    float emd(const std::vector<std::vector<double>>& target) const;
    float emd(const std::vector<std::vector<double>>& heatmap1, const std::vector<std::vector<double>>& heatmap2) const;

    float high_emd() const { return m_highEmd; }

    const std::vector<std::vector<int>>& cells() const { return m_cells; }
    const std::vector<std::vector<double>>& cellsNormalised() const { return m_cellsNormalised; }

    const std::vector<std::vector<double>>& uniformTargetNormalised() const { return m_uniformTargetNormalised; }
    const std::vector<std::vector<double>>& antiTargetNormalised() const { return m_antiTargetNormalised; } // for use in PolyBeeEvolve

private:
    void calcNormalised(); // calculate the normalised version of the heatmap

    // various implementations of EMD calculation
    float emd_opencv(const std::vector<std::vector<double>>& heatmap1, const std::vector<std::vector<double>>& heatmap2) const;

    template<typename T>
    void print_backend(std::ostream& os, std::vector<std::vector<T>> const& heatmap) const;

    std::vector<Bee>* m_pBees;

    int m_numCellsX;
    int m_numCellsY;
    int m_cellSize;
    float m_highEmd; // a high EMD value for this environment (as measured between uniform target and anti-target heatmaps)
    bool m_bCalcNormalised; // whether to calculate and store a normalised version of the heatmap

    std::vector<std::vector<int>> m_cells; // 2D array of cell counts
    std::vector<std::vector<double>> m_cellsNormalised; // 2D array of normalised cell counts

    std::vector<std::vector<double>> m_uniformTargetNormalised; // Normalised uniform target heatmap
    std::vector<std::vector<double>> m_antiTargetNormalised; // Normlised heatmap with top left cell = 1, all others = 0
};

#endif /* _HEATMAP_H */
