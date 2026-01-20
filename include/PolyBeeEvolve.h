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
#include <pagmo/rng.hpp>

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


// User Defined Replacement Policy (UDRP) for pagmo - random replacement
// Replaces randomly selected individuals with migrants regardless of fitness
struct replace_random {
    // Default constructor (required by Pagmo)
    replace_random() : m_pCore(nullptr), m_rate(1) {}

    // Constructor with PolyBeeCore reference and number of individuals to replace
    replace_random(PolyBeeCore& core, pagmo::pop_size_t rate) : m_pCore(&core), m_rate(rate) {}

    // The replace method - core of the replacement policy
    pagmo::individuals_group_t replace(
        const pagmo::individuals_group_t &inds,
        const pagmo::vector_double::size_type &nx,
        const pagmo::vector_double::size_type &nix,
        const pagmo::vector_double::size_type &nobj,
        const pagmo::vector_double::size_type &nec,
        const pagmo::vector_double::size_type &nic,
        const pagmo::vector_double &tol,
        const pagmo::individuals_group_t &mig
    ) const;

    // Get the name of the policy
    std::string get_name() const { return "Random replacement"; }

    // Human-readable extra info
    std::string get_extra_info() const;

private:
    PolyBeeCore* m_pCore; // pointer to PolyBeeCore for RNG access
    pagmo::pop_size_t m_rate; // max number of individuals that can be replaced
};


// User Defined Selection Policy (UDSP) for pagmo - random selection
// Selects individuals at random for migration rather than selecting the best
struct select_random {
    // Default constructor (required by Pagmo)
    select_random() : m_pCore(nullptr), m_rate(1) {}

    // Constructor with PolyBeeCore reference and number of individuals to select
    select_random(PolyBeeCore& core, pagmo::pop_size_t rate) : m_pCore(&core), m_rate(rate) {}

    // The select method - core of the selection policy
    pagmo::individuals_group_t select(
        const pagmo::individuals_group_t &inds,
        const pagmo::vector_double::size_type &nx,
        const pagmo::vector_double::size_type &nix,
        const pagmo::vector_double::size_type &nobj,
        const pagmo::vector_double::size_type &nec,
        const pagmo::vector_double::size_type &nic,
        const pagmo::vector_double &tol
    ) const;

    // Get the name of the policy
    std::string get_name() const { return "Random selection"; }

    // Human-readable extra info
    std::string get_extra_info() const;

private:
    PolyBeeCore* m_pCore; // pointer to PolyBeeCore for RNG access
    pagmo::pop_size_t m_rate; // max number of individuals that can be selected
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
    void showBestIndividuals(const pagmo::archipelago& arc, int gen) const;

    PolyBeeCore& m_masterPolyBeeCore;
    std::vector<std::unique_ptr<PolyBeeCore>> m_islandPolyBeeCores; // one per island
};

#endif /* _POLYBEEEVOLVE_H */
