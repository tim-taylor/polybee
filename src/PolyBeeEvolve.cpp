/**
 * @file
 *
 * Implementation of the PolyBeeEvolve class
 */

#include "PolyBeeEvolve.h"
#include "PolyBeeCore.h"
#include "Params.h"
#include "utils.h"
#include <numbers>
#include <pagmo/types.hpp>
#include <pagmo/problem.hpp>
#include <pagmo/population.hpp>
#include <pagmo/archipelago.hpp>
#include <pagmo/topology.hpp>
#include <pagmo/topologies/ring.hpp>
#include <pagmo/topologies/unconnected.hpp>
#include <pagmo/r_policy.hpp>
#include <pagmo/r_policies/fair_replace.hpp>
#include <pagmo/s_policy.hpp>
#include <pagmo/s_policies/select_best.hpp>
#include <pagmo/island.hpp>
#include <pagmo/algorithm.hpp>
#include <pagmo/algorithms/sga.hpp>
//#include <pagmo/algorithms/de1220.hpp> // not suitable for stochastic problems
//#include <pagmo/algorithms/sade.hpp>   // not suitable for stochastic problems
//#include <pagmo/algorithms/gaco.hpp>
//#include <pagmo/algorithms/pso_gen.hpp>
//#include <pagmo/algorithms/cmaes.hpp> // produces an error (tries to use side=5) at end of first generation
//#include <pagmo/algorithms/xnes.hpp>  // produces an error (tries to use side=5) at end of first generation
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <format>
#include <cassert>


// Implementation of the objective function.
pagmo::vector_double PolyBeeHeatmapOptimization::fitness(const pagmo::vector_double &dv) const
{
    std::vector<double> fitnessValues;
    PolyBeeCore& core = m_pPolyBeeEvolve->polyBeeCore(m_islandNum);
    const Environment& env = core.getEnvironment();

    float entranceWidth = 100.0f; // fixed entrance width of 100 units

    assert(dv.size() == 8); // we expect 8 decision variables
    assert(entranceWidth < Params::tunnelW && entranceWidth < Params::tunnelH);

    Params::tunnelEntranceSpecs.clear();

    std::vector<float> tunnelLengths = {
        Params::tunnelW - entranceWidth, // North
        Params::tunnelH - entranceWidth, // East
        Params::tunnelW - entranceWidth, // South
        Params::tunnelH - entranceWidth  // West
    };

    for (int i = 0; i < 4; ++i) {
        int side = static_cast<int>(dv[4+i]);
        float e1 = static_cast<float>(dv[i]) * tunnelLengths[side];
        float e2 = e1 + entranceWidth;
        Params::tunnelEntranceSpecs.emplace_back(e1, e2, side);
    }
    core.getTunnel().initialiseEntrances();

    // Write config file for this configuration once at the start of the run
    if (core.isMasterCore()  && core.evaluationCount() == 0) {
        core.writeConfigFile();
        std::cout << "~~~~~~~~~~" << std::endl;
    }

    for (int i = 0; i < Params::numTrialsPerConfig; ++i) {
        core.incrementEvaluationCount();
        core.resetForNewRun();
        core.run(false); // false = do not log output files during the run
        const Heatmap& runHeatmap = core.getHeatmap();
        double emd = runHeatmap.emd(env.getRawTargetHeatmapNormalised());
        fitnessValues.push_back(emd);
    }

    //double mean_emd = std::accumulate(fitnessValues.begin(), fitnessValues.end(), 0.0) / fitnessValues.size();
    const double median_emd = pb::median(fitnessValues);

    int num_evals_per_gen = Params::numConfigsPerGen * Params::numTrialsPerConfig;
    int gen = (core.evaluationCount()-1) / num_evals_per_gen;
    int eval_in_gen = (core.evaluationCount()-1) % num_evals_per_gen;
    int config_num = eval_in_gen / Params::numTrialsPerConfig;

    pb::msg_info(std::format("Island {} gen {} evals {} config_num {}: entrances e1:{},{}:{} e2:{},{}:{} e3:{},{}:{} e4:{},{}:{}, medianEMD {:.4f}",
        core.getIslandNum(), gen, core.evaluationCount(), config_num,
        Params::tunnelEntranceSpecs[0].e1, Params::tunnelEntranceSpecs[0].e2, Params::tunnelEntranceSpecs[0].side,
        Params::tunnelEntranceSpecs[1].e1, Params::tunnelEntranceSpecs[1].e2, Params::tunnelEntranceSpecs[1].side,
        Params::tunnelEntranceSpecs[2].e1, Params::tunnelEntranceSpecs[2].e2, Params::tunnelEntranceSpecs[2].side,
        Params::tunnelEntranceSpecs[3].e1, Params::tunnelEntranceSpecs[3].e2, Params::tunnelEntranceSpecs[3].side,
        median_emd));

    return {median_emd};
}


// Implementation of the box bounds.
std::pair<pagmo::vector_double, pagmo::vector_double> PolyBeeHeatmapOptimization::get_bounds() const
{
    return {{0.0, 0.0, 0.0, 0.0, 0, 0, 0, 0}, {1.0, 1.0, 1.0, 1.0, 3, 3, 3, 3}};
}


// Define the number of integer (as opposed to continuous) decision variables
pagmo::vector_double::size_type PolyBeeHeatmapOptimization::get_nix() const {
    return 4; // last 4 decision variables are integers
}


PolyBeeEvolve::PolyBeeEvolve(PolyBeeCore& core) : m_masterPolyBeeCore(core) {
    /*
    // Read in the target heatmap and store it in m_targetHeatmap
    loadTargetHeatmap(Params::strTargetHeatmapFilename);
    */
};


void PolyBeeEvolve::evolve()
{
    if (Params::numIslands <= 1) {
        evolveSinglePop();
    } else {
        evolveArchipelago();
    }
}


PolyBeeCore& PolyBeeEvolve::polyBeeCore(std::size_t islandNum)
{
    if (islandNum == 0) {
        return m_masterPolyBeeCore;
    } else {
        // NB Indices in this vector are offset by 1 compared to island number (island 0 is the master PolyBeeCore)!
        assert(islandNum - 1 < m_islandPolyBeeCores.size());
        return *m_islandPolyBeeCores[islandNum - 1];
    }
}


void PolyBeeEvolve::evolveSinglePop() {
    // 1 - Instantiate a pagmo problem constructing it from a UDP
    // (user defined problem).
    pagmo::problem prob{PolyBeeHeatmapOptimization{this}};

    // 2 - Instantiate a pagmo algorithm
    // For info on available algorithms see: https://esa.github.io/pagmo2/overview.html
    int numGensForAlgo = Params::numGenerations - 1; // -1 because Pagmo doesn't count the initial population evaluation as a generation
    //pagmo::algorithm algo{pagmo::de1220(numGensForAlgo)}; // not suitable for stochastic problems
    //pagmo::algorithm algo{pagmo::sade(numGensForAlgo)};   // not suitable for stochastic problems
    //pagmo::algorithm algo {pagmo::gaco(numGensForAlgo)};
    //pagmo::algorithm algo {pagmo::pso_gen(numGensForAlgo)};
    pagmo::algorithm algo {pagmo::sga(numGensForAlgo)};
    //pagmo::algorithm algo {pagmo::cmaes(numGensForAlgo)}; // produces an error (tries to use side=5) at end of first generation
    //pagmo::algorithm algo {pagmo::xnes(numGensForAlgo)};  // produces an error (tries to use side=5) at end of first generation

    // ensure that the Pagmo RNG seed is determined by our own RNG, so runs can be reproduced
    // by just ensuring we use the same seed for our own RNG
    unsigned int algo_seed = static_cast<unsigned int>(m_masterPolyBeeCore.m_uniformIntDistrib(m_masterPolyBeeCore.m_rngEngine));
    algo.set_seed(algo_seed);

    // 3 - Instantiate a population
    // (again, taking care to seed the population RNG from our own RNG)
    unsigned int pop_seed = static_cast<unsigned int>(m_masterPolyBeeCore.m_uniformIntDistrib(m_masterPolyBeeCore.m_rngEngine));

    pagmo::population pop{prob, static_cast<unsigned int>(Params::numConfigsPerGen), pop_seed};

    // 4 - Evolve the population
    pop = algo.evolve(pop);

    // 5 - Output the population
    writeResultsFile(algo, pop, true);
}


void PolyBeeEvolve::evolveArchipelago()
{
    assert(Params::numIslands > 0);
    assert(Params::migrationPeriod > 0);

    // 2. Create an archipelago with multiple islands
    pagmo::archipelago arc;

    // 4. Set up topology for migration (how islands are connected)
    // Using a ring topology: each island connects to its neighbors
    //
    // Topology options are:
    // * Unconnected
    // * Fully connected
    // * Base BGL (Boost Graph Libary) - requires further implementation
    // * Ring
    // * Free-form
    arc.set_topology(pagmo::topology{pagmo::ring{}});
    //
    // Migration type options are:
    // * p2p
    // * broadcast
    arc.set_migration_type(pagmo::migration_type::p2p);
    //
    // Migration handing options are:
    // * preserve (a single migrant can get copied to multiple other islands)
    // * evict    (a single migrant can only get copied to one different island)
    arc.set_migrant_handling(pagmo::migrant_handling::evict);


    for (size_t i = 0; i < Params::numIslands; ++i)
    {
        if (i > 0){
            // Create a copy of the master PolyBeeCore for this island
            // First, create a unique seed string for this island, but derived from the master RNG seed string
            std::string islandSeedStr = Params::strRngSeed + std::to_string(i);
            // NB Indices in this vector are offset by 1 compared to island number (island 0 is the master PolyBeeCore)!
            m_islandPolyBeeCores.push_back(std::make_unique<PolyBeeCore>(m_masterPolyBeeCore, islandSeedStr));
        }

        // 1 - Instantiate a pagmo problem constructing it from a UDP
        // (user defined problem).
        pagmo::problem prob{PolyBeeHeatmapOptimization{this, i}};

        // 3a - Instantiate a pagmo algorithm
        pagmo::algorithm algo {pagmo::sga(Params::numGenerations)};

        // ensure that the Pagmo RNG seed is determined by our own RNG, so runs can be reproduced
        // by just ensuring we use the same seed for our own RNG
        unsigned int algo_seed = static_cast<unsigned int>(m_masterPolyBeeCore.m_uniformIntDistrib(m_masterPolyBeeCore.m_rngEngine));
        algo.set_seed(algo_seed);

        // 3b - Instantiate a population
        // (again, taking care to seed the population RNG from our own RNG)
        unsigned int pop_seed = static_cast<unsigned int>(m_masterPolyBeeCore.m_uniformIntDistrib(m_masterPolyBeeCore.m_rngEngine));

        pagmo::population pop{prob, static_cast<unsigned int>(Params::numConfigsPerGen), pop_seed};

        // 3c - Add the island to the archipelago
        //
        // Selection policy options are:
        // * Best
        //
        // Replacement polict options are:
        // * Fair
        arc.push_back(pagmo::island{algo,
            pop,
            pagmo::fair_replace{1}, // one individual in a population can be replaced by a migrant
            pagmo::select_best{1}   // one individual in a population can be selected for migration
        });
    }

    // Print connections for ring
    if (!Params::bCommandLineQuiet) {
        std::string msg = "Topology info:\n";
        msg += std::format(" type = {}\n",arc.get_topology().get_name());
        /*
        for (size_t i = 0; i < arc.size(); ++i) {
            auto weights_and_destinations = arc.get_topology().get_connections(i);
            msg += std::format("Island {} connects to:\n", i);
            size_t num_connections = weights_and_destinations.first.size();
            for (size_t j = 0; j < num_connections; ++j) {
                msg += std::format(" Island {} (weight {}) ", weights_and_destinations.first[j], weights_and_destinations.second[j]);
            }
            msg += "\n";
        }
        */
        pb::msg_info(msg);
    }

    // 5 - Evolve the archipelago
    int numCycles = Params::numGenerations / Params::migrationPeriod;
    int extraGens = Params::numGenerations - (numCycles * Params::migrationPeriod);

    /*
    for (int cycle = 0; cycle < numCycles; ++cycle) {
        arc.evolve(Params::migrationPeriod);  // evolve each island for this number of generations, then perform one migration event
        arc.wait_check();
    }

    // do some extra generations if numGenerations is not an exact divisor of migrationPeriod
    if (extraGens > 0) {
        // first set the topology to unconnected so that we don't do another migration event
        // at the end of these final generations
        arc.set_topology(pagmo::topology{pagmo::unconnected{}});
        arc.evolve(extraGens);
        arc.wait_check();
    }
    */

    /////////////////////////////////////////////////////

    int numGensBetweenMigrations = Params::migrationPeriod - 1;
    int globalGen = 0;

    for (int cycle = 0; cycle < numCycles; ++cycle) {
        std::cout << "\nMigration cycle " << cycle + 1 << std::endl;

        // Phase 1: Local evolution (no migration)
        std::cout << "  Running " << numGensBetweenMigrations << " local generations..." << std::endl;
        for (int gen = 0; gen < numGensBetweenMigrations; ++gen) {
            for (size_t i = 0; i < arc.size(); ++i) {
                std::cout << std::format("    Initiating generation {} on island {}...", gen, i) << std::endl;
                arc[i].evolve();
            }
            for (size_t i = 0; i < arc.size(); ++i) {
                arc[i].wait_check();
            }
            globalGen++;
        }

        // Phase 2: Migration event
        std::cout << "  Performing a generation with migration..." << std::endl;
        arc.evolve();
        arc.wait_check();
        globalGen++;

        // Show best fitness
        double best_f = std::numeric_limits<double>::max();
        size_t best_i = 0;
        for (size_t i = 0; i < arc.size(); ++i) {
            double island_best = arc[i].get_population().champion_f()[0];
            if (island_best < best_f) {
                best_f = island_best;
                best_i = i;
            }
        }
        std::cout << std::format("  Best fitness: {} (island {})", best_f, best_i) << std::endl;
    }

    std::cout << "\nFinal migration stats:" << std::endl;
    auto log = arc.get_migration_log();
    for (auto [ts, id, dv, fv, source, dest] : log) {
        double median_fitness = pb::median(fv);
        std::cout << std::format("  At time {:.2f} individual {} (median fitness {}) migrated from Island {} -> {}",
            ts, id, median_fitness, source, dest) << std::endl;
    }


    /////////////////////////////////////////////////////

    // 6 - Output the population
    writeResultsFileArchipelago(arc, true);
}


void PolyBeeEvolve::writeResultsFileArchipelago(const pagmo::archipelago& arc, bool alsoToStdout) const
{
    // TODO
}


void PolyBeeEvolve::writeResultsFile(const pagmo::algorithm& algo, const pagmo::population& pop, bool alsoToStdout) const
{
    // write evolution results to file
    std::string resultsFilename = std::format("{0}/{1}evo-results-{2}.cfg",
        Params::logDir,
        Params::logFilenamePrefix.empty() ? "" : (Params::logFilenamePrefix + "-"),
        m_masterPolyBeeCore.getTimestampStr());

    std::ofstream resultsFile(resultsFilename);

    if (!resultsFile) {
        if (!resultsFile) {
            pb::msg_warning(
                std::format("Unable to open evol-results output file {} for writing. Results will not be saved to file, printing to stdout instead.",
                    resultsFilename));
        }
        std::cout << "~~~~~~~~~~ EVOLUTION RESULTS ~~~~~~~~~~" << std::endl;
        writeResultsFileHelper(std::cout, algo, pop);
    }
    else {
        writeResultsFileHelper(resultsFile, algo, pop);
        resultsFile.close();
        pb::msg_info(std::format("Evolution results written to file: {}", resultsFilename));
        if (alsoToStdout) {
            std::cout << "~~~~~~~~~~ EVOLUTION RESULTS ~~~~~~~~~~" << std::endl;
            writeResultsFileHelper(std::cout, algo, pop);
        }
    }
}


// Helper method to write results to a given output stream
void PolyBeeEvolve::writeResultsFileHelper(std::ostream& os, const pagmo::algorithm& algo, const pagmo::population& pop) const
{
    os << "Using algorithm: " << algo.get_name() << std::endl;
    os << "The population: \n" << pop;
    os << "\n";
    os << "Champion individual: ";
    auto champ = pop.champion_x();
    for (size_t i = 0; i < champ.size(); ++i) {
        if (i > 0) { os << ", "; }
        os << champ[i];
    }
    os << "\n";
    os << "Champion fitness: " << pop.champion_f()[0] << std::endl;
}
