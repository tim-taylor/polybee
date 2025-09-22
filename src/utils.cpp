#include "utils.h"
#include "Params.h"
#include "lemon_thresholded_emd.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <vector>
#include <limits>
#include <numeric>

namespace Polybee {

void msg_error_and_exit(std::string msg) {
    std::cerr << "ERROR: " << msg << std::endl;
    exit(EXIT_FAILURE);
}

void msg_warning(std::string msg) {
    std::cerr << "WARNING: " << msg << std::endl;
}

void msg_info(std::string msg) {
    if (!Params::bCommandLineQuiet) {
        std::cout << "INFO: " << msg << std::endl;
    }
}

// Earth Mover's Distance (EMD) between two 2D heatmaps using the LEMON library
//  - Exact EMD calculation using thresholded ground distance
//    Technically, it calculates the EMD Hat variant (as proposed by Pele and Werman)
//    between two non-normalised histograms/heatmaps.
//
float earthMoversDistanceLemon(const std::vector<std::vector<int>>& heatmap1,
    const std::vector<std::vector<int>>& heatmap2)
{
    // Flatten the heatmaps to 1D vectors
    std::vector<int> flat1, flat2;

    // Flatten heatmap1
    size_t total_size1 = heatmap1.size() * heatmap1[0].size(); // assuming non-empty and rectangular
    flat1.reserve(total_size1);
    for (const auto& inner : heatmap1) {
        flat1.insert(flat1.end(), inner.begin(), inner.end());
    }

    // Flatten heatmap2
    size_t total_size2 = heatmap2.size() * heatmap2[0].size(); // assuming non-empty and rectangular
    flat2.reserve(total_size2);
    for (const auto& inner : heatmap2) {
        flat2.insert(flat2.end(), inner.begin(), inner.end());
    }

    // Initialize parameters for thresholded EMD algorithm
    // Based on the Matlab reference implementation
    int rows = heatmap1.size();
    int cols = heatmap1[0].size();
    int total_cells = rows * cols;

    // Cost multiplier to convert float distances to int
    // (as required by the lemon_thresholded_emd function)
    // The values 1000 and 3*1000 are taken from the Matlab code in
    // https://github.com/ofirpele/FastEMD/blob/master/src/demo_FastEMD3.m
    const int COST_MULT_FACTOR = 1000;
    const int thresh = 3 * COST_MULT_FACTOR;  // Threshold distance

    // C_inds[i] contains indices of cells within threshold distance of cell i
    // C_vals[i] contains the corresponding distances to those cells
    std::vector<std::vector<size_t>> C_inds(total_cells);
    std::vector<std::vector<int>> C_vals(total_cells);

    // Build sparse distance matrix representation
    for (int r1 = 0; r1 < rows; r1++) {
        for (int c1 = 0; c1 < cols; c1++) {
            size_t j = r1 * cols + c1;  // Current cell index (row-major order)

            for (int r2 = 0; r2 < rows; r2++) {
                for (int c2 = 0; c2 < cols; c2++) {
                    size_t i = r2 * cols + c2;  // Target cell index

                    // Calculate Euclidean distance scaled by cost factor
                    float euclidean_dist = std::sqrt((r1 - r2) * (r1 - r2) + (c1 - c2) * (c1 - c2));
                    int scaled_dist = static_cast<int>(COST_MULT_FACTOR * euclidean_dist);

                    // Apply threshold: min(thresh, scaled_distance)
                    int final_dist = std::min(thresh, scaled_dist);

                    // Store only if within threshold (sparse representation)
                    if (final_dist < thresh) {
                        C_inds[j].push_back(i);
                        C_vals[j].push_back(final_dist);
                    }
                }
            }
        }
    }

    return lemon_thresholded_emd(flat1, flat2, C_inds, C_vals, thresh);

    // The following initialisation docs are from the Matlab code in
    // https://github.com/ofirpele/FastEMD/blob/master/src/demo_FastEMD3.m

    /*
    COST_MULT_FACTOR= 1000;
    THRESHOLD= 3*COST_MULT_FACTOR;
    D= zeros(R*C,R*C,'int32');
    j= 0;
    for c1=1:C
        for r1=1:R
            j= j+1;
            i= 0;
            for c2=1:C
                for r2=1:R
                    i= i+1;
                    D(i,j)= min( [THRESHOLD (COST_MULT_FACTOR*sqrt((r1-r2)^2+(c1-c2)^2))] );
                end
            end
        end
    end
    extra_mass_penalty= int32(-1);
    flowType= int32(3);

    P= int32(im1(:));
    Q= int32(im2(:));
    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

    % The demo includes several ways to call emd_hat_mex and emd_hat_gd_metric_mex
    demo_FastEMD_compute(P,Q,D,extra_mass_penalty,flowType);
    */

    /* COMMENTS FROM demo_FastEMD_compute
    %
    % Fastest version - Exploits the fact that the ground distance is
    % thresholded and a metric.
    tic
    emd_hat_gd_metric_mex_val= emd_hat_gd_metric_mex(P,Q,D,extra_mass_penalty);
    fprintf(1,'emd_hat_gd_metric_mex time in seconds: %f\n',toc);

    % A little slower - with flows
    tic
    [emd_hat_gd_metric_mex_val_with_flow F1]= emd_hat_gd_metric_mex(P,Q,D,extra_mass_penalty,flowType);
    fprintf(1,'emd_hat_gd_metric_mex computing the flow also, time in seconds: %f\n', ...
            toc);

    % Even slower - Only exploits the fact that the ground distance is
    % thresholded
    tic
    emd_hat_mex_val= emd_hat_mex(P,Q,D,extra_mass_penalty);
    fprintf(1,'emd_hat_mex time in seconds: %f\n',toc);

    % Even slower - with flows
    tic
    [emd_hat_mex_val_with_flow F2]= emd_hat_mex(P,Q,D,extra_mass_penalty,flowType);
    fprintf(1,'emd_hat_mex computing the flow also, time in seconds: %f\n',toc);
    */

    /* HEADERS FROM emd_hat_gd_metric_mex.m
    %
    % [dist F]= emd_hat_gd_metric_mex(P,Q,D,extra_mass_penalty,FType)
    %
    % Fastest version of EMD. Also, in my experience metric ground distance
    % yields better performance.
    %
    % Output:
    %  dist - the computed distance
    %  F - the flow matrix, see "FType"
    %  Note: if it's not required it's computation is totally skipped.
    %
    % Required Input:
    %  P,Q - two histograms of size N
    %  D - the NxN matrix of the ground distance between bins of P and Q.
    %  Must be a metric. I recommend it to be a thresholded metric (which
    %  is also a metric, see ICCV paper).
    %
    % Optional Input:
    %  extra_mass_penalty - the penalty for extra mass. If you want the
    %                       resulting distance to be a metric, it should be
    %                       at least half the diameter of the space (maximum
    %                       possible distance between any two points). If you
    %                       want partial matching you can set it to zero (but
    %                       then the resulting distance is not guaranteed to be a metric).
    %                       Default value is -1 which means 1*max(D(:)).
    %  FType - type of flow that is returned.
    %          1 - represent no flow (then F should also not be requested, that is the
    %          call should be: dist= emd_hat_gd...
    %          2 - fills F with the flows between bins connected
    %              with edges smaller than max(D(:)).
    %          3 - fills F with the flows between all bins, except the flow
    %              to the extra mass bin.
    %          Defaults: 1 if F is not requested, 2 otherwise
    */
}


///////////////////////////////////////////////////////////////////////////////////////

/*

// Approximate Earth Mover's Distance (EMD) between two 2D heatmaps
//  - What it is: Fast approximation using cumulative distribution comparison
//  - Use case: When you need quick comparisons and approximate results are sufficient
//  - Computational complexity: O(n×m) where n,m are grid dimensions
float earthMoversDistanceApprox(const std::vector<std::vector<float>>& heatmap1,
                               const std::vector<std::vector<float>>& heatmap2) {
    if (heatmap1.size() != heatmap2.size() ||
        (heatmap1.size() > 0 && heatmap1[0].size() != heatmap2[0].size())) {
        msg_error_and_exit("Heatmaps must have the same dimensions for EMD calculation");
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
//  - What it is: True EMD implementation using optimal transport principles
//  - Method: Greedy transport algorithm with Manhattan distance as ground metric
//  - Use case: When you need mathematically accurate EMD results
//  - Computational complexity: O(k²×s) where k is number of non-zero cells, s is iterations
//
// Note: This "Full" implementation uses a greedy approximation rather than the optimal Hungarian
// algorithm for computational efficiency. For truly optimal EMD, you'd need specialized libraries
// like LEMON or CPLEX.
float earthMoversDistanceFull(const std::vector<std::vector<float>>& heatmap1,
                             const std::vector<std::vector<float>>& heatmap2) {
    if (heatmap1.size() != heatmap2.size() ||
        (heatmap1.size() > 0 && heatmap1[0].size() != heatmap2[0].size())) {
        msg_error_and_exit("Heatmaps must have the same dimensions for EMD calculation");
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

*/

} // namespace Polybee
