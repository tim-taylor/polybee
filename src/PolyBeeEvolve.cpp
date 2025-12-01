/**
 * @file
 *
 * Implementation of the PolyBeeEvolve class
 */

#include "PolyBeeEvolve.h"
#include "PolyBeeCore.h"
#include "Params.h"
#include <numbers>
#include <pagmo/types.hpp>
#include <pagmo/problem.hpp>
#include <pagmo/population.hpp>
#include <pagmo/algorithm.hpp>
//#include <pagmo/algorithms/de1220.hpp> // not suitable for stochastic problems
//#include <pagmo/algorithms/sade.hpp>   // not suitable for stochastic problems
//#include <pagmo/algorithms/gaco.hpp>
//#include <pagmo/algorithms/pso_gen.hpp>
#include <pagmo/algorithms/sga.hpp>
//#include <pagmo/algorithms/cmaes.hpp> // produces an error (tries to use side=5) at end of first generation
//#include <pagmo/algorithms/xnes.hpp>  // produces an error (tries to use side=5) at end of first generation
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <format>

int PolyBeeHeatmapOptimization::eval_counter = 0; // initialise static member


/*
pagmo::vector_double PolyBeeHeatmapOptimization::fitness(const pagmo::vector_double &dv) const
{
    std::vector<double> fitnessValues;
    PolyBeeCore& core = m_pPolyBeeEvolve->polyBeeCore();

    //assert(dv.size() == 2); // we expect 1 decision variables
    //Params::beeMaxDirDelta = static_cast<float>(dv[0]);
    //Params::numBees = static_cast<int>(dv[1]);
    assert(dv.size() == 1); // we expect 1 decision variables
    Params::beeStepLength = static_cast<float>(dv[0]);

    //if (Params::numBees < 1) { // ensure at least one bee after rounding
    //    Params::numBees = 1;
    //}

    for (int i = 0; i < Params::numTrialsPerConfig; ++i) {
        ++eval_counter;
        core.resetForNewRun();
        core.run(false); // false = do not log output files during the run
        const Heatmap& runHeatmap = core.getHeatmap();
        double emd = runHeatmap.emd(m_pPolyBeeEvolve->targetHeatmap());
        fitnessValues.push_back(emd);
    }

    double mean_emd = std::accumulate(fitnessValues.begin(), fitnessValues.end(), 0.0) / fitnessValues.size();

    int num_evals_per_gen = Params::numConfigsPerGen * Params::numTrialsPerConfig;
    int gen = (eval_counter-1) / num_evals_per_gen;
    int eval_in_gen = (eval_counter-1) % num_evals_per_gen;
    int config_num = eval_in_gen / Params::numTrialsPerConfig;

    //pb::msg_info(std::format("Gen {} eval_ctr {} config_num {}: dirdelta {:.4f}, numBees {}, meanEMD {:.4f}",
    //    gen, eval_counter, config_num, Params::beeMaxDirDelta, Params::numBees, mean_emd));
    pb::msg_info(std::format("Gen {} eval_ctr {} config_num {}: beeStepLength {:.4f}, meanEMD {:.4f}",
        gen, eval_counter, config_num, Params::beeStepLength, mean_emd));

    return {mean_emd};
}

std::pair<pagmo::vector_double, pagmo::vector_double> PolyBeeHeatmapOptimization::get_bounds() const
{
    //return {{0., 1.}, {std::numbers::pi_v<double>, 20.}}; // beeMaxDirDelta range, numBees range
    return {{0.0}, {30.0}}; // beeStepLength range
}
*/


// Implementation of the objective function.
pagmo::vector_double PolyBeeHeatmapOptimization::fitness(const pagmo::vector_double &dv) const
{
    std::vector<double> fitnessValues;
    PolyBeeCore& core = m_pPolyBeeEvolve->polyBeeCore();
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
    if (eval_counter == 0) {
        core.writeConfigFile();
        std::cout << "~~~~~~~~~~" << std::endl;
    }

    for (int i = 0; i < Params::numTrialsPerConfig; ++i) {
        ++eval_counter;
        core.resetForNewRun();
        core.run(false); // false = do not log output files during the run
        const Heatmap& runHeatmap = core.getHeatmap();
        //double emd = runHeatmap.emd(m_pPolyBeeEvolve->targetHeatmap());
        double emd = runHeatmap.emd(env.getRawTargetHeatmapNormalised());
        fitnessValues.push_back(emd);
    }

    double mean_emd = std::accumulate(fitnessValues.begin(), fitnessValues.end(), 0.0) / fitnessValues.size();

    int num_evals_per_gen = Params::numConfigsPerGen * Params::numTrialsPerConfig;
    int gen = (eval_counter-1) / num_evals_per_gen;
    int eval_in_gen = (eval_counter-1) % num_evals_per_gen;
    int config_num = eval_in_gen / Params::numTrialsPerConfig;

    pb::msg_info(std::format("Gen {} eval_ctr {} config_num {}: entrances e1:{},{}:{} e2:{},{}:{} e3:{},{}:{} e4:{},{}:{}, meanEMD {:.4f}",
        gen, eval_counter, config_num,
        Params::tunnelEntranceSpecs[0].e1, Params::tunnelEntranceSpecs[0].e2, Params::tunnelEntranceSpecs[0].side,
        Params::tunnelEntranceSpecs[1].e1, Params::tunnelEntranceSpecs[1].e2, Params::tunnelEntranceSpecs[1].side,
        Params::tunnelEntranceSpecs[2].e1, Params::tunnelEntranceSpecs[2].e2, Params::tunnelEntranceSpecs[2].side,
        Params::tunnelEntranceSpecs[3].e1, Params::tunnelEntranceSpecs[3].e2, Params::tunnelEntranceSpecs[3].side,
        mean_emd));

    return {mean_emd};
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


PolyBeeEvolve::PolyBeeEvolve(PolyBeeCore& core) : m_polyBeeCore(core) {
    /*
    // Read in the target heatmap and store it in m_targetHeatmap
    loadTargetHeatmap(Params::strTargetHeatmapFilename);
    */
};


void PolyBeeEvolve::evolve() {
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
    unsigned int algo_seed = static_cast<unsigned int>(PolyBeeCore::m_sUniformIntDistrib(PolyBeeCore::m_sRngEngine));
    algo.set_seed(algo_seed);

    // 3 - Instantiate a population
    // (again, taking care to seed the population RNG from our own RNG)
    unsigned int pop_seed = static_cast<unsigned int>(PolyBeeCore::m_sUniformIntDistrib(PolyBeeCore::m_sRngEngine));

    pagmo::population pop{prob, static_cast<unsigned int>(Params::numConfigsPerGen), pop_seed};

    // 4 - Evolve the population
    pop = algo.evolve(pop);

    // 5 - Output the population
    writeResultsFile(algo, pop, true);
}


void PolyBeeEvolve::writeResultsFile(const pagmo::algorithm& algo, const pagmo::population& pop, bool alsoToStdout) const
{
    // write evolution results to file
    std::string resultsFilename = std::format("{0}/{1}evo-results-{2}.cfg",
        Params::logDir,
        Params::logFilenamePrefix.empty() ? "" : (Params::logFilenamePrefix + "-"),
        m_polyBeeCore.getTimestampStr());

    std::ofstream resultsFile(resultsFilename);

    if (!resultsFile) {
        if (!resultsFile) {
            pb::msg_warning(
                std::format("Unable to open evol-results output file {} for writing. Results will not be saved to file, printing to stdout instead.",
                    resultsFilename));
        }
        std::cout << "~~~~~~~~~~ EVOLUTION RESULTS ~~~~~~~~~~";
        writeResultsFileHelper(std::cout, algo, pop);
    }
    else {
        writeResultsFileHelper(resultsFile, algo, pop);
        resultsFile.close();
        pb::msg_info(std::format("Evolution results written to file: {}", resultsFilename));
        if (alsoToStdout) {
            std::cout << "~~~~~~~~~~ EVOLUTION RESULTS ~~~~~~~~~~";
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


/*
void PolyBeeEvolve::loadTargetHeatmap(const std::string& filename) {
    // Check if file exists and can be opened
    std::ifstream file(filename);
    if (!file.is_open()) {
        pb::msg_error_and_exit("Cannot open target heatmap file: " + filename);
    }

    // Get expected dimensions from the core heatmap
    const Heatmap& coreHeatmap = m_polyBeeCore.getHeatmap();
    int expectedWidth = coreHeatmap.size_x();
    int expectedHeight = coreHeatmap.size_y();

    // Read CSV data
    std::vector<std::vector<double>> targetData;
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty()) continue; // Skip empty lines

        std::vector<double> row;
        std::stringstream ss(line);
        std::string cell;

        while (std::getline(ss, cell, ',')) {
            try {
                double value = std::stod(cell);
                row.push_back(value);
            } catch (const std::exception& e) {
                pb::msg_error_and_exit("Invalid numeric value in target heatmap CSV file: " + cell);
            }
        }

        if (!row.empty()) {
            targetData.push_back(row);
        }
    }

    file.close();

    // Check dimensions match
    if (targetData.empty()) {
        pb::msg_error_and_exit("Target heatmap file is empty: " + filename);
    }

    int actualHeight = static_cast<int>(targetData.size());
    int actualWidth = static_cast<int>(targetData[0].size());

    if (actualHeight != expectedHeight || actualWidth != expectedWidth) {
        pb::msg_error_and_exit(
            std::format("Target heatmap dimensions ({}x{} do not match core heatmap dimensions ({}x{})",
                actualWidth, actualHeight, expectedWidth, expectedHeight));
    }

    // Check all rows have the same width
    for (size_t i = 0; i < targetData.size(); ++i) {
        if (static_cast<int>(targetData[i].size()) != expectedWidth) {
            pb::msg_error_and_exit(std::format("Inconsistent row width in CSV file at row {}", i + 1));
        }
    }

    // Transpose the matrix before storing, because we access these matrices as [x][y] elsewhere
    std::vector<std::vector<double>> transposed(expectedWidth, std::vector<double>(expectedHeight));
    for (int i = 0; i < expectedHeight; ++i) {
        for (int j = 0; j < expectedWidth; ++j) {
            transposed[j][i] = targetData[i][j];
        }
    }

    // Store the transposed data
    m_targetHeatmap = std::move(transposed);
}
*/