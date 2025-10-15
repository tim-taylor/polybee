#include "Params.h"
#include "utils.h"
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
int Params::envW;
int Params::envH;
int Params::tunnelW;
int Params::tunnelH;
int Params::tunnelX;
int Params::tunnelY;

// Bee configuration
int Params::numBees;
float Params::beeMaxDirDelta;
int Params::beePathRecordLen;

// Hive configuration
std::vector<HiveSpec> Params::hiveSpecs;

// Optimization
bool Params::bEvolve;
std::string Params::strTargetHeatmapFilename;
int Params::numConfigsPerGen;
int Params::numTrialsPerConfig;
int Params::numGenerations;

// Logging and output
int Params::heatmapCellSize;
std::string Params::logDir;
std::string Params::logFilenamePrefix;
bool Params::logging;
bool Params::bCommandLineQuiet;

// Visualisation
bool Params::bVis;
int Params::visCellSize;
int Params::visDelayPerStep;
int Params::visBeePathDrawLen;
float Params::visBeePathThickness;

// Options that can be specified on command line but are not in a config file
std::string Params::strConfigFilename = "polybee.cfg";

// Derived and other internal parameters (not set by user, calculated internally)
bool Params::bInitialised = false;

// private variables (not set by user, used internally)
std::vector<ParamInfo> Params::REGISTRY;

/////////////////////////////////////////////////////////////////
// Params methods

// Initial registratin of all primary (non-derived) parameters and associated details
void Params::initRegistry()
{
    REGISTRY.emplace_back("env-width", "envW", ParamType::INT, &envW, 450, "Width (number of cells) of environment");
    REGISTRY.emplace_back("env-height", "envH", ParamType::INT, &envH, 250, "Height (number of cells) of environment");
    REGISTRY.emplace_back("tunnel-width", "tunnelW", ParamType::INT, &tunnelW, 50, "Width (number of cells) of tunnel");
    REGISTRY.emplace_back("tunnel-height", "tunnelH", ParamType::INT, &tunnelH, 50, "Height (number of cells) of tunnel");
    REGISTRY.emplace_back("tunnel-x", "tunnelX", ParamType::INT, &tunnelX, 200, "X position of left edge of tunnel");
    REGISTRY.emplace_back("tunnel-y", "tunnelY", ParamType::INT, &tunnelY, 100, "Y position of top edge of tunnel");
    REGISTRY.emplace_back("num-bees", "numBees", ParamType::INT, &numBees, 50, "Number of bees in the simulation");
    REGISTRY.emplace_back("bee-max-dir-delta", "beeMaxDirDelta", ParamType::FLOAT, &beeMaxDirDelta, 0.4f, "Maximum change in direction (radians) per step");
    REGISTRY.emplace_back("bee-path-record-len", "beePathRecordLen", ParamType::INT, &beePathRecordLen, 250, "Maximum number of positions to record in bee's path");
    REGISTRY.emplace_back("num-iterations", "numIterations", ParamType::INT, &numIterations, 100, "Number of iterations to run the simulation");
    REGISTRY.emplace_back("evolve", "bEvolve", ParamType::BOOL, &bEvolve, false, "Run optimization to match output heatmap against target heatmap");
    REGISTRY.emplace_back("target-heatmap-filename", "strTargetHeatmapFilename", ParamType::STRING, &strTargetHeatmapFilename, "", "CSV file containing target heatmap for optimization");
    REGISTRY.emplace_back("num-trials-per-config", "numTrialsPerConfig", ParamType::INT, &numTrialsPerConfig, 1, "Number of trials to run for each configuration/individual in each generation");
    REGISTRY.emplace_back("num-configs-per-gen", "numConfigsPerGen", ParamType::INT, &numConfigsPerGen, 50, "Number of configurations/inidividuals to test during each generation");
    REGISTRY.emplace_back("num-generations", "numGenerations", ParamType::INT, &numGenerations, 50, "Number of generations to run the optimization process");
    REGISTRY.emplace_back("visualise", "bVis", ParamType::BOOL, &bVis, true, "Determines whether graphical output is displayed");
    REGISTRY.emplace_back("vis-cell-size", "visCellSize", ParamType::INT, &visCellSize, 4, "Size of an individual cell for visualisation");
    REGISTRY.emplace_back("vis-delay-per-step", "visDelayPerStep", ParamType::INT, &visDelayPerStep, 100, "Delay (in milliseconds) per step when visualising");
    REGISTRY.emplace_back("vis-bee-path-draw-len", "visBeePathDrawLen", ParamType::INT, &visBeePathDrawLen, 250, "Maximum number of path segments to draw for each bee");
    REGISTRY.emplace_back("vis-bee-path-thickness", "visBeePathThickness", ParamType::FLOAT, &visBeePathThickness, 5.0f, "Thickness of bee path lines");
    REGISTRY.emplace_back("heatmap-cell-size", "heatmapCellSize", ParamType::INT, &heatmapCellSize, 10, "Size of each cell in the heatmap of bee positions");
    REGISTRY.emplace_back("logging", "logging", ParamType::BOOL, &logging, true, "Determines whether output files are written at the end of a run");
    REGISTRY.emplace_back("log-dir", "logDir", ParamType::STRING, &logDir, ".", "Directory for output files");
    REGISTRY.emplace_back("log-filename-prefix", "logFilenamePrefix", ParamType::STRING, &logFilenamePrefix, "polybee", "Prefix for output file names");
    REGISTRY.emplace_back("rng-seed", "strRngSeed", ParamType::STRING, &strRngSeed, "", "Seed (an alphanumeric string) for random number generator");
    REGISTRY.emplace_back("command-line-quiet", "bCommandLineQuiet", ParamType::BOOL, &bCommandLineQuiet, false, "Silence messages to command line");
}


// Helper function to parse hive positions from strings of the form "x,y" (x and y are floats)
std::vector<HiveSpec> parse_positions(const std::vector<std::string>& pos_strings) {
    std::vector<HiveSpec> hives;
    std::regex pos_regex(R"((\d+|\d+\.\d+),(\d+|\d+\.\d+):([0-4]))");

    for (const auto& pos_str : pos_strings) {
        std::smatch match;
        if (std::regex_match(pos_str, match, pos_regex)) {
            hives.emplace_back(std::stof(match[1]), std::stof(match[2]), std::stoi(match[3]));
        } else {
            throw std::invalid_argument("Invalid hive specification: " + pos_str);
        }
    }
    return hives;
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
            hiveSpecs = parse_positions(vm["hive"].as<std::vector<std::string>>());
        }

        if (vm.count("help"))
        {
            std::cout << visible << "\n";
            exit(0);
        }

        if (vm.count("version"))
        {
            // TODO get the version number from somewhere reliable!
            std::cout << "PolyBeeCore server, version 0.1\n";
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

    if (bEvolve) {
        if (strTargetHeatmapFilename.empty()) {
            pb::msg_error_and_exit("Parameter 'target-heatmap-filename' must be specified if 'evolve' is true");
        }
        if (numConfigsPerGen < 7) {
            pb::msg_error_and_exit("Parameter 'num-trials-per-gen' must be greater than or equal to 7 if 'evolve' is true");
        }
        if (numGenerations <= 0) {
            pb::msg_error_and_exit("Parameter 'num-generations' must be greater than zero if 'evolve' is true");
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