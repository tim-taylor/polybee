#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <algorithm>

constexpr float FLOAT_COMPARISON_EPSILON = 0.000001f; // small value to use when comparing floats for equality

namespace Polybee {

    void msg_error_and_exit(std::string msg);
    void msg_warning(std::string msg);
    void msg_info(std::string msg);

    template<typename T>
    T median(const std::vector<T>& values) {
        if (values.empty()) {
            return T(0);
        }

        std::vector<T> sorted = values;
        std::sort(sorted.begin(), sorted.end());

        size_t n = sorted.size();
        if (n % 2 == 0) {
            // Even number of elements: return average of two middle values
            return (sorted[n/2 - 1] + sorted[n/2]) / static_cast<T>(2);
        } else {
            // Odd number of elements: return middle value
            return sorted[n/2];
        }
    }

    float distanceSq(float x1, float y1, float x2, float y2);

    struct Pos2D {
        float x, y;

        Pos2D(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}

        float length() const;
        void resize(float newLength);
        void set(float newX, float newY) { x = newX; y = newY; }
        void setToZero() { x = 0.0f; y = 0.0f; }
    };

    struct PosAndDir2D {
        float x, y;
        float angle; // direction in radians

        PosAndDir2D(float x = 0.0f, float y = 0.0f, float angle = 0.0f) : x(x), y(y), angle(angle) {}

        void setToZero() { x = 0.0f; y = 0.0f; angle = 0.0f; }
    };

    struct Line2D {
        Pos2D start, end;

        Line2D() {}
        Line2D(Pos2D start, Pos2D end) : start(start), end(end) {}
    };

} // namespace Polybee

namespace pb = Polybee;

#endif // UTILS_H
