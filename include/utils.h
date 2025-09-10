#ifndef UTILS_H
#define UTILS_H

#include <string>

template <typename T>
struct Position {
    T x, y;
    Position(T x, T y) : x(x), y(y) {}
};

using iPos = Position<int>;
using fPos = Position<float>;

void msg_error_and_exit(std::string msg);
void msg_warning(std::string msg);

#endif // UTILS_H
