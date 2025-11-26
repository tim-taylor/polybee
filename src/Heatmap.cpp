/**
 * @file
 *
 * Implementation of the Heatmap class
 */

#include "Heatmap.h"
#include "Params.h"
#include "Bee.h"
#include "utils.h"
#include <opencv2/opencv.hpp> // for OpenCV EMD calculation
#include <algorithm>
#include <cassert>
#include <chrono>
#include <format>
#include <limits>
#include <vector>


Heatmap::Heatmap(bool calcNormalised) : m_bCalcNormalised(calcNormalised),
    m_numCellsX(0), m_numCellsY(0), m_cellSize(0), m_pBees{nullptr} {
}

void Heatmap::initialise(std::vector<Bee>* bees) {
    m_pBees = bees;
    m_cellSize = Params::heatmapCellSize;
    m_numCellsX = Params::envW / m_cellSize;
    m_numCellsY = Params::envH / m_cellSize;

    if (static_cast<int>(Params::envW) % m_cellSize != 0) {
        pb::msg_warning(std::format("env-w ({0}) is not a multiple of heatmap-cell-size ({1}). The heatmap will extend beyond the environment width.",
            Params::envW, m_cellSize));
        ++m_numCellsX;
    }
    if (static_cast<int>(Params::envH) % m_cellSize != 0) {
        pb::msg_warning(std::format("env-h ({0}) is not a multiple of heatmap-cell-size ({1}). The heatmap will extend beyond the environment height.",
            Params::envH, m_cellSize));
        ++m_numCellsY;
    }

    // ensure we start with empty heatmaps
    m_cells.clear();
    m_cellsNormalised.clear();

    // size the heatmap as required and initialise all counts to zero
    m_cells.resize(m_numCellsX);
    for (int x = 0; x < m_numCellsX; ++x) {
        m_cells[x].resize(m_numCellsY, 0);  // initialize all counts to zero
    }

    // set up the uniform target heatmap (for use in PolyBeeEvolve)
    m_uniformTargetNormalised.resize(m_numCellsX);
    for (int x = 0; x < m_numCellsX; ++x) {
        m_uniformTargetNormalised[x].resize(m_numCellsY, 1.0 / (m_numCellsX * m_numCellsY));
    }

    // set up the anti-target heatmap (for use in PolyBeeEvolve)
    // Note: The anti-target heatmap is a degenerate case where all the density is
    // concentrated in a single cell (the top-left cell). This is useful for testing
    // the EMD calculation and as a challenging target for optimization.
    m_antiTargetNormalised.resize(m_numCellsX);
    for (int x = 0; x < m_numCellsX; ++x) {
        m_antiTargetNormalised[x].resize(m_numCellsY, 0.0f);
    }
    m_antiTargetNormalised[0][0] = 1.0f;

    m_highEmd = emd(m_uniformTargetNormalised, m_antiTargetNormalised);
}


void Heatmap::reset() {
    // reset all counts to zero
    for (int x = 0; x < m_numCellsX; ++x) {
        std::fill(m_cells[x].begin(), m_cells[x].end(), 0);
    }

    if (m_bCalcNormalised && !m_cellsNormalised.empty()) {
        for (int x = 0; x < m_numCellsX; ++x) {
            std::fill(m_cellsNormalised[x].begin(), m_cellsNormalised[x].end(), 0.0f);
        }
    }
}


void Heatmap::update() {
    assert(m_pBees != nullptr);

    // update counts based on current bee positions
    for (const Bee& bee : *m_pBees) {
        // Handle bees exactly on upper boundaries by placing them in the last valid cell
        int cellX = static_cast<int>(bee.x()) / m_cellSize;
        int cellY = static_cast<int>(bee.y()) / m_cellSize;

        // Clamp to valid cell indices (handles bees exactly at envW or envH)
        if (cellX >= m_numCellsX) cellX = m_numCellsX - 1;
        if (cellY >= m_numCellsY) cellY = m_numCellsY - 1;

        if (cellX >= 0 && cellX < m_numCellsX && cellY >= 0 && cellY < m_numCellsY) {
            m_cells[cellX][cellY]++;
        }
        else {
            // This should not happen if bees are correctly constrained within the environment
            pb::msg_error_and_exit(std::format("Bee at position ({}, {}) is out of bounds for the heatmap.", bee.x(), bee.y()));
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


// Wrapper to call the selected EMD implementation
float Heatmap::emd(const std::vector<std::vector<double>>& target) const
{
    return emd_opencv(m_cellsNormalised, target);
}

float Heatmap::emd(const std::vector<std::vector<double>>& heatmap1,
                   const std::vector<std::vector<double>>& heatmap2) const
{
    return emd_opencv(heatmap1, heatmap2);
}


// Earth Mover's Distance (EMD) between two 2D heatmaps using OpenCV
//  - Uses OpenCV's cv::EMD function with Manhattan (DIST_L1) distance metric
//  - Converts 2D heatmaps to OpenCV signature format
//  - Computational complexity: O(n³) for optimal transport solution
//
float Heatmap::emd_opencv(const std::vector<std::vector<double>>& heatmap1,
                          const std::vector<std::vector<double>>& heatmap2) const
{
    if (!m_bCalcNormalised) {
        pb::msg_error_and_exit("In Heatmap::emd_opencv(): Normalised heatmap calculation was not enabled.");
    }

    if (heatmap1.size() != heatmap2.size() ||
        (heatmap1.size() > 0 && heatmap1[0].size() != heatmap2[0].size())) {
        pb::msg_error_and_exit(
            std::format("Heatmaps must have the same dimensions for EMD calculation. Given sizes are {}x{} and {}x{}.",
                heatmap1.size(), heatmap1.empty() ? 0 : heatmap1[0].size(),
                heatmap2.size(), heatmap2.empty() ? 0 : heatmap2[0].size())
        );
    }

    if (heatmap1.empty()) {
        return 0.0f;
    }

    int rows = heatmap1.size();
    int cols = heatmap1[0].size();

    // Convert heatmaps to OpenCV signature format
    // Each signature is a matrix where each row represents: [weight, x, y]
    std::vector<cv::Vec3f> sig1, sig2;

    // Extract non-zero entries from heatmap1
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            if (heatmap1[y][x] > FLOAT_COMPARISON_EPSILON) {
                sig1.push_back(cv::Vec3f(static_cast<float>(heatmap1[y][x]),
                                        static_cast<float>(x),
                                        static_cast<float>(y)));
            }
        }
    }

    // Extract non-zero entries from heatmap2
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            if (heatmap2[y][x] > FLOAT_COMPARISON_EPSILON) {
                sig2.push_back(cv::Vec3f(static_cast<float>(heatmap2[y][x]),
                                        static_cast<float>(x),
                                        static_cast<float>(y)));
            }
        }
    }

    // Handle edge cases
    if (sig1.empty() && sig2.empty()) {
        return 0.0f;
    }
    if (sig1.empty()) {
        float sum = 0.0f;
        for (const auto& s : sig2) sum += s[0];
        return sum;
    }
    if (sig2.empty()) {
        float sum = 0.0f;
        for (const auto& s : sig1) sum += s[0];
        return sum;
    }

    // Convert to OpenCV Mat format
    cv::Mat signature1(sig1.size(), 3, CV_32F);
    cv::Mat signature2(sig2.size(), 3, CV_32F);

    for (size_t i = 0; i < sig1.size(); i++) {
        signature1.at<float>(i, 0) = sig1[i][0]; // weight
        signature1.at<float>(i, 1) = sig1[i][1]; // x
        signature1.at<float>(i, 2) = sig1[i][2]; // y
    }

    for (size_t i = 0; i < sig2.size(); i++) {
        signature2.at<float>(i, 0) = sig2[i][0]; // weight
        signature2.at<float>(i, 1) = sig2[i][1]; // x
        signature2.at<float>(i, 2) = sig2[i][2]; // y
    }

    // Compute EMD using Manhattan distance (DIST_L1)
    float emd = cv::EMD(signature1, signature2, cv::DIST_L1);

    return emd;
}


void Heatmap::print(std::ostream& os) const
{
    print_backend(os, m_cells);
}


void Heatmap::printNormalised(std::ostream& os) const
{
    if (!m_bCalcNormalised) {
        pb::msg_warning("Normalised heatmap calculation was not enabled. Nothing to print.");
        return;
    }

    print_backend(os, m_cellsNormalised);
}


template<typename T>
void Heatmap::print_backend(std::ostream& os, std::vector<std::vector<T>> const& heatmap) const
{
    for (int y = 0; y < m_numCellsY; ++y) {
        for (int x = 0; x < m_numCellsX; ++x) {
            //os << std::format("{}", heatmap[x][y]);
            os << heatmap[x][y];
            if (x < m_numCellsX - 1) {
                os << ",";
            }
        }
        os << std::endl;
    }
}

