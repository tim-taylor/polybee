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
#include <string>
#include "Environment.h"
#include "Params.h"
#include "LocalVis.h"
#include "Bee.h"
#include "Tunnel.h"
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
    PolyBeeCore(const PolyBeeCore& other) = delete; // disable copy constructor
    PolyBeeCore(const PolyBeeCore& other, const std::string& rngSeedStr);
    ~PolyBeeCore() {}

    //////////////////////////////////////////////////////////////
    // public methods
    void run(bool logIfRequested = true);
    void earlyExit();
    void resetForNewRun();

    const Environment& getEnvironment() const { return m_env; }
    Environment& getEnvironment() { return m_env; }
    const Heatmap& getHeatmap() const { return m_env.getHeatmap(); }
    double getSuccessfulVisitFraction() const { return m_env.getSuccessfulVisitFraction(); }
    Tunnel& getTunnel() { return m_env.getTunnel(); }
    const std::vector<Hive>& getHives() const { return m_env.getHives(); }
    const std::vector<Bee>& getBees() const { return m_env.getBees(); }

    void pauseSimulation(bool pause) {m_bPaused = pause;}

    void writeConfigFile() const;

    const std::string& getTimestampStr() const { return m_timestampStr; }

    std::size_t getIslandNum() const { return m_islandNum; }
    bool isMasterCore() const { return m_islandNum == 0; }
    std::size_t evaluationCount() const { return m_evaluationCount; }
    void incrementEvaluationCount() { ++m_evaluationCount; }

    //////////////////////////////////////////////////////////////
    // public members
    std::mt19937 m_rngEngine;
    std::uniform_real_distribution<float> m_uniformProbDistrib; ///< Uniform distrib 0.0--1.0
    std::uniform_real_distribution<float> m_angle2PiDistrib;    ///< Uniform distrib 0.0--2π
    std::uniform_int_distribution<int> m_uniformIntDistrib;     ///< Uniform distrib of unsigned ints

    //////////////////////////////////////////////////////////////
    // public static methods

    void seedRng(const std::string* pRngSeedStr = nullptr);      ///< Seed the model's RNG from the seed specified in ModelParams

    //////////////////////////////////////////////////////////////
    // public static members


private:
    //////////////////////////////////////////////////////////////
    // private methods
    void generateTimestampString();
    bool stopCriteriaReached();
    void writeOutputFiles() const;
    void printRunInfo(std::ostream& os, const std::string& filename) const;

    //////////////////////////////////////////////////////////////
    // private members
    Environment m_env;

    bool m_bRngInitialised;

    std::string m_timestampStr {}; // timestamp string for this run, used in output filenames

    int m_iIteration {-1};
    std::unique_ptr<LocalVis> m_pLocalVis;

    bool m_bEarlyExitRequested {false};
    bool m_bPaused {false};

    std::size_t m_islandNum {0};        // island number for this PolyBeeCore instance (used in pagmo archipelago runs)

    std::size_t m_evaluationCount {0};  // count of number of fitness evaluations performed for this PolyBeeCore instance
                                        // (only used when running PolyBeeEvolve)

    //////////////////////////////////////////////////////////////
    // private static members

    static std::size_t m_sNextIslandNum; // static counter to assign island numbers when copying PolyBeeCore instances

    //////////////////////////////////////////////////////////////
    // friends
    friend LocalVis;
};

#endif /* _PolyBeeCore_H */
