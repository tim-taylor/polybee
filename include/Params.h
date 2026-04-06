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


enum class EvolveObjective {
    EMD_TO_TARGET_HEATMAP = 0,
    FRACTION_FLOWERS_SUCCESSFUL_VISIT_RANGE = 1
};


enum class NetType {
    NONE,
    ANTIBIRD,
    ANTIHAIL
};


struct EntranceSpec {
    float e1;           // position of first edge of entrance along the specified side of tunnel, in tunnel coordinates
    float e2;           // position of second edge of entrance along the specified side of tunnel, in tunnel coordinates
    int side;           // 0=North, 1=East, 2=South, 3=West
    int id {0};         // unique ID for this entrance, assigned automatically in Tunnel::addEntrance()
    NetType netType;

    EntranceSpec(float e1, float e2, int side) : e1(e1), e2(e2), side(side), netType(NetType::NONE) {}
    EntranceSpec(float e1, float e2, int side, NetType netType) : e1(e1), e2(e2), side(side), netType(netType) {}
};


struct FromMidpoints {};    // tag struct to disambiguate constructor of BarrierSpec that takes midpoints, length, and orientation

struct BarrierSpec {
    // x1,y1:x2,y2[:nrx,dx[:nry,dy]]
    float x1;               // x position of first endpoint of barrier in environment coordinates
    float y1;               // y position of first endpoint of barrier in environment coordinates
    float x2;               // x position of second endpoint of barrier in environment coordinates
    float y2;               // y position of second endpoint of barrier in environment coordinates
    int numRepeatsX {1};    // number of repeats of the barrier along x axis
    float dx {200.0f};      // spacing between repeats along x axis
    int numRepeatsY {1};    // number of repeats of the barrier along y axis
    float dy {200.0f};      // spacing between repeats along y axis

    BarrierSpec(float x1, float y1, float x2, float y2) :
        x1(x1), y1(y1), x2(x2), y2(y2) { calcLengthAndOrientation(); }
    BarrierSpec(float x1, float y1, float x2, float y2, int numRepeatsX, float dx) :
        x1(x1), y1(y1), x2(x2), y2(y2), numRepeatsX(numRepeatsX), dx(dx) { calcLengthAndOrientation(); }
    BarrierSpec(float x1, float y1, float x2, float y2, int numRepeatsX, float dx, int numRepeatsY, float dy) :
        x1(x1), y1(y1), x2(x2), y2(y2), numRepeatsX(numRepeatsX), dx(dx), numRepeatsY(numRepeatsY), dy(dy) { calcLengthAndOrientation(); }

    BarrierSpec(FromMidpoints, float midpointX, float midpointY, float length, float orientation) {
        float halfLength = length / 2.0f;
        float cosOrient = std::cos(orientation);
        float sinOrient = std::sin(orientation);
        x1 = midpointX - halfLength * cosOrient;
        y1 = midpointY - halfLength * sinOrient;
        x2 = midpointX + halfLength * cosOrient;
        y2 = midpointY + halfLength * sinOrient;
        numRepeatsX = 1;
        dx = 0.0f;
        numRepeatsY = 1;
        dy = 0.0f;
        len = length;
        ori = orientation;
    }

    // we assume that the x and y positions are set once at construction and never changed after, so we can calculate
    // the length of the barrier once at construction and just return it in the length() method
    float length() const { return len; };
    float orientation() const { return ori; }
    float midpointX() const { return (x1 + x2) / 2.0f; }
    float midpointY() const { return (y1 + y2) / 2.0f; }

private:
    float len;
    float ori;

    void calcLengthAndOrientation() {
        len = std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
        ori = std::atan2(y2 - y1, x2 - x1);
    }
};


struct PatchSpec {
    //  x,y,w,h:r[:j:[s[:n:dx,dy]]]
    float x;                    // x position of top-left-corner of patch in environment coordinates
    float y;                    // y position of top-left-corner of patch in environment coordinates
    float w;                    // width of patch
    float h;                    // height of patch
    float spacing {0.0f};       // spacing between plants
    float jitter {0.0f};        // jitter between plants (std dev)
    int   speciesID {1};        // species
    int   numRepeats {1};       // number of repeats of the patch
    float dx {200.0f};          // x offset between repeats of patch
    float dy {0.0f};            // y offset between repeats of patch
    bool  ignoreForSVF {false}; // if set, flowers in this patch don't count in calculation of successful visit fraction

    int getNumX() const { return numX; } // number of plants along x axis of patch - derived from w and spacing
    int getNumY() const { return numY; } // number of plants along y axis of patch - derived from h and spacing

    PatchSpec(float x, float y, float w, float h, float spacing);
    PatchSpec(float x, float y, float w, float h, float spacing, float jitter);
    PatchSpec(float x, float y, float w, float h, float spacing, float jitter, int speciesID);
    PatchSpec(float x, float y, float w, float h, float spacing, float jitter, int speciesID, int numRepeats, float dx, float dy);
    PatchSpec(float x, float y, float w, float h, float spacing, float jitter, int speciesID, int numRepeats, float dx, float dy, bool ignore);
    PatchSpec(float x, float y, float sz, float spacing, float jitter, bool ignore);

private:
    void commonInit();

    int numX {1};         // number of plants along x axis of patch - derived from w and spacing
    int numY {1};         // number of plants along y axis of patch - derived from h and spacing
};


struct EvolveSpec {
    // [E:n,w][;][H:i,o,f]][;][B:n,w][;][X:n,w]
    bool  evolveEntrancePositions {true};   // whether to include tunnel entrance positions in the optimization
    bool  evolveHivePositions {false};      // whether to include hive positions in the optimization
    bool  evolveBridgePositions {false};    // whether to include bridge positions in the optimization
    bool  evolveBarrierPositions {false};   // whether to include barrier positions in the optimization

    int   numEntrances {0};         // number of tunnel entrances (if evolveEntrancePositions is true)
    float entranceWidth {100.0f};   // width of tunnel entrances (if evolveEntrancePositions is true)
    int   numHivesInsideTunnel {0}; // number of hives that must be placed inside tunnel (if evolveHivePositions is true)
    int   numHivesOutsideTunnel {0};// number of hives that must be placed outside tunner (if evolveHivePositions is true)
    int   numHivesFree {0};         // number of hives that can be placed either inside or outside the tunnel (if evolveHivePositions is true)
    int   numBridges {0};           // number of bridges to include in the environment (if evolveBridgePositions is true)
    float bridgeWidth {50.0f};      // width of a bridge (if evolveBridgePositions is true)
    int   numBarriers {0};          // number of barriers to include in the environment (if evolveBarrierPositions is true)
    float barrierWidth {100.0f};    // width of barriers (if evolveBarrierPositions is true)

    EvolveSpec() = default;
    EvolveSpec(bool evolveEntrancePositions, bool evolveHivePositions,
               bool evolveBridgePositions, bool evolveBarrierPositions,
               int numEntrances, float entranceWidth,
               int numHivesInsideTunnel, int numHivesOutsideTunnel, int numHivesFree,
               int numBridges, float bridgeWidth,
               int numBarriers, float barrierWidth) :
        evolveEntrancePositions(evolveEntrancePositions), evolveHivePositions(evolveHivePositions),
        evolveBridgePositions(evolveBridgePositions), evolveBarrierPositions(evolveBarrierPositions),
        numEntrances(numEntrances), entranceWidth(entranceWidth),
        numHivesInsideTunnel(numHivesInsideTunnel), numHivesOutsideTunnel(numHivesOutsideTunnel), numHivesFree(numHivesFree),
        numBridges(numBridges), bridgeWidth(bridgeWidth),
        numBarriers(numBarriers), barrierWidth(barrierWidth)
    {}
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
    static std::vector<EntranceSpec> entranceSpecs;

    // Tunnel exit net properties
    static float netAntibirdExitProb;       // probability of bee exiting through antibird net
    static float netAntihailExitProb;       // probability of bee exiting through antihail net
    static int netAntibirdMaxExitAttempts;  // max exit attempts through antibird net
    static int netAntihailMaxExitAttempts;  // max exit attempts through antihail net

    // Barrier configuration
    static std::vector<BarrierSpec> barrierSpecs;
    static float barrierPassProb; // probability that a bee will fly over a barrier rather than having its path blocked

    // Patch configuration
    static std::vector<PatchSpec> patchSpecs;
    static float plantDefaultSpacing; // default plant spacing used for bridge patches when evolving bridge positions
    static float plantDefaultJitter;  // default plant jitter used for bridge patches when evolving bridge positions

    // Flower configuration
    static float flowerInitialNectar; // initial nectar amount for each flower
    static int minVisitCountSuccess; // minimum number of bee visits for successful pollination
    static int maxVisitCountSuccess; // maximum number of bee visits for successful pollination

    // Bee configuration
    static int numBees;
    static float beeMaxDirDelta; // maximum change in direction (radians) per step
    static float beeStepLength; // how far a bee moves forward at each time step
    static int beePathRecordLen; // maximum number of positions to record in bee's path
    static float beeVisualRange; // maximum distance over which a bee can detect a flower
    static int beeVisitMemoryLength; // how many recently visited plants a bee remembers
    static float beeProbVisitNearestFlower; // probability that a bee visits the nearest flower rather than a random visible flower
    //static int beeForageDuration; // duration (number of iterations) of a bee's foraging bout
    static int beeInHiveDuration; // duration (number of iterations) of a bee's stay in the hive between foraging bouts
    static float beeInitialEnergy; // energy a bee has when it leaves the hive to commence a foraging trip
    static float beeEnergyDepletionPerStep; // energy a bee expends on each step when foraging
    static float beeEnergyBoostPerFlower; // energy a bee extracts from an unvisited flower
    static int beeOnFlowerDuration; // number of simulation steps a bee will stay on a flower having landed on it
    static float beeEnergyMinThreshold; // lower threshold of bee's energy below which it will return to hive
    static float beeEnergyMaxThreshold; // upper threshold of bee's energy above which it will return to hive

    // Hive configuration
    static std::vector<HiveSpec> hiveSpecs;

    // Optimization
    static bool bEvolve; // determines whether to run optimization to match output heatmap against target
    static EvolveObjective evolveObjective; // this is the public-facing version of evolveObjectivePvt that is set in calculateDerivedParams()
    static int evolveObjectivePvt; // 0 = EMD to target heatmap, 1 = fraction of flowers in successful visit range
    static EvolveSpec evolveSpec; // specifications for the optimization process, parsed from evolveSpecPvt in calculateDerivedParams()
    static std::string evolveSpecPvt; // string form of evolve-spec parameter, format: [E:n,w][;][H:i,o,f]]
    static std::string strTargetHeatmapFilename; // CSV file containing target heatmap for optimization
    static int numConfigsPerGen; // number of trials to run during each generation of optimization
    static int numTrialsPerConfig; // number of trials to run for each configuration/individual in each generation
    static int numGenerations; // number of generations to run the optimization process
    static int numIslands; // number of islands of evolving populations (when num-islands=1, there is just a single population with no migration)
    static int migrationPeriod; // period (number of generations) between each migration event when using multiple islands
    static int migrationNumReplace; // number of individuals on an Island that can be replaced by migrants at each migration event
    static int migrationNumSelect; // number of individuals on an Island that can be selected for migration at each migration event
    static bool useDiverseAlgorithms; // use diverse optimisation algorithms on each island (when num-islands > 1)
    static bool bridgeOverlapsAllowed; // if false, attempt to resolve overlaps of bridges with patches/other bridges by shifting; if unsuccessful, remove the bridge

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