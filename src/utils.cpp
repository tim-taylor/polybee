#include "utils.h"

#include <iostream>

void msg_error_and_exit(std::string msg) {
    std::cerr << "ERROR: " << msg << std::endl;
    exit(EXIT_FAILURE);
}

void msg_warning(std::string msg) {
    std::cerr << "WARNING: " << msg << std::endl;
}
