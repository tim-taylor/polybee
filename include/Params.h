/**
 * @file
 *
 * Declaration of the Params struct
 */

#ifndef _PARAMS_H
#define _PARAMS_H

#include "utils.h"
#include <iostream>
#include <string>
#include <vector>
#include <variant>

using param_variant = std::variant<bool, int, float, std::string>;


enum ParamType
{
    BOOL,
    INT,
    FLOAT,
    STRING
};


struct ParamInfo
{
    // variables
    std::string uname;
    std::string name;
    ParamType type;
    void* pVar;
    param_variant defaultVal;
    std::string desc;

    // constructors
    ParamInfo(std::string username, std::string varname, ParamType vartype,
        void* var_ptr, param_variant default_val, std::string description) :
        uname(username), name(varname), type(vartype), pVar(var_ptr), defaultVal(default_val), desc(description)
    {
    };

    // methods
    std::string valueAsStr();
};


struct HiveSpec {
    float x;        // position of hive in environment coordinates
    float y;        // position of hive in environment coordinates
    int direction;  // 0=North, 1=East, 2=South, 3=West, 4=Random

    HiveSpec(float x, float y, int direction) : x(x), y(y), direction(direction) {}
};


struct TunnelEntranceSpec {
    float e1; // position of first edge of entrance along the specified side of tunnel, in tunnel coordinates
    float e2; // position of second edge of entrance along the specified side of tunnel, in tunnel coordinates
    int side; // 0=North, 1=East, 2=South, 3=West

    TunnelEntranceSpec(float e1, float e2, int side) : e1(e1), e2(e2), side(side) {}
};


struct PatchSpec {
    //  x,y,w,h:r[:j:[s[:n:dx,dy]]]
    float x;                // x position of top-left-corner of patch in environment coordinates
    float y;                // y position of top-left-corner of patch in environment coordinates
    float w;                // width of patch
    float h;                // height of patch
    float spacing {0.0f};   // spacing between plants
    float jitter {0.0f};    // jitter between plants (std dev)
    int   speciesID {1};    // species
    int   numRepeats {1};   // number of repeats of the patch
    float dx {200.0f};      // x offset between repeats of patch
    float dy {0.0f};        // y offset between repeats of patch
    int   numX {1};         // number of plants along x axis of patch
    int   numY {1};         // number of plants along y axis of patch

    PatchSpec(float x, float y, float w, float h, float spacing);
    PatchSpec(float x, float y, float w, float h, float spacing, float jitter);
    PatchSpec(float x, float y, float w, float h, float spacing, float jitter, int speciesID);
    PatchSpec(float x, float y, float w, float h, float spacing, float jitter, int speciesID, int numRepeats, float dx, float dy);

private:
    void commonInit();
};


/**
 * @brief Class for flexibily dealing with system parameters
 *
 * To add a new paramater, do the following:
 * 1. Add a new public static variable to the Params class in Params.h
 * 2. Instantiate the new static variable at the top of Params.cpp
 * 3. Add the new parameter to the registry in Params::initRegistry() in Params.cpp
 */
class Params
{
public:
    // Simulation control
    static std::string strRngSeed;        ///< Seed string used to seed RNG
    static int numIterations;

    // Environment configuration
    static float envW;
    static float envH;

    // Tunnel configuration
    static float tunnelW;
    static float tunnelH;
    static float tunnelX; // top-left x position of tunnel in environment coordinates
    static float tunnelY; // top-left y position of tunnel in environment coordinates
    static std::vector<TunnelEntranceSpec> tunnelEntranceSpecs;

    // Patch configuration
    static std::vector<PatchSpec> patchSpecs;

    // Bee configuration
    static int numBees;
    static float beeMaxDirDelta; // maximum change in direction (radians) per step
    static float beeStepLength; // how far a bee moves forward at each time step
    static int beePathRecordLen; // maximum number of positions to record in bee's path
    static float beeVisualRange; // maximum distance over which a bee can detect a flower
    static int beeVisitMemoryLength; // how many recently visited plants a bee remembers
    static float beeProbVisitNearestFlower; // probability that a bee visits the nearest flower rather than a random visible flower

    // Hive configuration
    static std::vector<HiveSpec> hiveSpecs;

    // Optimization
    static bool bEvolve; // determines whether to run optimization to match output heatmap against target
    static std::string strTargetHeatmapFilename; // CSV file containing target heatmap for optimization
    static int numConfigsPerGen; // number of trials to run during each generation of optimization
    static int numTrialsPerConfig; // number of trials to run for each configuration/individual in each generation
    static int numGenerations; // number of generations to run the optimization process

    // Logging and output
    static int heatmapCellSize; // size of each cell in the heatmap of bee positions
    static std::string logDir; // directory for output files
    static std::string logFilenamePrefix; // prefix for output file names
    static bool logging; // determines whether output files are written at the end of a run
    static bool bCommandLineQuiet;

    // Visualisation
    static bool bVis;
    static float visCellSize;
    static int visDelayPerStep;
    static int visBeePathDrawLen; // maximum number of path segments to draw for each bee

    // Options that can be specified on command line but are not in a config file
    static std::string strConfigFilename;

    // Derived and other internal parameters (not set by user, calculated internally)
    // static int numCells;

    // Parameter registry
    static std::vector<ParamInfo> REGISTRY;

    // initialiser (must be called explicitly before the Param class is used)
    static void initialise(int argc, char* argv[]);

    // public helper methods
    static bool initialised() { return bInitialised; }
    static void calculateDerivedParams();
    static void print(std::ostream& os, bool bGenerateForConfigFile = false);
    static void setAllDefault();
    static void checkConsistency();

private:
    // private methods
    static void initRegistry();

    // private flags
    static bool bInitialised;
};

#endif