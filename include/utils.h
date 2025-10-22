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

    struct Vector2 {
        Vector2() : x(0), y(0) {}
        Vector2(float x, float y) : x(x), y(y) {}
        Vector2(const Vector2& other) : x(other.x), y(other.y) {}
        Vector2& operator=(const Vector2& other) {
            if (this != &other) {
                x = other.x;
                y = other.y;
            }
            return *this;
        }

        float x;
        float y;
    };

    struct Rectangle {
        Rectangle() : x(0), y(0), width(0), height(0) {}
        Rectangle(float x, float y, float width, float height)
            : x(x), y(y), width(width), height(height) {}
        Rectangle(const Rectangle& other)
            : x(other.x), y(other.y), width(other.width), height(other.height) {}
        Rectangle& operator=(const Rectangle& other) {
            if (this != &other) {
                x = other.x;
                y = other.y;
                width = other.width;
                height = other.height;
            }
            return *this;
        }

        float x;        // Rectangle top-left corner position x
        float y;        // Rectangle top-left corner position y
        float width;    // Rectangle width
        float height;   // Rectangle height
    };

} // namespace Polybee

namespace pb = Polybee;

#endif // UTILS_H
