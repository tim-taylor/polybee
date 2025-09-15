/**
 * @file
 *
 * Implementation of the PolyBeeCore class
 */

#include <memory>
#include <ctime>
#include <cassert>
#include <algorithm>
#include <format>
#include "Params.h"
#include "LocalVis.h"
#include "Bee.h"
#include "utils.h"

#include "PolyBeeCore.h"

// Initialise static members
bool PolyBeeCore::m_sbRngInitialised = false;
// - create our static random number generator engine
std::mt19937 PolyBeeCore::m_sRngEngine;
// - and define commonly used distributions
std::uniform_real_distribution<float> PolyBeeCore::m_sUniformProbDistrib(0.0, 1.0);



PolyBeeCore::PolyBeeCore(int argc, char* argv[]) :
    m_iIteration{0}, m_pLocalVis{nullptr}, m_bEarlyExitRequested{false}
{
    Params::initialise(argc, argv);

    seedRng();

    if (!Params::bCommandLineQuiet) {
        std::cout << "~~~~~~~~~~ FINAL PARAM VALUES ~~~~~~~~~~\n";
        Params::print(std::cout);
    }

    if (Params::hiveSpecs.empty()) {
        msg_error_and_exit("Error: No hive positions have been defined!");
    }

    /*
    auto& hivepos = Params::hivePositions[0];

    if (Params::hivePositions.size() > 1) {
        msg_warning(std::format("Multiple hive positions defined! Using the first one only ({},{})", hivepos.x, hivepos.y));
    }

    // instantiate the required number of bees, initialised in the hive position
    for (int i = 0; i < Params::numBees; ++i) {
        m_bees.emplace_back(hivepos, &m_env);
    }
    */

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
        default:
            msg_error_and_exit(std::format("Invalid hive direction {} specified for hive at ({},{}). Must be 0=North, 1=East, 2=South, or 3=West.",
                hive.direction, hive.x, hive.y));
        }

        for (int j = 0; j < numBeesPerHive; ++j) {
            m_bees.emplace_back(fPos(hive.x, hive.y), angle, &m_env);
        }
    }

    m_env.initialise(&m_bees);

    if (Params::bVis) {
        m_pLocalVis = std::unique_ptr<LocalVis>(new LocalVis(this));
    }
}


/**
 * @brief Initiate an entire evolutionary run across the number of islands specified in @Params
 *
 */
void PolyBeeCore::run()
{
    while (!stopCriteriaReached()) {
        // TODO
        // update bee pos
        for (Bee& bee : m_bees) {
            bee.move();
        }

        if (Params::bVis && m_pLocalVis) {
            m_pLocalVis->updateDrawFrame();
        }

        ++m_iIteration;
    }

    if (m_pLocalVis  && !m_bEarlyExitRequested) {
        m_pLocalVis->continueUntilClosed();
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


void PolyBeeCore::reportState()
{
    // TODO
}


void PolyBeeCore::earlyExit()
{
    m_bEarlyExitRequested = true;
}


bool PolyBeeCore::stopCriteriaReached()
{
    return (m_iIteration >= Params::numIterations || m_bEarlyExitRequested);
}
