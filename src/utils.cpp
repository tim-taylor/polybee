#include "utils.h"
#include "Params.h"
#include <iostream>
#include <cmath>


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

    float distanceSq(float x1, float y1, float x2, float y2) {
        float dx = x2 - x1;
        float dy = y2 - y1;
        return dx * dx + dy * dy;
    }

    float Pos2D::length() const {
        return std::sqrt(x * x + y * y);
    }

    void Pos2D::resize(float newLength) {
        float currentLength = length();
        if (currentLength < FLOAT_COMPARISON_EPSILON) {
            // zero-length vector, cannot resize
            return;
        }
        float scale = newLength / currentLength;
        x *= scale;
        y *= scale;
    }

} // namespace Polybee
