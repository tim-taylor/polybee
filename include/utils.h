#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

constexpr float FLOAT_COMPARISON_EPSILON = 0.000001f; // small value to use when comparing floats for equality

namespace Polybee {

    void msg_error_and_exit(std::string msg);
    void msg_warning(std::string msg);
    void msg_info(std::string msg);

    float distanceSq(float x1, float y1, float x2, float y2);

    struct Pos2D {
        float x, y;

        Pos2D(float x = 0, float y = 0) : x(x), y(y) {}

        void resize(float newLength);
    };

    struct PosAndDir2D {
        float x, y;
        float angle; // direction in radians

        PosAndDir2D(float x = 0, float y = 0, float angle = 0) : x(x), y(y), angle(angle) {}
    };

    struct Line2D {
        Pos2D start, end;

        Line2D(Pos2D start, Pos2D end) : start(start), end(end) {}
    };

} // namespace Polybee

namespace pb = Polybee;

#endif // UTILS_H
