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
#include <pagmo/algorithms/de1220.hpp>
//#include <pagmo/algorithms/sade.hpp>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <format>

int PolyBeeHeatmapOptimization::eval_counter = 0; // initialise static member


// Implementation of the objective function.
pagmo::vector_double PolyBeeHeatmapOptimization::fitness(const pagmo::vector_double &dv) const
{
    std::vector<double> fitnessValues;
    PolyBeeCore& pbc = m_pPolyBeeEvolve->polyBeeCore();

    assert(dv.size() == 2); // we expect 1 decision variables
    Params::beeMaxDirDelta = static_cast<float>(dv[0]);
    Params::numBees = static_cast<int>(dv[1]);
    if (Params::numBees < 1) { // ensure at least one bee after rounding
        Params::numBees = 1;
    }

    for (int i = 0; i < Params::numTrialsPerConfig; ++i) {
        ++eval_counter;
        pbc.resetForNewRun();
        pbc.run(false); // false = do not log output files during the run
        const Heatmap& runHeatmap = pbc.getHeatmap();
        //double emd = runHeatmap.emd_approx(m_pPolyBeeEvolve->targetHeatmap());
        //double emd = runHeatmap.emd_full(m_pPolyBeeEvolve->targetHeatmap());
        double emd = runHeatmap.emd_hat(m_pPolyBeeEvolve->targetHeatmap());
        fitnessValues.push_back(emd);
    }

    double mean_emd = std::accumulate(fitnessValues.begin(), fitnessValues.end(), 0.0) / fitnessValues.size();

    int num_evals_per_gen = Params::numConfigsPerGen * Params::numTrialsPerConfig;
    int gen = (eval_counter-1) / num_evals_per_gen;
    int eval_in_gen = (eval_counter-1) % num_evals_per_gen;
    int config_num = eval_in_gen / Params::numTrialsPerConfig;

    pb::msg_info(std::format("Gen {} eval_ctr {} config_num {}: dirdelta {:.4f}, numBees {}, meanEMD {:.4f}",
        gen, eval_counter, config_num, Params::beeMaxDirDelta, Params::numBees, mean_emd));

    return {mean_emd};
}


// Implementation of the box bounds.
std::pair<pagmo::vector_double, pagmo::vector_double> PolyBeeHeatmapOptimization::get_bounds() const
{
    return {{0., 1.}, {std::numbers::pi_v<double>, 20.}}; // beeMaxDirDelta range, numBees range
}


PolyBeeEvolve::PolyBeeEvolve(PolyBeeCore& core) : m_polyBeeCore(core) {
    // Read in the target heatmap and store it in m_targetHeatmap
    loadTargetHeatmap(Params::strTargetHeatmapFilename);
};


void PolyBeeEvolve::evolve() {
    // 1 - Instantiate a pagmo problem constructing it from a UDP
    // (user defined problem).
    pagmo::problem prob{PolyBeeHeatmapOptimization{this}};

    // 2 - Instantiate a pagmo algorithm
    // (here we use the DE 122- differential evolution algorithm with default parameters)
    pagmo::algorithm algo{pagmo::de1220(Params::numGenerations-1)}; // -1 because the initial population counts as a generation
    //pagmo::algorithm algo{pagmo::sade(Params::numGenerations-1)}; // -1 because the initial population counts as a generation

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
    std::cout << "The population: \n" << pop;

}


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

    // Store the validated data
    m_targetHeatmap = std::move(targetData);

    /*
    std::cout << "Successfully loaded target heatmap from " << filename
              << " (dimensions: " << expectedWidth << "x" << expectedHeight << ")" << std::endl;
    */
}
