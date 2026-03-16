#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

class TunnelEntranceInfo; // forward declaration of TunnelEntranceInfo class


constexpr float FLOAT_COMPARISON_EPSILON = 0.000001f; // small value to use when comparing floats for equality

namespace Polybee {

    // first some forward declarations

    struct Line2D;
    struct IntersectInfo;

    // message/logging functions

    [[noreturn]] void msg_error_and_exit(const std::string& msg);
    void msg_warning(const std::string& msg);
    void msg_info(const std::string& msg);

    // stats and mathematical functions

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

    // geometry-related structs and functions

    struct Pos2D {
        float x, y;

        Pos2D(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}

        Pos2D operator+(const Pos2D& other) const { return Pos2D(x + other.x, y + other.y); }
        Pos2D operator*(float scalar) const { return Pos2D(x * scalar, y * scalar); }

        float length() const;
        void resize(float newLength);
        void set(float newX, float newY) { x = newX; y = newY; }
        void setToZero() { x = 0.0f; y = 0.0f; }
        float angle() const { return std::atan2(y, x); } // angle in radians from positive x-axis to the vector (x, y)
        Pos2D normalized() const;

        // return a new Pos2D that is the result of moving this position along the specified line by the specified distance
        // if clampToLineEnds is true, the movement will be clamped to the start and end points of the line, so the returned
        //position will never be beyond either end of the line segment
        Pos2D moveAlongLine(const Line2D& line, float distance, bool clampToLineEnds = false) const;
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
        Line2D(const Pos2D& start, const Pos2D& end) : start(start), end(end) {}

        Pos2D unitVector() const;                   // returns a unit vector parallel to the line, pointing in the direction from start to end
        Pos2D normalUnitVector() const;             // returns a unit vector perpendicular to the line, pointing to the left of the direction from start to end
        float length() const;                       // Calculate length of the line segment
        float distance(const Pos2D& point) const;   // Calculate perpendicular distance from point to (infinite projection of) line
        IntersectInfo getIntersectInfo(const Line2D& line) const; // Returns an IntersectInfo struct indicating whether the given line intersect with this one

    };

    struct IntersectInfo {
        bool intersects { false };                          // do the line projections intersect at all (i.e. not parallel)
        bool withinBounds { false };                        // is the intersection point within the limits of both lines (if applicable)
        bool enteringTunnel { false };                      // is the bee trying to enter the tunnel (as opposed to exit) (if applicable)
        Pos2D point;                                        // intersection point (if intersects is true)
        Line2D intersectedLine;                             // the line that was intersected (if intersects is true)
                                                            // (this will be either a tunnel wall or an entrance line,
                                                            // depending on whether withinBounds is true)
        const TunnelEntranceInfo* pEntranceUsed { nullptr };// pointer to the entrance that was used (if applicable)

        IntersectInfo() : intersects(false), withinBounds(false), enteringTunnel(false), point(), intersectedLine() {}

        IntersectInfo(bool intersects, bool withinBounds, const Pos2D& point, const Line2D& intersectedLine)
            : intersects(intersects), withinBounds(withinBounds), enteringTunnel(false), point(point), intersectedLine(intersectedLine) {}

        IntersectInfo(bool intersects, bool withinBounds)
            : intersects(intersects), withinBounds(withinBounds) {}

        void reset() {
            intersects = false;
            withinBounds = false;
            enteringTunnel = false;
            point.setToZero();
            intersectedLine = Line2D();
            pEntranceUsed = nullptr;
        }
    };

} // namespace Polybee

namespace pb = Polybee;

#endif // UTILS_H
