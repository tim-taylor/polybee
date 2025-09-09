#ifndef UTILS_H
#define UTILS_H

template <typename T>
struct Position {
    T x, y;
    Position(T x, T y) : x(x), y(y) {}
};

using iPos = Position<int>;
using fPos = Position<float>;

#endif // UTILS_H
