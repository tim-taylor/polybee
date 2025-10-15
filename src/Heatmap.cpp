/**
 * @file
 *
 * Implementation of the Heatmap class
 */

#include "Heatmap.h"
#include "Params.h"
#include "Bee.h"
#include "utils.h"
#include "FastEMD/emd_hat.hpp" // for emd_hat calculation
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


// Approximate Earth Mover's Distance (EMD) between two 2D heatmaps
//  - What it is: Fast approximation using cumulative distribution comparison
//  - Use case: When you need quick comparisons and approximate results are sufficient
//  - Computational complexity: O(n×m) where n,m are grid dimensions
//
// This implementation was written by Claude Code
//  - Based on the mathematical property that EMD can be approximated by comparing cumulative distributions
float Heatmap::emd_approx(const std::vector<std::vector<double>>& heatmap1,
                           const std::vector<std::vector<double>>& heatmap2) const
{
    if (!m_bCalcNormalised) {
        pb::msg_error_and_exit("In Heatmap::emd_approx(): Normalised heatmap calculation was not enabled.");
    }

    if (heatmap1.size() != heatmap2.size() ||
        (heatmap1.size() > 0 && heatmap1[0].size() != heatmap2[0].size())) {
        pb::msg_error_and_exit("Heatmaps must have the same dimensions for EMD calculation");
    }

    if (heatmap1.empty()) {
        return 0.0f;
    }

    int rows = heatmap1.size();
    int cols = heatmap1[0].size();

    // Create cumulative distribution grids
    std::vector<std::vector<float>> cumulative1(rows, std::vector<float>(cols, 0.0f));
    std::vector<std::vector<float>> cumulative2(rows, std::vector<float>(cols, 0.0f));

    // Build cumulative distributions
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            cumulative1[i][j] = heatmap1[i][j];
            cumulative2[i][j] = heatmap2[i][j];

            if (i > 0) {
                cumulative1[i][j] += cumulative1[i-1][j];
                cumulative2[i][j] += cumulative2[i-1][j];
            }
            if (j > 0) {
                cumulative1[i][j] += cumulative1[i][j-1];
                cumulative2[i][j] += cumulative2[i][j-1];
            }
            if (i > 0 && j > 0) {
                cumulative1[i][j] -= cumulative1[i-1][j-1];
                cumulative2[i][j] -= cumulative2[i-1][j-1];
            }
        }
    }

    // Calculate Earth Mover's Distance using L1 distance on cumulative distributions
    float emd = 0.0f;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            emd += std::abs(cumulative1[i][j] - cumulative2[i][j]);
        }
    }

    return emd;
}


// Full Earth Mover's Distance (EMD) between two 2D heatmaps
//  - What it is: Greedy approximation to EMD implementation using optimal transport principles
//  - Method: Greedy transport algorithm with Manhattan distance as ground metric
//  - Use case: When you need mathematically accurate EMD results
//  - Computational complexity: O(k²×s) where k is number of non-zero cells, s is iterations
//
// Note: This "Full" implementation uses a greedy approximation rather than the optimal Hungarian
// algorithm for computational efficiency. For truly optimal EMD, you'd need specialized libraries
// like LEMON or CPLEX.
//
// This implementation was written by Claude Code
//
float Heatmap::emd_full(const std::vector<std::vector<double>>& heatmap1,
                        const std::vector<std::vector<double>>& heatmap2) const
{
    if (!m_bCalcNormalised) {
        pb::msg_error_and_exit("In Heatmap::emd_full(): Normalised heatmap calculation was not enabled.");
    }

    if (heatmap1.size() != heatmap2.size() ||
        (heatmap1.size() > 0 && heatmap1[0].size() != heatmap2[0].size())) {
        pb::msg_error_and_exit("Heatmaps must have the same dimensions for EMD calculation");
    }

    if (heatmap1.empty()) {
        return 0.0f;
    }

    int rows = heatmap1.size();
    int cols = heatmap1[0].size();

    // Convert 2D heatmaps to 1D with coordinates
    std::vector<float> supply, demand;
    std::vector<std::pair<int, int>> supply_coords, demand_coords;

    // Extract non-zero supply points (heatmap1)
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (heatmap1[i][j] > FLOAT_COMPARISON_EPSILON) {
                supply.push_back(heatmap1[i][j]);
                supply_coords.push_back({i, j});
            }
        }
    }

    // Extract non-zero demand points (heatmap2)
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (heatmap2[i][j] > FLOAT_COMPARISON_EPSILON) {
                demand.push_back(heatmap2[i][j]);
                demand_coords.push_back({i, j});
            }
        }
    }

    if (supply.empty() && demand.empty()) {
        return 0.0f;
    }

    // If one is empty, return sum of the other
    if (supply.empty()) {
        return std::accumulate(demand.begin(), demand.end(), 0.0f);
    }
    if (demand.empty()) {
        return std::accumulate(supply.begin(), supply.end(), 0.0f);
    }

    // Build cost matrix (Manhattan distance between coordinates)
    int n_supply = supply.size();
    int n_demand = demand.size();
    std::vector<std::vector<float>> cost_matrix(n_supply, std::vector<float>(n_demand));

    for (int i = 0; i < n_supply; i++) {
        for (int j = 0; j < n_demand; j++) {
            float dx = supply_coords[i].first - demand_coords[j].first;
            float dy = supply_coords[i].second - demand_coords[j].second;
            cost_matrix[i][j] = std::abs(dx) + std::abs(dy);
        }
    }

    // Simplified transport solver using greedy approach
    // This is not optimal but computationally efficient
    std::vector<float> supply_remaining = supply;
    std::vector<float> demand_remaining = demand;

    float total_cost = 0.0f;

    while (true) {
        // Find the minimum cost unmet supply-demand pair
        float min_cost = std::numeric_limits<float>::max();
        int best_i = -1, best_j = -1;

        for (int i = 0; i < n_supply; i++) {
            if (supply_remaining[i] <= FLOAT_COMPARISON_EPSILON) continue;
            for (int j = 0; j < n_demand; j++) {
                if (demand_remaining[j] <= FLOAT_COMPARISON_EPSILON) continue;
                if (cost_matrix[i][j] < min_cost) {
                    min_cost = cost_matrix[i][j];
                    best_i = i;
                    best_j = j;
                }
            }
        }

        if (best_i == -1 || best_j == -1) break;

        // Transport as much as possible between best_i and best_j
        float transport_amount = std::min(supply_remaining[best_i], demand_remaining[best_j]);
        total_cost += transport_amount * cost_matrix[best_i][best_j];

        supply_remaining[best_i] -= transport_amount;
        demand_remaining[best_j] -= transport_amount;
    }

    return total_cost;
}


// This EMD method use's Pele's emd_hat implementation available at
// https://github.com/ofirpele/FastEMD/blob/master/src/emd_hat.hpp
//
float Heatmap::emd_hat_pele(const std::vector<std::vector<double>>& heatmap1,
                            const std::vector<std::vector<double>>& heatmap2) const
{
    if (!m_bCalcNormalised) {
        pb::msg_error_and_exit("In Heatmap::emd_hat(): Normalised heatmap calculation was not enabled.");
    }

    if ((heatmap1.size() != heatmap2.size()) ||
        (heatmap1.empty()) ||
        (heatmap1[0].size() != heatmap2[0].size())) {
        pb::msg_error_and_exit("In Heatmap::emd_hat(): Heatmaps must have the same dimensions for EMD calculation");
    }

    // Flatten the heatmaps to 1D vectors
    std::vector<double> flatHeatmap1, flatHeatmap2;

    int numRows = heatmap1.size();
    int numCols = heatmap1[0].size();
    int total_cells = numRows * numCols;

    // ... flatten heatmap1 heatmap
    flatHeatmap1.reserve(total_cells);
    for (const auto& inner : heatmap1) {
        flatHeatmap1.insert(flatHeatmap1.end(), inner.begin(), inner.end());
    }

    // .. flatten heatmap2 heatmap
    flatHeatmap2.reserve(total_cells);
    for (const auto& inner : heatmap2) {
        flatHeatmap2.insert(flatHeatmap2.end(), inner.begin(), inner.end());
    }

    // Calculate the ground distance matrix - here we're using Euclidean distance
    std::vector< std::vector<double> > distanceMatrix;
    std::vector<double> distanceMatrix_row(total_cells);
    for (size_t i = 0; i < total_cells; ++i) distanceMatrix.push_back(distanceMatrix_row);
    //int max_distanceMatrix = -1;

    for (int r1 = 0; r1 < numRows; r1++) {
        for (int c1 = 0; c1 < numCols; c1++) {
            size_t j = r1 * numCols + c1;  // Current cell index (row-major order)

            for (int r2 = 0; r2 < numRows; r2++) {
                for (int c2 = 0; c2 < numCols; c2++) {
                    size_t i = r2 * numCols + c2;  // heatmap2 cell index
                    distanceMatrix[i][j]= static_cast<double>(sqrt((r1-r2)*(r1-r2)+(c1-c2)*(c1-c2)));
                    //if (distanceMatrix[i][j] > max_distanceMatrix) max_distanceMatrix = distanceMatrix[i][j];
                }
            }
		}
	}

    return emd_hat<double>()(flatHeatmap1, flatHeatmap2, distanceMatrix);
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
        pb::msg_error_and_exit("Heatmaps must have the same dimensions for EMD calculation");
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

