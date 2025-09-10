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
    static int numIterations;

    // Environment configuration
    static int envW;
    static int envH;
    static int tunnelW;
    static int tunnelH;
    static int tunnelX;
    static int tunnelY;

    // Bee configuration
    static int numBees;

    // Hive configuration
    static std::vector<fPos> hivePositions;

    // Visualisation
    static bool bVis;
    static int visCellSize;
    static int visTargetFPS;

    // Generic options (not included in registry)
    static std::string strConfigFilename;

    // Derived parameters. Do not set directly, call calculateDerivedParameters()!
    // static int numCells;


    // RNG
    static std::string strRngSeed;        ///< Seed string used to seed RNG

    // Miscellaneous control flags
    static bool bCommandLineQuiet;

    // Parameter registry
    static std::vector<ParamInfo> REGISTRY;

    // initialiser (must be called explicitly before the Param class is used)
    static void initialise(int argc, char* argv[]);

    // public helper methods
    static bool initialised() { return bInitialised; }
    static void calculateDerivedParams();
    static void print(std::ostream& os);
    static void setAllDefault();

private:
    // private methods
    static void initRegistry();

    // private flags
    static bool bInitialised;
};

#endif