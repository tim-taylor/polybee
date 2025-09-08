/**
 * @file
 *
 * Implementation of the PolyBee class
 */

#include <memory>
#include <ctime>
#include <cassert>
#include <algorithm>
#include "Params.h"
#include "LocalVis.h"

#include "PolyBee.h"

 // Initialise static members
bool PolyBee::m_sbRngInitialised = false;
// -- create our static random number generator engine
std::mt19937 PolyBee::m_sRngEngine;
// -- and define  commonly used distributions
std::uniform_real_distribution<float> PolyBee::m_sUniformProbDistrib(0.0, 1.0);



PolyBee::PolyBee(int argc, char* argv[])
: m_iIteration{0}
{
    Params::initialise(argc, argv);

    seedRng();

    if (!Params::bCommandLineQuiet) {
        // TODO - remove this TEMP code
        std::cout << "~~~~~~~~~~ FINAL PARAM VALUES ~~~~~~~~~~\n";
        Params::print(std::cout);
    }

    // TODO
    /*
    m_Cells1.insert(m_Cells1.end(), Params::numCells, State{});
    m_Cells2.insert(m_Cells2.end(), Params::numCells, State{});

    initialiseIslands();
    */

    if (Params::bVis) {
        m_pLocalVis = std::unique_ptr<LocalVis>(new LocalVis(this));
    }
}


/**
 * @brief Initiate an entire evolutionary run across the number of islands specified in @Params
 *
 */
void PolyBee::run()
{
    /*
    while (true) {
        evaluateIslands();
        reportState();
        if (stopCriteriaReached() || m_iGeneration >= 100) {
            break; // should also allow user to press escape to stop if vis is active?
        }
        migrateBetweenIslands();
        createNewGenerations();
        ++m_iGeneration;
    }
    */

    //initialiseCells(); -- need to do this in evaluateIslands for each evaluation
    // also need to update vis if needed during each evaluation
}


/**
 * @brief Run the main PolyBee evolutionary algorithm under manual control by the user
 * via the vidual frontend
 */
 void PolyBee::runWithVis()
 {
    // TODO
    assert(m_pLocalVis);
    m_pLocalVis->run();
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
void PolyBee::seedRng()
{
    assert(Params::initialised());
    assert(!m_sbRngInitialised);

    //const std::string& seedStr = Params::getRngSeedStr();

    if (Params::strRngSeed.empty()) {
        // if no seed string has been supplied, we generate a seed here
        // We keep it consistent with the format of user-supplied seeds by creating
        // a random string of alphanumeric characters (of length 20).
        std::srand(std::random_device()());
        std::string alphanum{ "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789" };
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

    if (!Params::bCommandLineQuiet)
    {
        std::cout << "rng-seed (actual): " << Params::strRngSeed << std::endl;
    }

    m_sbRngInitialised = true;
}

void PolyBee::reportState()
{
    // TODO
}


bool PolyBee::stopCriteriaReached()
{
    // TODO
    return false;
}
