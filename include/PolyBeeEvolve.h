/**
 * @file
 *
 * Declaration of the PolyBeeEvolve class
 */

#ifndef _POLYBEEEVOLVE_H
#define _POLYBEEEVOLVE_H

#include "PolyBeeCore.h"
#include <pagmo/types.hpp>
#include <vector>

class PolyBeeEvolve;

// User Defined Problem struct for pagmo (must be copyable)
struct PolyBeeHeatmapOptimization {

    PolyBeeHeatmapOptimization() : m_pPolyBeeEvolve{nullptr} {}
    PolyBeeHeatmapOptimization(PolyBeeEvolve* ptr) : m_pPolyBeeEvolve(ptr) {}
    PolyBeeHeatmapOptimization(const PolyBeeHeatmapOptimization& other) : m_pPolyBeeEvolve(other.m_pPolyBeeEvolve) {}

    // Implementation of the objective function.
    pagmo::vector_double fitness(const pagmo::vector_double &dv) const;

    // Implementation of the box bounds.
    std::pair<pagmo::vector_double, pagmo::vector_double> get_bounds() const;

    // Define the number of integer (as opposed to continuous) decision variables
    inline pagmo::vector_double::size_type get_nix() const;

    // Pointer back to the PolyBeeEvolve instance
    PolyBeeEvolve* m_pPolyBeeEvolve;

    static int eval_counter; // static counter of number of evaluations performed
};


/**
 * The PolyBeeEvolve class ...
 */
class PolyBeeEvolve {

public:
    PolyBeeEvolve(PolyBeeCore& core);
    ~PolyBeeEvolve() {}

    void evolve(); // run optimization

    PolyBeeCore& polyBeeCore() { return m_polyBeeCore; }
    const std::vector<std::vector<double>>& targetHeatmap() const { return m_targetHeatmap; }

private:
    void loadTargetHeatmap(const std::string& filename);

    PolyBeeCore& m_polyBeeCore;

    std::vector<std::vector<double>> m_targetHeatmap; // the target heatmap we are trying to evolve towards
};

#endif /* _POLYBEEEVOLVE_H */
