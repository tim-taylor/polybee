#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

constexpr float FLOAT_COMPARISON_EPSILON = 0.00000001f; // small value to use when comparing floats for equality

template <typename T>
struct Position {
    T x, y;
    Position(T x, T y) : x(x), y(y) {}
};

using iPos = Position<int>;
using fPos = Position<float>;

namespace Polybee {

void msg_error_and_exit(std::string msg);
void msg_warning(std::string msg);
void msg_info(std::string msg);

float earthMoversDistanceLemon(const std::vector<std::vector<int>>& heatmap1,
                            const std::vector<std::vector<int>>& heatmap2);


//
float earthMoversDistanceApprox(const std::vector<std::vector<float>>& heatmap1,
                            const std::vector<std::vector<float>>& heatmap2);

float earthMoversDistanceFull(const std::vector<std::vector<float>>& heatmap1,
                            const std::vector<std::vector<float>>& heatmap2);
//


} // namespace Polybee

namespace pb = Polybee;

#endif // UTILS_H
