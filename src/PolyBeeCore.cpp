/**
 * @file
 *
 * Implementation of the PolyBeeCore class
 */

#include "PolyBeeCore.h"
#include "Params.h"
#include "LocalVis.h"
#include "Bee.h"
#include "utils.h"
#include <memory>
#include <ctime>
#include <cassert>
#include <algorithm>
#include <format>
#include <chrono>
#include <fstream>

// Initialise static members
bool PolyBeeCore::m_sbRngInitialised = false;
// - create our static random number generator engine
std::mt19937 PolyBeeCore::m_sRngEngine;
// - and define commonly used distributions
std::uniform_real_distribution<float> PolyBeeCore::m_sUniformProbDistrib(0.0, 1.0);
std::uniform_real_distribution<float> PolyBeeCore::m_sAngle2PiDistrib(0.0f, 2.0f * std::numbers::pi_v<float>);
std::uniform_int_distribution<int> PolyBeeCore::m_sUniformIntDistrib(0, std::numeric_limits<int>::max());



PolyBeeCore::PolyBeeCore(int argc, char* argv[]) :
    m_pLocalVis{nullptr}
{
    Params::initialise(argc, argv);

    seedRng();

    if (!Params::bCommandLineQuiet) {
        std::cout << "~~~~~~~~~~ FINAL PARAM VALUES ~~~~~~~~~~\n";
        Params::print(std::cout);
    }

    if (Params::hiveSpecs.empty()) {
        pb::msg_error_and_exit("No hive positions have been defined!");
    }

    // get a timestamp string for this run, used in output filenames
    generateTimestampString();

    // create the required number of bees, evenly distributed among the hives
    initialiseBees();

    // initialise environment
    m_env.initialise(&m_bees);

    // initialise heatmap
    m_heatmap.initialise(&m_bees);

    if (Params::bVis) {
        m_pLocalVis = std::unique_ptr<LocalVis>(new LocalVis(this));
    }
}

void PolyBeeCore::generateTimestampString()
{
    // create a timestamp string for this run, used in output filenames
    // Format: YYYYMMDD-HHMMSS-XXXXXX where XXXXXX is a random
    // string of 6 hex digits to help ensure uniqueness

    auto now = std::chrono::system_clock::now();
    auto truncated = std::chrono::floor<std::chrono::seconds>(now);
    m_timestampStr = std::format("{:%Y%m%d-%H%M%S-}", truncated);
    std::uniform_int_distribution<int> distrib(0, 15);
    for (int i = 0; i < 6; ++i) {
        m_timestampStr += std::format("{:x}", distrib(PolyBeeCore::m_sRngEngine));
    }
}

// a private helper method to create the bees at the start of the simulation
void PolyBeeCore::initialiseBees()
{
    auto numHives = Params::hiveSpecs.size();
    int numBeesPerHive = Params::numBees / numHives;
    for (int i = 0; i < numHives; ++i) {
        const HiveSpec& hive = Params::hiveSpecs[i];
        float angle = 0.0f;
        switch (hive.direction) {
        case 0: angle = -std::numbers::pi_v<float> / 2.0f; break; // North
        case 1: angle = 0.0f; break; // East
        case 2: angle = std::numbers::pi_v<float> / 2.0f; break; // South
        case 3: angle = std::numbers::pi_v<float>; break; // West
        case 4: angle = 0.0f; break; // Random (will be set per bee below)
        default:
            pb::msg_error_and_exit(std::format("Invalid hive direction {} specified for hive at ({},{}). Must be 0=North, 1=East, 2=South, 3=West, or 4=Random.",
                hive.direction, hive.x, hive.y));
        }

        for (int j = 0; j < numBeesPerHive; ++j) {
            float beeAngle = angle;
            if (hive.direction == 4) {
                // Random direction: uniform random angle between 0 and 2π
                beeAngle = m_sAngle2PiDistrib(m_sRngEngine);
            }
            m_bees.emplace_back(fPos(hive.x, hive.y), beeAngle, &m_env);
        }
    }

    if (numBeesPerHive * numHives < Params::numBees) {
        pb::msg_warning(std::format("Number of bees ({0}) is not a multiple of number of hives ({1}). Created {2} bees instead of the requested {0}.",
            Params::numBees, numHives, numBeesPerHive * numHives));
    }
}


void PolyBeeCore::run(bool logIfRequested)
{
    // main simulation loop
    while (!stopCriteriaReached()) {
        if (!m_bPaused) {
            ++m_iIteration;

            // update bee pos
            for (Bee& bee : m_bees) {
                bee.move();
            }

            m_heatmap.update();
        }

        if (Params::bVis && m_pLocalVis) {
            m_pLocalVis->updateDrawFrame();
        }
    }

    // log output files if the user has requested it AND the caller of the method has requested it
    if (logIfRequested && Params::logging) {
        writeOutputFiles();
    }

    // if visualisation is enabled, keep the window open until it is closed by the user
    if (m_pLocalVis  && !m_bEarlyExitRequested) {
        m_pLocalVis->continueUntilClosed();
    }
}


void::PolyBeeCore::resetForNewRun()
{
    // TODO - work in progress
    m_iIteration = -1;
    m_bEarlyExitRequested = false;
    m_bPaused = false;

    // reset bees
    m_bees.clear();
    initialiseBees();

    // reset environment
    m_env.reset();

    // reset heatmap
    m_heatmap.reset();
}


void PolyBeeCore::writeOutputFiles()
{
    // write config to file
    std::string configFilename = std::format("{0}/{1}config-{2}.cfg",
        Params::logDir,
        Params::logFilenamePrefix.empty() ? "" : (Params::logFilenamePrefix + "-"),
        m_timestampStr);
    std::ofstream configFile(configFilename);
    if (!configFile) {
        pb::msg_warning(
            std::format("Unable to open config output file {} for writing. Config will not be saved to file, printing to stdout instead.",
                configFilename));
        std::cout << "~~~~~~~~~~ FINAL PARAM VALUES ~~~~~~~~~~";
        Params::print(std::cout);
    }
    else {
        Params::print(configFile, true);
        configFile.close();
        pb::msg_info(std::format("Config output written to file: {}", configFilename));
    }

    // write heatmap to file
    std::string heatmapFilename = std::format("{0}/{1}heatmap-{2}.csv",
        Params::logDir,
        Params::logFilenamePrefix.empty() ? "" : (Params::logFilenamePrefix + "-"),
        m_timestampStr);
    std::ofstream heatmapFile(heatmapFilename);
    if (!heatmapFile) {
        pb::msg_warning(
            std::format("Unable to open heatmap output file {} for writing. Heatmap will not be saved to file, printing to stdout instead.",
                heatmapFilename));
        std::cout << "~~~~~~~~~~ HEATMAP OUTPUT ~~~~~~~~~~\n";
        m_heatmap.print(std::cout);
    }
    else {
        m_heatmap.print(heatmapFile);
        heatmapFile.close();
        pb::msg_info(std::format("Heatmap output written to file: {}", heatmapFilename));
    }

    // write normalised heatmap to file
    std::string normHeatmapFilename = std::format("{0}/{1}heatmap-normalised-{2}.csv",
        Params::logDir,
        Params::logFilenamePrefix.empty() ? "" : (Params::logFilenamePrefix + "-"),
        m_timestampStr);
    std::ofstream normHeatmapFile(normHeatmapFilename);
    if (!normHeatmapFile) {
        pb::msg_warning(
            std::format("Unable to open normalised heatmap output file {} for writing. Heatmap will not be saved to file, printing to stdout instead.",
                normHeatmapFilename));
        std::cout << "~~~~~~~~~~ NORMALISED HEATMAP OUTPUT ~~~~~~~~~~\n";
        m_heatmap.printNormalised(std::cout);
    }
    else {
        m_heatmap.printNormalised(normHeatmapFile);
        normHeatmapFile.close();
        pb::msg_info(std::format("Normalised heatmap output written to file: {}", normHeatmapFilename));
    }
}


/**
 * Static method to seed the RNG generator from the seed in Params, or
 * with a random seed if no seed is defined in Params.
 *
 * If it is desired to use the seed in Params, the Params::initialise()
 * method should be called before this method is called.
 *
 * If no seed was defined in Params, the randomly generated seed created
 * in this method is then stored in the Params class for later inspection.
 */
void PolyBeeCore::seedRng()
{
    assert(Params::initialised());
    assert(!m_sbRngInitialised);

    if (Params::strRngSeed.empty()) {
        // if no seed string has been supplied, we generate a seed here
        // We keep it consistent with the format of user-supplied seeds by creating
        // a random string of alphanumeric characters (of length 20).
        std::srand(std::random_device()());
        std::string alphanum{"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"};
        std::string newSeedStr(20, 0);
        std::generate(newSeedStr.begin(),
            newSeedStr.end(),
            [alphanum]() { return alphanum[rand() % alphanum.length()]; });

        // having generated a seed string, now use it to seed the RNG!
        std::seed_seq seed1(newSeedStr.begin(), newSeedStr.end());
        m_sRngEngine.seed(seed1);

        // and store the generated seed string back in ModelParams
        Params::strRngSeed = newSeedStr;
    }
    else
    {
        std::seed_seq seed2(Params::strRngSeed.begin(), Params::strRngSeed.end());
        m_sRngEngine.seed(seed2);
    }

    m_sbRngInitialised = true;
}


void PolyBeeCore::earlyExit()
{
    m_bEarlyExitRequested = true;
}


bool PolyBeeCore::stopCriteriaReached()
{
    return (m_iIteration >= Params::numIterations || m_bEarlyExitRequested);
}
