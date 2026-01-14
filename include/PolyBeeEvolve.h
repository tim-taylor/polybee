/**
 * @file
 *
 * Declaration of the PolyBeeEvolve class
 */

#ifndef _POLYBEEEVOLVE_H
#define _POLYBEEEVOLVE_H

#include "PolyBeeCore.h"
#include <pagmo/population.hpp>
#include <pagmo/algorithm.hpp>
#include <pagmo/archipelago.hpp>
#include <vector>
#include <memory>
#include <ostream>

class PolyBeeEvolve;

// User Defined Problem struct for pagmo (must be copyable)
struct PolyBeeHeatmapOptimization {

    PolyBeeHeatmapOptimization(std::size_t islandNum = 0) : m_pPolyBeeEvolve{nullptr}, m_islandNum(islandNum) {}
    PolyBeeHeatmapOptimization(PolyBeeEvolve* ptr, std::size_t islandNum = 0) : m_pPolyBeeEvolve(ptr), m_islandNum(islandNum) {}
    PolyBeeHeatmapOptimization(const PolyBeeHeatmapOptimization& other) : m_pPolyBeeEvolve(other.m_pPolyBeeEvolve), m_islandNum(other.m_islandNum) {}

    // Implementation of the objective function.
    pagmo::vector_double fitness(const pagmo::vector_double &dv) const;

    // Implementation of the box bounds.
    std::pair<pagmo::vector_double, pagmo::vector_double> get_bounds() const;

    // Define the number of integer (as opposed to continuous) decision variables
    inline pagmo::vector_double::size_type get_nix() const;

    // Pointer back to the PolyBeeEvolve object
    PolyBeeEvolve* m_pPolyBeeEvolve;

    // Island number for this problem instance
    std::size_t m_islandNum;
};


/**
 * The PolyBeeEvolve class ...
 */
class PolyBeeEvolve {

public:
    PolyBeeEvolve(PolyBeeCore& core);
    ~PolyBeeEvolve() {}

    void evolve(); // run optimization

    PolyBeeCore& polyBeeCore(std::size_t islandNum = 0);

private:
    void evolveSinglePop();
    void evolveArchipelago();
    void writeResultsFile(const pagmo::algorithm& algo, const pagmo::population& pop, bool alsoToStdout) const;
    void writeResultsFileHelper(std::ostream& os, const pagmo::algorithm& algo, const pagmo::population& pop) const;
    void writeResultsFileArchipelago(const pagmo::archipelago& arc, bool alsoToStdout) const;
    void writeResultsFileArchipelagoHelper(std::ostream& os, const pagmo::archipelago& arc) const;

    PolyBeeCore& m_masterPolyBeeCore;
    std::vector<std::unique_ptr<PolyBeeCore>> m_islandPolyBeeCores; // one per island
};

#endif /* _POLYBEEEVOLVE_H */
