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

    Pos2D Pos2D::moveAlongLine(const Line2D& line, float distance, bool clampToLineEnds) const {
        float lineLength = line.length();
        if (lineLength < FLOAT_COMPARISON_EPSILON) {
            // Line segment is a point, cannot move along it
            return *this;
        }

        // Get unit vector in direction of line
        float unitX = (line.end.x - line.start.x) / lineLength;
        float unitY = (line.end.y - line.start.y) / lineLength;

        // Calculate new position by moving along the line direction
        float newX = x + unitX * distance;
        float newY = y + unitY * distance;

        if (clampToLineEnds) {
            // Clamp the new position to the start and end points of the line segment
            float t = ((newX - line.start.x) * (line.end.x - line.start.x) + (newY - line.start.y) * (line.end.y - line.start.y)) / (lineLength * lineLength);
            t = std::max(0.0f, std::min(1.0f, t)); // clamp t to [0, 1]
            newX = line.start.x + t * (line.end.x - line.start.x);
            newY = line.start.y + t * (line.end.y - line.start.y);
        }

        return Pos2D(newX, newY);
    }

    float Line2D::length() const {
        float dx = end.x - start.x;
        float dy = end.y - start.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    Pos2D Line2D::unitVector() const {
        float dx = end.x - start.x;
        float dy = end.y - start.y;
        float length = std::sqrt(dx * dx + dy * dy);
        if (length < FLOAT_COMPARISON_EPSILON) {
            return Pos2D(0, 0); // zero-length line, return zero vector
        }
        return Pos2D(dx / length, dy / length);
    }

    Pos2D Line2D::normalUnitVector() const {
        // Get unit vector in direction of line
        Pos2D unit = unitVector();
        // Rotate 90 degrees to get normal vector (pointing to the left of the line direction)
        return Pos2D(-unit.y, unit.x);
    }

    float Line2D::distance(const Pos2D& point) const {
        // Calculate perpendicular distance from point to (infinite projection of) line
        float dx = end.x - start.x;
        float dy = end.y - start.y;
        float lengthSq = dx * dx + dy * dy;

        if (lengthSq < FLOAT_COMPARISON_EPSILON) {
            // Line segment is a point
            return std::sqrt((point.x - start.x) * (point.x - start.x) + (point.y - start.y) * (point.y - start.y));
        }

        // Project point onto line segment (or onto projection of line if outside segment)
        float t = ((point.x - start.x) * dx + (point.y - start.y) * dy) / lengthSq;

        float projX = start.x + t * dx;
        float projY = start.y + t * dy;

        return std::sqrt((point.x - projX) * (point.x - projX) + (point.y - projY) * (point.y - projY));
    }

} // namespace Polybee
