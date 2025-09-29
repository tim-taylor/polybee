/**
 * @file
 *
 * Declaration of the PolyBeeCore class
 *
 * PolyBeeCore employs a 3 state version of the Game of Life as defined by Boris Lejkin
 * in PolyState Life (https://conwaylife.com/wiki/Polystate_Life)
 */

#ifndef _POLYBEECORE_H
#define _POLYBEECORE_H

#include <random>
#include <memory>
#include <vector>
#include "Params.h"
#include "LocalVis.h"
#include "Bee.h"
#include "Environment.h"
#include "Heatmap.h"

using State = unsigned char;

/**
 * The PolyBeeCore class ...
 */
class PolyBeeCore {

public:
    //////////////////////////////////////////////////////////////
    // constructors and destructors
    PolyBeeCore(int argc, char* argv[]);
    ~PolyBeeCore() {}

    //////////////////////////////////////////////////////////////
    // public methods
    void run();
    void earlyExit();
    void resetForNewRun();

    const Heatmap& getHeatmap() const { return m_heatmap; }

    void pauseSimulation(bool pause) {m_bPaused = pause;}

    //////////////////////////////////////////////////////////////
    // public static methods

    static void seedRng(); ///< Seed the model's RNG from the seed specified in ModelParams

    //////////////////////////////////////////////////////////////
    // public static members
    static std::mt19937 m_sRngEngine;
    static std::uniform_real_distribution<float> m_sUniformProbDistrib; ///< Uniform distrib 0.0--1.0
    static std::uniform_real_distribution<float> m_sAngle2PiDistrib;   ///< Uniform distrib 0.0--2π
    static std::uniform_int_distribution<int> m_sUniformIntDistrib; ///< Uniform distrib of unsigned ints

private:
    //////////////////////////////////////////////////////////////
    // private methods
    void generateTimestampString();
    void initialiseBees();
    bool stopCriteriaReached();
    void writeOutputFiles();

    //////////////////////////////////////////////////////////////
    // private members
    std::vector<Bee> m_bees;
    Environment m_env;
    Heatmap m_heatmap;

    std::string m_timestampStr {}; // timestamp string for this run, used in output filenames

    int m_iIteration {-1};
    std::unique_ptr<LocalVis> m_pLocalVis;

    bool m_bEarlyExitRequested {false};
    bool m_bPaused {false};

    //////////////////////////////////////////////////////////////
    // private static members
    static bool m_sbRngInitialised;

    //////////////////////////////////////////////////////////////
    // friends
    friend LocalVis;
};

#endif /* _PolyBeeCore_H */
