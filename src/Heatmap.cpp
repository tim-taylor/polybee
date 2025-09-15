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

Heatmap::Heatmap() :
    m_numCellsX(0), m_numCellsY(0), m_cellSize(0), m_pBees{nullptr} {
}

void Heatmap::initialise(std::vector<Bee>* bees) {
    m_pBees = bees;
    m_cellSize = Params::heatmapCellSize;
    m_numCellsX = Params::envW / m_cellSize;
    m_numCellsY = Params::envH / m_cellSize;

    if (Params::envW % m_cellSize != 0) {
        msg_warning(std::format("Warning: env-w ({0}) is not a multiple of heatmap-cell-size ({1}). The heatmap will extend beyond the environment width.",
            Params::envW, m_cellSize));
        ++m_numCellsX;
    }
    if (Params::envH % m_cellSize != 0) {
        msg_warning(std::format("Warning: env-h ({0}) is not a multiple of heatmap-cell-size ({1}). The heatmap will extend beyond the environment height.",
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
        float x = (bee.x < SMALL_FLOAT_NUMBER) ? bee.x : (bee.x - SMALL_FLOAT_NUMBER); // ensure bees on upper x boundary are counted in correct cell
        float y = (bee.y < SMALL_FLOAT_NUMBER) ? bee.y : (bee.y - SMALL_FLOAT_NUMBER); // ensure bees on upper y boundary are counted in correct cell

        int cellX = static_cast<int>(x) / m_cellSize;
        int cellY = static_cast<int>(y) / m_cellSize;

        if (cellX >= 0 && cellX < m_numCellsX && cellY >= 0 && cellY < m_numCellsY) {
            m_cells[cellX][cellY]++;
        }
        else {
            // This should not happen if bees are correctly constrained within the environment
            msg_error_and_exit(std::format("Bee at position ({}, {}) is out of bounds for the heatmap.", bee.x, bee.y));
        }
    }
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
