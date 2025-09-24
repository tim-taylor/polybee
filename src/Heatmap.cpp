/**
 * @file
 *
 * Implementation of the Heatmap class
 */

#include "Heatmap.h"
#include "Params.h"
#include "Bee.h"
#include "utils.h"
#include <format>
#include <cassert>
#include <algorithm>

#include <chrono>

Heatmap::Heatmap(bool calcNormalised) : m_bCalcNormalised(calcNormalised),
    m_numCellsX(0), m_numCellsY(0), m_cellSize(0), m_pBees{nullptr} {
}

void Heatmap::initialise(std::vector<Bee>* bees) {
    m_pBees = bees;
    m_cellSize = Params::heatmapCellSize;
    m_numCellsX = Params::envW / m_cellSize;
    m_numCellsY = Params::envH / m_cellSize;

    if (Params::envW % m_cellSize != 0) {
        pb::msg_warning(std::format("env-w ({0}) is not a multiple of heatmap-cell-size ({1}). The heatmap will extend beyond the environment width.",
            Params::envW, m_cellSize));
        ++m_numCellsX;
    }
    if (Params::envH % m_cellSize != 0) {
        pb::msg_warning(std::format("env-h ({0}) is not a multiple of heatmap-cell-size ({1}). The heatmap will extend beyond the environment height.",
            Params::envH, m_cellSize));
        ++m_numCellsY;
    }

    m_cells.resize(m_numCellsX);
    for (int x = 0; x < m_numCellsX; ++x) {
        m_cells[x].resize(m_numCellsY, 0);  // initialize all counts to zero
    }
}

void Heatmap::update() {
    assert(m_pBees != nullptr);

    // update counts based on current bee positions
    for (const Bee& bee : *m_pBees) {
        // Handle bees exactly on upper boundaries by placing them in the last valid cell
        int cellX = static_cast<int>(bee.x) / m_cellSize;
        int cellY = static_cast<int>(bee.y) / m_cellSize;

        // Clamp to valid cell indices (handles bees exactly at envW or envH)
        if (cellX >= m_numCellsX) cellX = m_numCellsX - 1;
        if (cellY >= m_numCellsY) cellY = m_numCellsY - 1;

        if (cellX >= 0 && cellX < m_numCellsX && cellY >= 0 && cellY < m_numCellsY) {
            m_cells[cellX][cellY]++;
        }
        else {
            // This should not happen if bees are correctly constrained within the environment
            pb::msg_error_and_exit(std::format("Bee at position ({}, {}) is out of bounds for the heatmap.", bee.x, bee.y));
        }
    }

    if (m_bCalcNormalised) {
        calcNormalised();
    }
}

void Heatmap::calcNormalised() {
    // if we've not initialised the normalised cells array yet, do so now
    if (m_cellsNormalised.empty()) {
        m_cellsNormalised.resize(m_numCellsX);
        for (int x = 0; x < m_numCellsX; ++x) {
            m_cellsNormalised[x].resize(m_numCellsY, 0.0f);
        }
    }

    assert(m_cellsNormalised.size() == m_numCellsX);
    assert(m_cellsNormalised[0].size() == m_numCellsY);

    // Calculate the normalised version of the heatmap
    int totalCount = 0;
    for (int x = 0; x < m_numCellsX; ++x) {
        for (int y = 0; y < m_numCellsY; ++y) {
            totalCount += m_cells[x][y];
        }
    }

    for (int x = 0; x < m_numCellsX; ++x) {
        for (int y = 0; y < m_numCellsY; ++y) {
            m_cellsNormalised[x][y] = (totalCount == 0) ? 0.0f : static_cast<float>(m_cells[x][y]) / totalCount;
        }
    }
}

float Heatmap::emd_approx(const std::vector<std::vector<float>>& target) const {
    return pb::earthMoversDistanceApprox(m_cellsNormalised, target);
}

float Heatmap::emd_full(const std::vector<std::vector<float>>& target) const {
    return pb::earthMoversDistanceFull(m_cellsNormalised, target);
}

float Heatmap::emd_lemon(const std::vector<std::vector<int>>& target) const {
    return pb::earthMoversDistanceLemon(m_cells, target);
}

float Heatmap::emd_hat(const std::vector<std::vector<float>>& target) const {

    std::vector<std::vector<double>> fCells(m_cellsNormalised.size()); //preconstruct to given size
    std::vector<std::vector<double>> fTarget(target.size()); //preconstruct output to given size

    std::transform(
        m_cellsNormalised.begin(), m_cellsNormalised.end(), fCells.begin(),
        [](const auto& in) {return std::vector<double>(in.begin(), in.end());}  //use vectors range based constructor
    );

    std::transform(
        target.begin(), target.end(), fTarget.begin(),
        [](const auto& in) {return std::vector<double>(in.begin(), in.end());}  //use vectors range based constructor
    );

    //return pb::earthMoversDistanceHat(fCells, fTarget);


    // TEMP TIMING CODE
    auto start = std::chrono::high_resolution_clock::now();
    auto emdHatVal = pb::earthMoversDistanceHat(fCells, fTarget);
    auto end = std::chrono::high_resolution_clock::now();
    auto emdHatTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    pb::msg_info(std::format("EMD Hat time: {} us", emdHatTime));

    return emdHatVal;

}

void Heatmap::print(std::ostream& os) {
    for (int y = 0; y < m_numCellsY; ++y) {
        for (int x = 0; x < m_numCellsX; ++x) {
            os << m_cells[x][y];
            if (x < m_numCellsX - 1) {
                os << ",";
            }
        }
        os << std::endl;
    }
}

void Heatmap::printNormalised(std::ostream& os) {
    if (!m_bCalcNormalised) {
        pb::msg_warning("Normalised heatmap calculation was not enabled. Nothing to print.");
        return;
    }

    for (int y = 0; y < m_numCellsY; ++y) {
        for (int x = 0; x < m_numCellsX; ++x) {
            //os << std::format("{}", m_cellsNormalised[x][y]);
            os << m_cellsNormalised[x][y];
            if (x < m_numCellsX - 1) {
                os << ",";
            }
        }
        os << std::endl;
    }
}

