/**
 * @file
 *
 * Declaration of the Tunnel class
 */

#ifndef _TUNNEL_H
#define _TUNNEL_H

#include "Params.h"
#include "utils.h"
#include <format>
#include <vector>
#include <optional>
#include <tuple>

class Environment;
class Tunnel;


struct TunnelEntranceInfo {
    float x1;   // x position of first edge of entrance (in environment coordinates)
    float y1;   // y position of first edge of entrance (in environment coordinates)
    float x2;   // x position of second edge of entrance (in environment coordinates)
    float y2;   // y position of second edge of entrance (in environment coordinates)
    int side;   // 0=North, 1=East, 2=South, 3=West
    NetType netType { NetType::NONE };  // type of net at this entrance

    /*
    TunnelEntranceInfo(float x1, float y1, float x2, float y2, int side)
        : x1(x1), y1(y1), x2(x2), y2(y2), side(side) {}

    TunnelEntranceInfo() : x1(0), y1(0), x2(0), y2(0), side(0) {}

    TunnelEntranceInfo(const TunnelEntranceInfo& other)
        : x1(other.x1), y1(other.y1), x2(other.x2), y2(other.y2), side(other.side) {}
    */

    TunnelEntranceInfo(const TunnelEntranceSpec& spec, const Tunnel* pTunnel);

    // probability of a bee exiting the tunnel when reaching this entrance
    float probExit() const {
        switch (netType) {
        case NetType::NONE:
            return 1.0f;
        case NetType::ANTIBIRD:
            return Params::netAntibirdExitProb;
        case NetType::ANTIHAIL:
            return Params::netAntihailExitProb;
        default: {
            pb::msg_error_and_exit(std::format("Unknown net type {} encountered in TunnelEntranceInfo::probExit()", static_cast<int>(netType)));
            return 0.0f;
        }
        }
    }

    // maximum number of attempts to exit via this entrance before a bee gives up
    int maxAttempts() const {
        switch (netType) {
        case NetType::NONE:
            return 1000; // effectively unlimited
        case NetType::ANTIBIRD:
            return Params::netAntibirdMaxExitAttempts;
        case NetType::ANTIHAIL:
            return Params::netAntihailMaxExitAttempts;
        default: {
            pb::msg_error_and_exit(std::format("Unknown net type {} encountered in TunnelEntranceInfo::maxAttempts()", static_cast<int>(netType)));
            return 0;
        }
        }
    }
};


struct IntersectInfo {
    bool intersects { false };                          // do the lines intersect at all
    bool crossesEntrance { false };                     // is the intersection point within the entrance limits (if applicable)
    pb::Pos2D point;                                    // intersection point (if intersects is true)
    pb::Line2D intersectedLine;                         // the line that was intersected (if intersects is true)
                                                        // (this will be either a tunnel wall or an entrance line,
                                                        // depending on whether crossesEntrance is true)
    const TunnelEntranceInfo* pEntranceUsed { nullptr };// pointer to the entrance that was used (if applicable)

    IntersectInfo() : intersects(false), crossesEntrance(false), point(), intersectedLine() {}

    IntersectInfo(bool intersects, bool crossesEntrance, const pb::Pos2D& point, const pb::Line2D& intersectedLine)
        : intersects(intersects), crossesEntrance(crossesEntrance), point(point), intersectedLine(intersectedLine) {}

    IntersectInfo(bool intersects, bool crossesEntrance)
        : intersects(intersects), crossesEntrance(crossesEntrance) {}

    void reset() {
        intersects = false;
        crossesEntrance = false;
        point.setToZero();
        intersectedLine = pb::Line2D();
        pEntranceUsed = nullptr;
    }
};


/**
 * The Tunnel class ...
 */
class Tunnel {

public:
    Tunnel();
    ~Tunnel() {}

    // initiailise the tunnel and entrances
    void initialise(float x, float y, float width, float height, Environment* pEnv);

    // initialise entrances only (for resetting between simulation runs)
    void initialiseEntrances();
    void initialiseEntrances(const std::vector<TunnelEntranceSpec>& specs);

    /// @brief Check if the line from point (x1, y1) to point (x2, y2) intersects any tunnel entrance
    /// @param x1 x position of point 1 in environment coordinates
    /// @param y1 y position of point 1 in environment coordinates
    /// @param x2 x position of point 2 in environment coordinates
    /// @param y2 y position of point 2 in environment coordinates
    /// @return information about the intersection
    IntersectInfo intersectsTunnelBoundary(float x1, float y1, float x2, float y2) const;

    float x() const { return m_x; }
    float y() const { return m_y; }
    float width() const { return m_width; }
    float height() const { return m_height; }
    const std::vector<TunnelEntranceInfo>& getEntrances() const { return m_entrances; }

private:
    void addEntrance(const TunnelEntranceSpec& spec);
    IntersectInfo getLineIntersection(const pb::Line2D& line1, const pb::Line2D& line2) const;

    float m_x;                  // top-left x position of tunnel in environment coordinates
    float m_y;                  // top-left y position of tunnel in environment coordinates
    float m_width;              // width of the tunnel
    float m_height;             // height of the tunnel
    std::vector<pb::Line2D> m_boundaries;
    std::vector<TunnelEntranceInfo> m_entrances;
    Environment* m_pEnv {nullptr};
};

#endif /* _TUNNEL_H */
