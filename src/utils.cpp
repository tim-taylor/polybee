#include "utils.h"
#include "Params.h"
#include <iostream>


namespace Polybee {

void msg_error_and_exit(std::string msg) {
    std::cerr << "ERROR: " << msg << std::endl;
    exit(EXIT_FAILURE);
}

void msg_warning(std::string msg) {
    std::cerr << "WARNING: " << msg << std::endl;
}

void msg_info(std::string msg) {
    if (!Params::bCommandLineQuiet) {
        std::cout << "INFO: " << msg << std::endl;
    }
}


} // namespace Polybee
