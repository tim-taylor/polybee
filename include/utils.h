#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

constexpr float FLOAT_COMPARISON_EPSILON = 0.00000001f; // small value to use when comparing floats for equality
constexpr float SMALL_FLOAT_NUMBER = 0.0001f;  // small value to use to avoid division by zero and other edge cases

template <typename T>
struct Position {
    T x, y;
    Position(T x, T y) : x(x), y(y) {}
};

using iPos = Position<int>;
using fPos = Position<float>;

void msg_error_and_exit(std::string msg);
void msg_warning(std::string msg);
void msg_info(std::string msg);

float earthMoversDistanceLemon(const std::vector<std::vector<float>>& heatmap1,
                               const std::vector<std::vector<float>>& heatmap2);

float earthMoversDistanceApprox(const std::vector<std::vector<float>>& heatmap1,
                               const std::vector<std::vector<float>>& heatmap2);

float earthMoversDistanceFull(const std::vector<std::vector<float>>& heatmap1,
                             const std::vector<std::vector<float>>& heatmap2);

#endif // UTILS_H
