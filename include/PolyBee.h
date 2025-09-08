/**
 * @file
 *
 * Declaration of the PolyBee class
 *
 * PolyBee employs a 3 state version of the Game of Life as defined by Boris Lejkin
 * in PolyState Life (https://conwaylife.com/wiki/Polystate_Life)
 */

#ifndef _PolyBee_H
#define _PolyBee_H

#include <random>
#include <memory>
#include "Params.h"
#include "LocalVis.h"

using State = unsigned char;

/**
 * The PolyBee class ...
 */
class PolyBee {

public:
    // constructors and destructors
    PolyBee(int argc, char* argv[]);
    ~PolyBee() {}

    // public methods
    void run();         ///< run evolutionary algorithm with no visualisation
    void runWithVis();  ///< run visual front-end with manual control over evolutionary algorithm

    // public static methods

    /**
     * Seed the model's RNG from the seed specified in ModelParams
     */
    static void seedRng();

    // public static members
    static std::mt19937 m_sRngEngine;
    static std::uniform_real_distribution<float> m_sUniformProbDistrib; ///< Uniform distrib 0.0--1.0

private:
    // private methods
    void reportState();
    bool stopCriteriaReached();


    //////////////////////////////////////////////////////////////
    // private members

    int m_iIteration;

    std::unique_ptr<LocalVis> m_pLocalVis;

     // private static members
    static bool m_sbRngInitialised;

    // friends
    friend LocalVis;
};

#endif /* _PolyBee_H */
