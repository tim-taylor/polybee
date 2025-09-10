#include "Params.h"
#include "utils.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <regex>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

// Instantiate all static members
// Note, default values are assigned in Params::initRegistry()

// Simulation control
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

// Hive configuration
std::vector<fPos> Params::hivePositions;

bool Params::bVis;
int Params::visCellSize;
int Params::visTargetFPS;

std::string Params::strConfigFilename = "polybee.cfg";
std::string Params::strRngSeed;

bool Params::bCommandLineQuiet;

bool Params::bInitialised = false;

// private variables
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
    REGISTRY.emplace_back("num-iterations", "numIterations", ParamType::INT, &numIterations, 100, "Number of iterations to run the simulation");
    REGISTRY.emplace_back("visualise", "bVis", ParamType::BOOL, &bVis, true, "Determines whether graphical output is displayed");
    REGISTRY.emplace_back("vis-cell-size", "visCellSize", ParamType::INT, &visCellSize, 4, "Size of an individual cell for visualisation");
    REGISTRY.emplace_back("vis-target-fps", "visTargetFPS", ParamType::INT, &visTargetFPS, 100, "Target frames per second for visualisation");
    REGISTRY.emplace_back("rng-seed", "strRngSeed", ParamType::STRING, &strRngSeed, "", "Seed (an alphanumeric string) for random number generator");
    REGISTRY.emplace_back("command-line-quiet", "bCommandLineQuiet", ParamType::BOOL, &bCommandLineQuiet, false, "Silence messages to command line");
}


// Helper function to parse hive positions from strings of the form "x,y" (x and y are floats)
std::vector<fPos> parse_positions(const std::vector<std::string>& pos_strings) {
    std::vector<fPos> positions;
    std::regex pos_regex(R"((\d+|\d+\.\d+),(\d+|\d+\.\d+))");

    for (const auto& pos_str : pos_strings) {
        std::smatch match;
        if (std::regex_match(pos_str, match, pos_regex)) {
            positions.emplace_back(std::stof(match[1]), std::stof(match[2]));
        } else {
            throw std::invalid_argument("Invalid position format: " + pos_str);
        }
    }
    return positions;
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
             "Hive positions in format x,y, e.g., --hive 10,8 --hive 4,6");

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
            auto positions = parse_positions(vm["hive"].as<std::vector<std::string>>());
            for (size_t i = 0; i < positions.size(); ++i) {
                hivePositions.push_back(positions[i]);
            }
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

    bInitialised = true;
}

// methods
void Params::calculateDerivedParams()
{
}

void Params::print(std::ostream& os)
{
    auto valsep = ": ";
    auto linesep = '\n';

    os << "config-filename" << valsep << strConfigFilename << linesep;

    for (auto& vinfo : REGISTRY)
    {
        os << vinfo.uname << valsep << vinfo.valueAsStr() << linesep;
    }

    os << "Hives:" << linesep;
    if (hivePositions.empty()) {
        os << "(none)" << linesep;
    }
    else {
        for (size_t i = 0; i < hivePositions.size(); ++i) {
            os << "hive" << (i+1) << valsep << "(" << hivePositions[i].x << ", " << hivePositions[i].y << ")" << linesep;
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