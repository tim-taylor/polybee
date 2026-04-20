/**
 * @file
 *
 * Implementation of the Flowmap class
 *
 * The double-angle method for calculating the predominant movement axis and strength in each cell
 * is based on the approach described online by Google search results and on ChatGPT:
 * https://chatgpt.com/share/69e0b16d-3edc-8396-84af-43de63a8a9ac
 */

#include "Flowmap.h"

#include "Params.h"
#include "Bee.h"
#include "utils.h"


Flowmap::Flowmap() :  m_numCellsX(0), m_numCellsY(0), m_cellSize(0), m_pAllBees{nullptr} {
}


void Flowmap::initialise(std::vector<Bee>* bees) {
    m_pAllBees = bees;
    m_cellSize = Params::flowmapCellSize;
    m_numCellsX = Params::envW / m_cellSize;
    m_numCellsY = Params::envH / m_cellSize;

    if (static_cast<int>(Params::envW) % m_cellSize != 0) {
        pb::msg_warning(std::format("env-w ({0}) is not a multiple of flowmap-cell-size ({1}). The Flowmap will extend beyond the environment width.",
            Params::envW, m_cellSize));
        ++m_numCellsX;
    }
    if (static_cast<int>(Params::envH) % m_cellSize != 0) {
        pb::msg_warning(std::format("env-h ({0}) is not a multiple of flowmap-cell-size ({1}). The Flowmap will extend beyond the environment height.",
            Params::envH, m_cellSize));
        ++m_numCellsY;
    }

    // ensure we start with an empty Flowmap
    m_cells.clear();

    // size the Flowmap as required and initialise all counts to zero
    m_cells.resize(m_numCellsX);
    for (int x = 0; x < m_numCellsX; ++x) {
        m_cells[x].resize(m_numCellsY);  // default constructor of FlowmapCell will initialize all counts to zero
    }
}


void Flowmap::reset() {
    // reset all counts to zero
    for (int x = 0; x < m_numCellsX; ++x) {
        for (auto& cell : m_cells[x]) {
            cell.reset();
        }
    }
}


// update flowmap data with current bee movements
void Flowmap::update() {
    assert(m_pAllBees != nullptr);

    // update counts based on current bee positions
    for (const Bee& bee : *m_pAllBees) {
        // Handle bees exactly on upper boundaries by placing them in the last valid cell
        int cellX = static_cast<int>(bee.x()) / m_cellSize;
        int cellY = static_cast<int>(bee.y()) / m_cellSize;

        // Clamp to valid cell indices (handles bees exactly at envW or envH)
        if (cellX >= m_numCellsX) cellX = m_numCellsX - 1;
        if (cellY >= m_numCellsY) cellY = m_numCellsY - 1;

        pb::Pos2D deltaMovement = bee.deltaMovement();
        if (deltaMovement.x == 0.0f && deltaMovement.y == 0.0f) {
            continue; // bee didn't move this step (e.g. on flower or in hive) — no direction to record
        }
        float theta = std::atan2(deltaMovement.y, deltaMovement.x);

        m_cells[cellX][cellY].thetas.push_back(theta);
        m_cells[cellX][cellY].count++;
    }
}


// calculate the predominant movement axis and strength for each cell based on all recorded movement angles
void Flowmap::calculateFlow() {
    for (int x = 0; x < m_numCellsX; ++x) {
        for (auto& cell : m_cells[x]) {
            if (cell.count > 0) {
                // calculate average flow vector by summing unit vectors for each movement angle and dividing by count
                float sumSinTwoTheta = 0.0f;
                float sumCosTwoTheta = 0.0f;
                for (float theta : cell.thetas) {
                    float twoTheta = 2.0f * theta; // multiply by 2 to get the headless direction in the range [0, 2*pi)
                    sumSinTwoTheta += std::sin(twoTheta);
                    sumCosTwoTheta += std::cos(twoTheta);
                }
                float avgSinTwoTheta = sumSinTwoTheta / cell.count;
                float avgCosTwoTheta = sumCosTwoTheta / cell.count;

                // calculate predominant movement axis as the angle of the average flow vector
                // (divide by 2 to get the headless direction back in the range [0, pi))
                cell.axis = 0.5f * std::atan2(avgSinTwoTheta, avgCosTwoTheta);

                // calculate strength of alignment to predominant axis as the length of the average flow vector
                cell.strength = std::sqrt(avgSinTwoTheta * avgSinTwoTheta + avgCosTwoTheta * avgCosTwoTheta);
            }
        }
    }
}


int Flowmap::max_count() const {
    int maxCount = 0;
    for (const auto& column : m_cells) {
        for (const auto& cell : column) {
            if (cell.count > maxCount) {
                maxCount = cell.count;
            }
        }
    }
    return maxCount;
}

