#include "Params.h"
#include "utils.h"
#include "polybeeConfig.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <format>
#include <vector>
#include <string>
#include <regex>
#include <filesystem>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

// Instantiate all static members
// Note, default values are assigned in Params::initRegistry()

// Simulation control
std::string Params::strRngSeed;
int Params::numIterations;

// Environment configuration
float Params::envW;
float Params::envH;

// Tunnel configuration
float Params::tunnelW;
float Params::tunnelH;
float Params::tunnelX;
float Params::tunnelY;
std::vector<TunnelEntranceSpec> Params::tunnelEntranceSpecs;

// Tunnel exit net properties
float Params::netAntibirdExitProb;
float Params::netAntihailExitProb;
int Params::netAntibirdMaxExitAttempts;
int Params::netAntihailMaxExitAttempts;

// Patch configuration
std::vector<PatchSpec> Params::patchSpecs;

// Flower configuration
float Params::flowerInitialNectar;
int Params::minVisitCountSuccess;
int Params::maxVisitCountSuccess;

// Bee configuration
int Params::numBees;
float Params::beeMaxDirDelta;
float Params::beeStepLength;
int Params::beePathRecordLen;
float Params::beeVisualRange;
int Params::beeVisitMemoryLength;
float Params::beeProbVisitNearestFlower;
//int Params::beeForageDuration;  // Is this now obsolete?
int Params::beeInHiveDuration;
float Params::beeInitialEnergy;
float Params::beeEnergyDepletionPerStep;
float Params::beeEnergyBoostPerFlower;
int Params::beeOnFlowerDuration;
float Params::beeEnergyMinThreshold;
float Params::beeEnergyMaxThreshold;

// Hive configuration
std::vector<HiveSpec> Params::hiveSpecs;

// Optimization
bool Params::bEvolve;
EvolveObjective Params::evolveObjective;
int Params::evolveObjectivePvt;
std::string Params::strTargetHeatmapFilename;
int Params::numConfigsPerGen;
int Params::numTrialsPerConfig;
int Params::numGenerations;
int Params::numIslands;
int Params::migrationPeriod;
int Params::migrationNumReplace;
int Params::migrationNumSelect;
bool Params::useDiverseAlgorithms;

// Logging and output
int Params::heatmapCellSize;
std::string Params::logDir;
std::string Params::logFilenamePrefix;
bool Params::logging;
bool Params::bCommandLineQuiet;

// Visualisation
bool Params::bVis;
float Params::visCellSize;
int Params::visDelayPerStep;
int Params::visBeePathDrawLen;

// Options that can be specified on command line but are not in a config file
std::string Params::strConfigFilename = "polybee.cfg";

// Derived and other internal parameters (not set by user, calculated internally)
bool Params::bInitialised = false;

// private variables (not set by user, used internally)
std::vector<ParamInfo> Params::REGISTRY;


/////////////////////////////////////////////////////////////////
// PatchSpec methods

PatchSpec::PatchSpec(float x, float y, float w, float h, float spacing) :
    x(x), y(y), w(w), h(h), spacing(spacing)
{
    commonInit();
}

PatchSpec::PatchSpec(float x, float y, float w, float h, float spacing, float jitter) :
    x(x), y(y), w(w), h(h), spacing(spacing), jitter(jitter)
{
    commonInit();
}

PatchSpec::PatchSpec(float x, float y, float w, float h, float spacing, float jitter, int speciesID) :
    x(x), y(y), w(w), h(h), spacing(spacing), jitter(jitter), speciesID(speciesID)
{
    commonInit();
}

PatchSpec::PatchSpec(float x, float y, float w, float h, float spacing, float jitter, int speciesID, int numRepeats, float dx, float dy) :
    x(x), y(y), w(w), h(h), spacing(spacing), jitter(jitter), speciesID(speciesID), numRepeats(numRepeats), dx(dx), dy(dy)
{
    commonInit();
}

void PatchSpec::commonInit()
{
    // Calculate numX and numY based on w, h, and spacing
    numX = std::max(1, static_cast<int>(w / spacing));
    numY = std::max(1, static_cast<int>(h / spacing));
}



/////////////////////////////////////////////////////////////////
// Params methods

// Initial registratin of all primary (non-derived) parameters and associated details
void Params::initRegistry()
{
    REGISTRY.emplace_back("env-width", "envW", ParamType::FLOAT, &envW, 450.0f, "Width (number of cells) of environment");
    REGISTRY.emplace_back("env-height", "envH", ParamType::FLOAT, &envH, 250.0f, "Height (number of cells) of environment");
    REGISTRY.emplace_back("tunnel-width", "tunnelW", ParamType::FLOAT, &tunnelW, 50.0f, "Width (number of cells) of tunnel");
    REGISTRY.emplace_back("tunnel-height", "tunnelH", ParamType::FLOAT, &tunnelH, 50.0f, "Height (number of cells) of tunnel");
    REGISTRY.emplace_back("tunnel-x", "tunnelX", ParamType::FLOAT, &tunnelX, 200.0f, "X position of left edge of tunnel");
    REGISTRY.emplace_back("tunnel-y", "tunnelY", ParamType::FLOAT, &tunnelY, 100.0f, "Y position of top edge of tunnel");
    REGISTRY.emplace_back("net-antibird-exit-prob", "netAntibirdExitProb", ParamType::FLOAT, &netAntibirdExitProb, 0.1187f, "Per-attempt probability of bee exiting through antibird net");
    REGISTRY.emplace_back("net-antihail-exit-prob", "netAntihailExitProb", ParamType::FLOAT, &netAntihailExitProb, 0.0371f, "Per-attempt probability of bee exiting through antihail net");
    REGISTRY.emplace_back("net-antibird-max-exit-attempts", "netAntibirdMaxExitAttempts", ParamType::INT, &netAntibirdMaxExitAttempts, 7, "Maximum exit attempts through antibird net before giving up");
    REGISTRY.emplace_back("net-antihail-max-exit-attempts", "netAntihailMaxExitAttempts", ParamType::INT, &netAntihailMaxExitAttempts, 11, "Maximum exit attempts through antihail net before giving up");
    REGISTRY.emplace_back("flower-initial-nectar", "flowerInitialNectar", ParamType::FLOAT, &flowerInitialNectar, 100.0f, "Initial nectar amount for each flower");
    REGISTRY.emplace_back("num-bees", "numBees", ParamType::INT, &numBees, 50, "Number of bees in the simulation");
    REGISTRY.emplace_back("bee-max-dir-delta", "beeMaxDirDelta", ParamType::FLOAT, &beeMaxDirDelta, 0.4f, "Maximum change in direction (radians) per step");
    REGISTRY.emplace_back("bee-step-length", "beeStepLength", ParamType::FLOAT, &beeStepLength, 20.0f, "How far a bee moves forward at each time step");
    REGISTRY.emplace_back("bee-path-record-len", "beePathRecordLen", ParamType::INT, &beePathRecordLen, 250, "Maximum number of positions to record in bee's path");
    REGISTRY.emplace_back("bee-visual-range", "beeVisualRange", ParamType::FLOAT, &beeVisualRange, 1.0f, "Maximum distance over which a bee can detect a flower");
    REGISTRY.emplace_back("bee-visit-memory-length", "beeVisitMemoryLength", ParamType::INT, &beeVisitMemoryLength, 5, "How many recently visited plants a bee remembers");
    REGISTRY.emplace_back("bee-prob-visit-nearest-flower", "beeProbVisitNearestFlower", ParamType::FLOAT, &beeProbVisitNearestFlower, 0.9f, "Probability that a bee visits a nearby flower rather than a move in a random direction");
    //REGISTRY.emplace_back("bee-forage-duration", "beeForageDuration", ParamType::INT, &beeForageDuration, 1000, "Duration (number of iterations) of a bee's foraging bout");
    REGISTRY.emplace_back("bee-in-hive-duration", "beeInHiveDuration", ParamType::INT, &beeInHiveDuration, 200, "Duration (number of iterations) of a bee's stay in the hive between foraging bouts");
    REGISTRY.emplace_back("bee-initial-energy", "beeInitialEnergy", ParamType::FLOAT, &beeInitialEnergy, 100.0f, "Energy a bee has when it leaves the hive to commence a foraging trip");
    REGISTRY.emplace_back("bee-energy-depletion-per-step", "beeEnergyDepletionPerStep", ParamType::FLOAT, &beeEnergyDepletionPerStep, 1.0f, "Energy a bee expends on each step when foraging");
    REGISTRY.emplace_back("bee-energy-boost-per-flower", "beeEnergyBoostPerFlower", ParamType::FLOAT, &beeEnergyBoostPerFlower, 10.0f, "Energy a bee extracts from an unvisited flower");
    REGISTRY.emplace_back("bee-on-flower-duration", "beeOnFlowerDuration", ParamType::INT, &beeOnFlowerDuration, 5, "Number of simulation steps a bee will stay on a flower having landed on it");
    REGISTRY.emplace_back("bee-energy-min-threshold", "beeEnergyMinThreshold", ParamType::FLOAT, &beeEnergyMinThreshold, 0.0f, "Lower threshold of bee's energy store below which it will return to hive to replenish");
    REGISTRY.emplace_back("bee-energy-max-threshold", "beeEnergyMaxThreshold", ParamType::FLOAT, &beeEnergyMaxThreshold, 100.0f, "Upper threshold of bee's energy store above which it will return to hive after successful foraging");
    REGISTRY.emplace_back("num-iterations", "numIterations", ParamType::INT, &numIterations, 100, "Number of iterations to run the simulation");
    REGISTRY.emplace_back("evolve", "bEvolve", ParamType::BOOL, &bEvolve, false, "Run optimization to match output heatmap against target heatmap");
    REGISTRY.emplace_back("evolve-objective", "evolveObjectivePvt", ParamType::INT, &evolveObjective, 0, "Optimization objective: 0=EMD to target heatmap, 1=Fraction of flowers in successful visit range");
    REGISTRY.emplace_back("min-visit-count-success", "minVisitCountSuccess", ParamType::INT, &minVisitCountSuccess, 1, "Minimum number of bee visits for successful pollination");
    REGISTRY.emplace_back("max-visit-count-success", "maxVisitCountSuccess", ParamType::INT, &maxVisitCountSuccess, 1000, "Maximum number of bee visits for successful pollination");
    REGISTRY.emplace_back("num-trials-per-config", "numTrialsPerConfig", ParamType::INT, &numTrialsPerConfig, 1, "Number of trials to run for each configuration/individual in each generation");
    REGISTRY.emplace_back("num-configs-per-gen", "numConfigsPerGen", ParamType::INT, &numConfigsPerGen, 50, "Number of configurations/inidividuals to test during each generation (if using multiple islands, this is the number per island)");
    REGISTRY.emplace_back("num-generations", "numGenerations", ParamType::INT, &numGenerations, 50, "Number of generations to run the optimization process");
    REGISTRY.emplace_back("num-islands", "numIslands", ParamType::INT, &numIslands, 1, "Number of islands of evolving populations (when num-islands=1, there is just a single population with no migration)");
    REGISTRY.emplace_back("migration-period", "migrationPeriod", ParamType::INT, &migrationPeriod, 10, "Period (number of generations) between each migration event when using multiple islands");
    REGISTRY.emplace_back("migration-num-replace", "migrationNumReplace", ParamType::INT, &migrationNumReplace, 1, "Number of individuals on an Island that can be replaced by migrants at each migration event");
    REGISTRY.emplace_back("use-diverse-algorithms", "useDiverseAlgorithms", ParamType::BOOL, &useDiverseAlgorithms, false, "Use diverse optimisation algorithms on each island (when num-islands > 1)");
    REGISTRY.emplace_back("migration-num-select", "migrationNumSelect", ParamType::INT, &migrationNumSelect, 1, "Number of individuals on an Island that can be selected for migration at each migration event");
    REGISTRY.emplace_back("target-heatmap-filename", "strTargetHeatmapFilename", ParamType::STRING, &strTargetHeatmapFilename, "", "CSV file containing target heatmap for optimization");
    REGISTRY.emplace_back("heatmap-cell-size", "heatmapCellSize", ParamType::INT, &heatmapCellSize, 10, "Size of each cell in the heatmap of bee positions");
    REGISTRY.emplace_back("visualise", "bVis", ParamType::BOOL, &bVis, true, "Determines whether graphical output is displayed");
    REGISTRY.emplace_back("vis-cell-size", "visCellSize", ParamType::FLOAT, &visCellSize, 1.0f, "Size of an individual cell for visualisation");
    REGISTRY.emplace_back("vis-delay-per-step", "visDelayPerStep", ParamType::INT, &visDelayPerStep, 100, "Delay (in milliseconds) per step when visualising");
    REGISTRY.emplace_back("vis-bee-path-draw-len", "visBeePathDrawLen", ParamType::INT, &visBeePathDrawLen, 250, "Maximum number of path segments to draw for each bee");
    REGISTRY.emplace_back("logging", "logging", ParamType::BOOL, &logging, true, "Determines whether output files are written at the end of a run");
    REGISTRY.emplace_back("log-dir", "logDir", ParamType::STRING, &logDir, ".", "Directory for output files");
    REGISTRY.emplace_back("log-filename-prefix", "logFilenamePrefix", ParamType::STRING, &logFilenamePrefix, "polybee", "Prefix for output file names");
    REGISTRY.emplace_back("rng-seed", "strRngSeed", ParamType::STRING, &strRngSeed, "", "Seed (an alphanumeric string) for random number generator");
    REGISTRY.emplace_back("command-line-quiet", "bCommandLineQuiet", ParamType::BOOL, &bCommandLineQuiet, false, "Silence messages to command line");
}


// Helper function to parse hive positions from strings of the form "x,y:d" (x and y are floats, d in int)
std::vector<HiveSpec> parse_hive_positions(const std::vector<std::string>& hive_strings) {
    std::vector<HiveSpec> hives;
    std::regex pos_regex(R"((\d+|\d+\.\d+),(\d+|\d+\.\d+):([0-4]))");

    for (const auto& hive_str : hive_strings) {
        std::smatch match;
        if (std::regex_match(hive_str, match, pos_regex)) {
            hives.emplace_back(std::stof(match[1]), std::stof(match[2]), std::stoi(match[3]));
        } else {
            throw std::invalid_argument("Invalid hive specification: " + hive_str);
        }
    }
    return hives;
}

// Helper function to parse tunnel entrance positions from strings of the form "e1,e2:s[:t]"
// (e1 and e2 are floats, s is side int 0-3, t is optional net type int 0=NONE, 1=ANTIBIRD, 2=ANTIHAIL)
std::vector<TunnelEntranceSpec> parse_tunnel_entrance_positions(const std::vector<std::string>& tunnel_strings) {
    std::vector<TunnelEntranceSpec> entrances;
    std::string regex_str_p1 = R"((\d+|\d+\.\d+),(\d+|\d+\.\d+):([0-3]))";  // e1,e2:s
    std::string regex_str_p2 = R"(:([0-2]))";                               // :t (net type)

    std::regex pos_regex_basic(regex_str_p1);
    std::regex pos_regex_with_net(regex_str_p1 + regex_str_p2);

    for (const auto& tunnel_str : tunnel_strings) {
        std::smatch match;
        if (std::regex_match(tunnel_str, match, pos_regex_with_net)) {
            NetType netType = static_cast<NetType>(std::stoi(match[4]));
            entrances.emplace_back(std::stof(match[1]), std::stof(match[2]), std::stoi(match[3]), netType);
        } else if (std::regex_match(tunnel_str, match, pos_regex_basic)) {
            entrances.emplace_back(std::stof(match[1]), std::stof(match[2]), std::stoi(match[3]));
        } else {
            throw std::invalid_argument("Invalid tunnel entrance specification: " + tunnel_str);
        }
    }
    return entrances;
}

// Helper function to parse patch specifications from strings of the form "x,y,w,h:r[:j[:s[:n:dx,dy]]]"
// (x,y,w,h,r,j are floats, s,n are ints, dx,dy are floats)
std::vector<PatchSpec> parse_patch_positions(const std::vector<std::string>& patch_strings) {
    std::vector<PatchSpec> patches;
    std::string regex_str_p1 = R"((\d+|\d+\.\d+),(\d+|\d+\.\d+),(\d+|\d+\.\d+),(\d+|\d+\.\d+):(\d*\.?\d+))"; // x,y,w,h:r
    std::string regex_str_p2 = R"(:(\d*\.?\d+)?)";                       // :j
    std::string regex_str_p3 = R"(:(\d+)?)";                             // :s
    std::string regex_str_p4 = R"(:(\d+)?:(\d*\.?\d+)?,(\d*\.?\d+)?)";   // :n:dx,dy

    std::regex patch_regex_1(regex_str_p1);
    std::regex patch_regex_2(regex_str_p1 + regex_str_p2);
    std::regex patch_regex_3(regex_str_p1 + regex_str_p2 + regex_str_p3);
    std::regex patch_regex_4(regex_str_p1 + regex_str_p2 + regex_str_p3 + regex_str_p4);

    for (const auto& patch_str : patch_strings) {
        std::smatch match;
        if (std::regex_match(patch_str, match, patch_regex_4)) {
            patches.emplace_back(std::stof(match[1]), std::stof(match[2]), std::stof(match[3]), std::stof(match[4]),
                std::stof(match[5]), std::stof(match[6]), std::stoi(match[7]), std::stoi(match[8]), std::stof(match[9]),
                std::stof(match[10]));
        } else if (std::regex_match(patch_str, match, patch_regex_3)) {
            patches.emplace_back(std::stof(match[1]), std::stof(match[2]), std::stof(match[3]), std::stof(match[4]),
                std::stof(match[5]), std::stof(match[6]), std::stoi(match[7]));
        } else if (std::regex_match(patch_str, match, patch_regex_2)) {
            patches.emplace_back(std::stof(match[1]), std::stof(match[2]), std::stof(match[3]), std::stof(match[4]),
                std::stof(match[5]), std::stof(match[6]));
        } else if (std::regex_match(patch_str, match, patch_regex_1)) {
            patches.emplace_back(std::stof(match[1]), std::stof(match[2]), std::stof(match[3]), std::stof(match[4]),
                std::stof(match[5]));
        } else {
            throw std::invalid_argument("Invalid patch specification: " + patch_str);
        }
    }
    return patches;
}


// Initialise all parameters using configuartion file and command line specs if given, otherwise
// using default values
void Params::initialise(int argc, char* argv[])
{
    initRegistry();
    setAllDefault();
    calculateDerivedParams();

    try
    {
        // Declare a group of options that will be allowed only on command line
        po::options_description generic("Generic options");
        generic.add_options()
            ("version,v", "Show program version number")
            ("help,h", "Show this help message")
            ("config-filename,c", po::value<std::string>(&Params::strConfigFilename), "Name of configuration file");

        // Declare a group of options that will be allowed both on command line and in config file.
        // These are taken from the global registry stored in the Params class
        po::options_description config("Configuration");

        for (auto& vinfo : Params::REGISTRY)
        {
            switch (vinfo.type)
            {
            case ParamType::BOOL:
            {
                config.add_options()
                    (vinfo.uname.c_str(), po::value<bool>((bool*)(vinfo.pVar))->default_value(std::get<bool>(vinfo.defaultVal)), vinfo.desc.c_str());
                break;
            }
            case ParamType::INT:
            {
                config.add_options()
                    (vinfo.uname.c_str(), po::value<int>((int*)(vinfo.pVar))->default_value(std::get<int>(vinfo.defaultVal)), vinfo.desc.c_str());
                break;
            }
            case ParamType::FLOAT:
            {
                config.add_options()
                    (vinfo.uname.c_str(), po::value<float>((float*)(vinfo.pVar))->default_value(std::get<float>(vinfo.defaultVal)), vinfo.desc.c_str());
                break;
            }
            case ParamType::STRING:
            {
                config.add_options()
                    (vinfo.uname.c_str(), po::value<std::string>((std::string*)(vinfo.pVar))->default_value(std::get<std::string>(vinfo.defaultVal)), vinfo.desc.c_str());
                break;
            }
            default:
                std::cerr << "Unkown type encountered in Params Registry when building program options!" << std::endl;
                exit(1);
            }
        }

        // Special case for hive positions (multiple allowed)
        config.add_options()
            ("hive", po::value<std::vector<std::string>>()->multitoken(),
             "Hive specification in format x,y:d where d is the direction of the opening (0=North, 1=East, 2=South, 3=West, 4=Random), e.g., --hive 10,8:0 --hive 4,6:4");

        // Special case for tunnel entrance positions (multiple allowed)
        config.add_options()
            ("tunnel-entrance", po::value<std::vector<std::string>>()->multitoken(),
             "Tunnel entrance specification in format e1,e2:s[:t] where e1 and e2 are positions of edges of entrance along the specified side of tunnel (position being the distance measured from one end of that side), and s is the side (0=North, 1=East, 2=South, 3=West), e.g., --tunnel-entrance 5.5,10.0:0 --tunnel-entrance 3.0,6.0:2. The optional t specifies the net type (0=NONE, 1=ANTIBIRD, 2=ANTIHAIL), e.g., --tunnel-entrance 5.5,10.0:0:1. Default value if not specified is 0 (NONE)");

        // Special case for plant patch defintions (multiple allowed)
        config.add_options()
            ("patch", po::value<std::vector<std::string>>()->multitoken(),
             "Plant patch specification in format x,y,w,h:r[:j[:s[:n:dx,dy]]] where x,y,w,h define the"
             "top-left corner of the patch (in environment coordinates)"
             "r is spacing between plants, j is jitter between plants (std dev, default 0.0), s is species id (default 1)"
             "n in the number of repeats of the patch (default 1), and dx,dy is the offset between repeats"
             "e.g. --patch 200,200,50,400:2:0.5:1:3:100,0");


        // Hidden options, will be allowed both on command line and
        // in config file, but will not be shown to the user.
        po::options_description hidden("Hidden options");
        // TODO: may want to add some hidden options later on to allow saved organism files to
        // be specified on the command line to be loaded and run?
        /*
        hidden.add_options()
            ("input-file", po::value< std::vector<std::string> >(), "input file")
            ;
        */

        po::options_description cmdline_options;
        cmdline_options.add(generic).add(config).add(hidden);

        po::options_description config_file_options;
        config_file_options.add(config).add(hidden);

        po::options_description visible("Allowed options");
        visible.add(generic).add(config);

        /*
        po::positional_options_description p;
        p.add("input-file", -1);
        */

        po::variables_map vm;
        // store(po::command_line_parser(ac, av).options(cmdline_options).positional(p).run(), vm);
        store(po::command_line_parser(argc, argv).options(cmdline_options).run(), vm);
        notify(vm);

        if (!(Params::strConfigFilename.empty()))
        {
            std::ifstream ifs(Params::strConfigFilename.c_str());
            if (!ifs)
            {
                std::cerr << "Cannot open config file: " << Params::strConfigFilename << "\n";
                exit(0);
            }
            else
            {
                store(parse_config_file(ifs, config_file_options), vm);
                notify(vm);
            }
        }

        if (vm.count("hive")) {
            hiveSpecs = parse_hive_positions(vm["hive"].as<std::vector<std::string>>());
        }

        if (vm.count("tunnel-entrance")) {
            tunnelEntranceSpecs = parse_tunnel_entrance_positions(vm["tunnel-entrance"].as<std::vector<std::string>>());
        }

        if (vm.count("patch")) {
            patchSpecs = parse_patch_positions(vm["patch"].as<std::vector<std::string>>());
        }

        if (vm.count("help"))
        {
            std::cout << visible << "\n";
            exit(0);
        }

        if (vm.count("version"))
        {
            std::cout << std::format("Polybee version {}", polybee_VERSION_STR) << std::endl;
            exit(0);
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Error in Params::initialise: " << e.what() << "\n";
        exit(1);
    }
    catch (...)
    {
        std::cerr << "Exception of unknown type encountered in Params::initialise!\n";
        exit(1);
    }

    // when all specified parameters have been dealt with, we need to update
    // the values of derived parameters as well
    calculateDerivedParams();

    // check parameter consistency
    checkConsistency();

    bInitialised = true;
}


// methods
void Params::calculateDerivedParams()
{
    switch (evolveObjectivePvt) {
    case 0:
        evolveObjective = EvolveObjective::EMD_TO_TARGET_HEATMAP;
        break;
    case 1:
        evolveObjective = EvolveObjective::FRACTION_FLOWERS_SUCCESSFUL_VISIT_RANGE;
        break;
    default:
        pb::msg_error_and_exit(std::format(
            "Invalid value for evolve-objective: {}. Valid values are 0=EMD to target heatmap, "
            "1=Fraction of flowers in successful visit range",
            evolveObjectivePvt)
        );
    }
}


void Params::print(std::ostream& os, bool bGenerateForConfigFile)
{
    auto valsep = bGenerateForConfigFile ? "=" : ": ";
    auto linesep = '\n';
    auto coordOpen = bGenerateForConfigFile ? "" : "(";
    auto coordClose = bGenerateForConfigFile ? "" : ")";

    if (!bGenerateForConfigFile) {
        os << "config-filename" << valsep << strConfigFilename << linesep;
    }

    for (auto& vinfo : REGISTRY) {
        os << vinfo.uname << valsep << vinfo.valueAsStr() << linesep;
    }

    // print hive info
    if (!bGenerateForConfigFile) {
        os << "Hives:" << linesep;
    }

    if (!hiveSpecs.empty()) {
        for (size_t i = 0; i < hiveSpecs.size(); ++i) {
            os << "hive";
            if (!bGenerateForConfigFile) {
                os << (i+1);
            }
            os << valsep << coordOpen << hiveSpecs[i].x << "," << hiveSpecs[i].y << coordClose << ":" << hiveSpecs[i].direction << linesep;
        }
    }
    else {
        if (!bGenerateForConfigFile) {
            os << "(none)" << linesep;
        }
    }

    // print tunnel entrance info
    if (!bGenerateForConfigFile) {
        os << "Tunnel Entrances:" << linesep;
    }

    if (!tunnelEntranceSpecs.empty()) {
        for (size_t i = 0; i < tunnelEntranceSpecs.size(); ++i) {
            os << "tunnel-entrance";
            if (!bGenerateForConfigFile) {
                os << (i+1);
            }
            os << valsep << coordOpen << tunnelEntranceSpecs[i].e1 << "," << tunnelEntranceSpecs[i].e2 << coordClose
                << ":" << tunnelEntranceSpecs[i].side
                << ":" << static_cast<int>(tunnelEntranceSpecs[i].netType) << linesep;
        }
    }
    else {
        if (!bGenerateForConfigFile) {
            os << "(none)" << linesep;
        }
    }

    // print patch info
    if (!bGenerateForConfigFile) {
        os << "Plant Patches:" << linesep;
    }

    if (!patchSpecs.empty()) {
        for (size_t i = 0; i < patchSpecs.size(); ++i) {
            os << "patch";
            if (!bGenerateForConfigFile) {
                os << (i+1);
            }
            os << valsep << coordOpen
               << patchSpecs[i].x << "," << patchSpecs[i].y << "," << patchSpecs[i].w << "," << patchSpecs[i].h << coordClose
               << ":" << patchSpecs[i].spacing
               << ":" << patchSpecs[i].jitter
               << ":" << patchSpecs[i].speciesID
               << ":" << patchSpecs[i].numRepeats
               << ":" << coordOpen << patchSpecs[i].dx << "," << patchSpecs[i].dy << coordClose
               << linesep;
        }
    }
    else {
        if (!bGenerateForConfigFile) {
            os << "(none)" << linesep;
        }
    }
}


void Params::setAllDefault()
{
    for (auto& vinfo : Params::REGISTRY)
    {
        switch (vinfo.type)
        {
        case ParamType::BOOL:
            *((bool*)(vinfo.pVar)) = std::get<bool>(vinfo.defaultVal);
            break;
        case ParamType::INT:
            *((int*)(vinfo.pVar)) = std::get<int>(vinfo.defaultVal);
            break;
        case ParamType::FLOAT:
            *((float*)(vinfo.pVar)) = std::get<float>(vinfo.defaultVal);
            break;
        case ParamType::STRING:
            *((std::string*)(vinfo.pVar)) = std::get<std::string>(vinfo.defaultVal);
            break;
        default:
            std::cerr << "Unexpected type " << vinfo.type << " encountered in Params::setAllDefault!" << std::endl;
            exit(1);
        }
    }
}


void Params::checkConsistency()
{
    // check whether logDir ends with a '/' and remove it if so
    if (!logDir.empty() && logDir.back() == '/') {
        logDir.pop_back();
    }

    // check whether logDir exists and is a directory, create it if necessary
    if (!logDir.empty()) {
        std::filesystem::path dir_path(logDir);
        std::error_code ec;

        if (!std::filesystem::exists(dir_path, ec)) {
            // Directory doesn't exist, try to create it
            if (std::filesystem::create_directories(dir_path, ec)) {
                pb::msg_info(std::format("Created log directory: {}", logDir));
            } else {
                pb::msg_error_and_exit(std::format("Failed to create log directory '{}': {}", logDir, ec.message()));
            }
        } else if (!std::filesystem::is_directory(dir_path, ec)) {
            // Path exists but is not a directory
            pb::msg_error_and_exit(std::format("Log directory path '{}' exists but is not a directory", logDir));
        }
    }

    // check at least one hive is specified
    if (hiveSpecs.empty()) {
        pb::msg_error_and_exit("At least one hive must be specified using the 'hive' parameter");
    }

    // if we're doing optimisation, check that the necessary parameters are set and valid
    if (bEvolve) {
        if (strTargetHeatmapFilename.empty()) {
            pb::msg_error_and_exit("Parameter 'target-heatmap-filename' must be specified if 'evolve' is true");
        }
        if (evolveObjectivePvt < 0 || evolveObjectivePvt > 1) {
            pb::msg_error_and_exit("Parameter 'evolve-objective' must be 0 (EMD to target heatmap) or 1 (Fraction of flowers in successful visit range)");
        }
        if (numConfigsPerGen < 7) {
            pb::msg_error_and_exit("Parameter 'num-trials-per-gen' must be greater than or equal to 7 if 'evolve' is true");
        }
        if (numGenerations <= 0) {
            pb::msg_error_and_exit("Parameter 'num-generations' must be greater than zero if 'evolve' is true");
        }
        if (numIslands <= 0) {
            pb::msg_error_and_exit("Parameter 'num-islands' must be greater than zero if 'evolve' is true");
        }
        if (migrationPeriod <= 0 && numIslands > 1) {
            pb::msg_error_and_exit("Parameter 'migration-period' must be greater than zero if 'num-islands' is greater than 1");
        }
    }

    if (bVis) {
        if (visBeePathDrawLen > beePathRecordLen) {
            pb::msg_warning(
                std::format("vis-bee-path-draw-len ({0}) cannot be larger than bee-path-record-len ({1}). Resetting vis-bee-path-draw-len to {1}.",
                visBeePathDrawLen, beePathRecordLen));
            visBeePathDrawLen = beePathRecordLen;
        }

        if (visDelayPerStep < 0) {
            pb::msg_warning("Parameter 'vis-delay-per-step' is negative, setting it to zero");
            visDelayPerStep = 0;
        }
    }
    else { // !bVis
        if (beePathRecordLen > 0) {
            beePathRecordLen = 0; // no need to record bee paths if not visualising (silently set to zero)
        }
        if (visDelayPerStep > 0) {
            visDelayPerStep = 0; // no need to delay visualisation if not visualising (silently set to zero)
        }
    }
}


/////////////////////////////////////////////////////////////////
// ParamInfo methods

std::string ParamInfo::valueAsStr()
{
    std::ostringstream ss;

    switch (type)
    {
    case ParamType::BOOL:
        ss << ((*((bool*)pVar)) ? "true" : "false");
        break;
    case ParamType::INT:
        ss << *((int*)pVar);
        break;
    case ParamType::FLOAT:
        ss << *((float*)pVar);
        break;
    case ParamType::STRING:
        ss << *((std::string*)pVar);
        break;
    default:
        ss << "UNKOWN_TYPE";
    }

    return ss.str();
}