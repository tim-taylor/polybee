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
#include "polybeeConfig.h"
#include <opencv2/core/version.hpp>
#include <memory>
#include <ctime>
#include <cassert>
#include <algorithm>
#include <format>
#include <chrono>
#include <fstream>

// Initialise static members
std::size_t PolyBeeCore::m_sNextIslandNum = 0;


// Constructor
PolyBeeCore::PolyBeeCore(int argc, char* argv[]) :
    m_pLocalVis {nullptr},
    m_islandNum {m_sNextIslandNum++},
    m_uniformProbDistrib(0.0, 1.0),
    m_angle2PiDistrib(0.0f, 2.0f * std::numbers::pi_v<float>),
    m_uniformIntDistrib(0, std::numeric_limits<int>::max())
{
    Params::initialise(argc, argv);

    seedRng();

    if (!Params::bCommandLineQuiet) {
        std::cout << "~~~~~~~~~~ FINAL PARAM VALUES ~~~~~~~~~~\n";
        Params::print(std::cout);
        std::cout << "~~~~~~~~~~" << std::endl;
    }

    if (Params::hiveSpecs.empty()  && !(Params::bEvolve && Params::evolveSpec.evolveHivePositions)) {
        pb::msg_error_and_exit("No hive positions have been defined!");
    }

    // get a timestamp string for this run, used in output filenames
    generateTimestampString();

    // initialise environment
    m_env.initialise(this);

    if (Params::bVis) {
        m_pLocalVis = std::unique_ptr<LocalVis>(new LocalVis(this));
    }
}


// Copy constructor to create a new PolyBeeCore instance as a copy of an existing one
//
PolyBeeCore::PolyBeeCore(const PolyBeeCore& other, const std::string& rngSeedStr) :
    m_pLocalVis {nullptr},
    m_islandNum {m_sNextIslandNum++}
{
    assert(Params::initialised); // Params must have been initialised before copying PolyBeeCore

    seedRng(&rngSeedStr);

    m_timestampStr = std::format("{}-island-{}", other.m_timestampStr, m_islandNum);

    // initialise environment
    m_env.initialise(this);

    // Note: LocalVis is not initialised when running in island mode
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
        m_timestampStr += std::format("{:x}", distrib(m_rngEngine));
    }
}


void PolyBeeCore::run(bool logIfRequested)
{
    // main simulation loop
    while (!stopCriteriaReached()) {
        if (!m_bPaused) {
            ++m_iIteration;
            m_env.update(); // update environment state, including bee positions and heatmap
        }

        if (Params::bVis && m_pLocalVis) {
            m_pLocalVis->updateDrawFrame();
        }
    }

    // log output files if the user has requested it AND the caller of the method has requested it,
    // AND this is the master island (islandNum == 0) [don't repeat output files for every island]
    if (logIfRequested && Params::logging && m_islandNum == 0) {
        writeOutputFiles();
    }

    // if visualisation is enabled, keep the window open until it is closed by the user
    if (m_pLocalVis  && !m_bEarlyExitRequested) {
        m_pLocalVis->continueUntilClosed();
    }
}


void::PolyBeeCore::resetForNewRun()
{
    m_iIteration = -1;
    m_bEarlyExitRequested = false;
    m_bPaused = false;

    // reset environment (including bees, heatmap etc)
    m_env.reset();
}


void PolyBeeCore::writeOutputFiles() const
{
    // write config to file
    writeConfigFile();

    // write heatmap to file
    const Heatmap& heatmap = m_env.getHeatmap();
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
        heatmap.print(std::cout);
    }
    else {
        heatmap.print(heatmapFile);
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
        heatmap.printNormalised(std::cout);
    }
    else {
        heatmap.printNormalised(normHeatmapFile);
        normHeatmapFile.close();
        pb::msg_info(std::format("Normalised heatmap output written to file: {}", normHeatmapFilename));
    }

    // write general run info to file
    std::string infoFilename = std::format("{0}/{1}run-info-{2}.txt",
        Params::logDir,
        Params::logFilenamePrefix.empty() ? "" : (Params::logFilenamePrefix + "-"),
        m_timestampStr);
    std::ofstream infoFile(infoFilename);
    if (!infoFile) {
        pb::msg_warning(
            std::format("Unable to open run info output file {} for writing. Run info will not be saved to file, printing to stdout instead.",
                infoFilename));
        std::cout << "~~~~~~~~~~ RUN INFO OUTPUT ~~~~~~~~~~\n";
        printRunInfo(std::cout, infoFilename);
    }
    else {
        printRunInfo(infoFile, infoFilename);
        infoFile.close();
        pb::msg_info(std::format("Run info output written to file: {}", infoFilename));
    }
}


void PolyBeeCore::writeConfigFile() const
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
        std::cout << "~~~~~~~~~~ FINAL PARAM VALUES ~~~~~~~~~~" << std::endl;
        Params::print(std::cout);
    }
    else {
        Params::print(configFile, true);
        configFile.close();
        pb::msg_info(std::format("Config output written to file: {}", configFilename));
    }
}


void PolyBeeCore::printRunInfo(std::ostream& os, const std::string& filename) const
{
    os << std::format("Run: {}\n", filename);
    os << std::format("Polybee version: {}.{}.{}.{}\n",
        polybee_VERSION_MAJOR,
        polybee_VERSION_MINOR,
        polybee_VERSION_PATCH,
        polybee_VERSION_TWEAK);
    os << std::format("Git branch: {}\n", polybee_GIT_BRANCH);
    os << std::format("Git commit hash: {}\n", polybee_GIT_COMMIT_HASH);
    os << std::format("OpenCV version: {}.{}.{}\n",
        CV_VERSION_MAJOR,
        CV_VERSION_MINOR,
        CV_VERSION_REVISION);

    auto target = m_env.getRawTargetHeatmapNormalised();
    if (!target.empty()) {
        const Heatmap& heatmap = m_env.getHeatmap();
        float finalEmd = heatmap.emd(target);
        os << std::format("High EMD value: {:.6f}\n", heatmap.high_emd());
        os << std::format("Final EMD between output heatmap and target heatmap: {:.6f}\n", finalEmd);
    }

    os << std::format("Successful visit fraction ({}-{} visits): {:.5f}\n",
        Params::minVisitCountSuccess,
        Params::maxVisitCountSuccess,
        m_env.getSuccessfulVisitFraction());
}


/**
 * Static method to seed the RNG generator from the seed in Params, or
 * with a random seed if no seed is defined in Params.
 * Alternatively, a seed string can be provided directly to this method.
 *
 * If it is desired to use the seed in Params, the Params::initialise()
 * method should be called before this method is called.
 *
 * If no seed was defined in Params, the randomly generated seed created
 * in this method is then stored in the Params class for later inspection.
 */
void PolyBeeCore::seedRng(const std::string* pRngSeedStr)
{
    assert(Params::initialised());
    assert(!m_bRngInitialised);

    if (pRngSeedStr != nullptr) {
        // seed string has been provided directly to this method
        std::seed_seq seed(pRngSeedStr->begin(), pRngSeedStr->end());
        m_rngEngine.seed(seed);
        // we don't store the seed string back in Params in this case, as we assume
        // the caller has derived the given seed string from the seed specified in Params
    }
    else if (Params::strRngSeed.empty() || Params::strRngSeed == "0") {
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
        m_rngEngine.seed(seed1);

        // and store the generated seed string back in ModelParams
        Params::strRngSeed = newSeedStr;
    }
    else
    {
        std::seed_seq seed2(Params::strRngSeed.begin(), Params::strRngSeed.end());
        m_rngEngine.seed(seed2);
    }

    m_bRngInitialised = true;
}


void PolyBeeCore::earlyExit()
{
    m_bEarlyExitRequested = true;
}


bool PolyBeeCore::stopCriteriaReached()
{
    return (m_iIteration >= Params::numIterations || m_bEarlyExitRequested);
}
