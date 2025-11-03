/**
 * @file
 *
 * Implementation of the Tunnel class
 */

#include "Tunnel.h"
#include "Environment.h"
#include "utils.h"
#include <cassert>
#include <format>
#include <cmath>

TunnelEntranceInfo::TunnelEntranceInfo(const TunnelEntranceSpec& spec, const Tunnel* pTunnel) :
    side(spec.side)
{
    // calculate entrance positions in environment coordinates based on tunnel position, tunnel size, and
    // relative entrance positions specified in spec

    switch (spec.side) {
    case 0: { // North
        x1 = pTunnel->x() + spec.e1;
        y1 = pTunnel->y();
        x2 = pTunnel->x() + spec.e2;
        y2 = pTunnel->y();
        break;
    }
    case 1: { // East
        x1 = pTunnel->x() + pTunnel->width();
        y1 = pTunnel->y() + spec.e1;
        x2 = pTunnel->x() + pTunnel->width();
        y2 = pTunnel->y() + spec.e2;
        break;
    }
    case 2: { // South
        x1 = pTunnel->x() + spec.e1;
        y1 = pTunnel->y() + pTunnel->height();
        x2 = pTunnel->x() + spec.e2;
        y2 = pTunnel->y() + pTunnel->height();
        break;
    }
    case 3: {  // West
        x1 = pTunnel->x();
        y1 = pTunnel->y() + spec.e1;
        x2 = pTunnel->x();
        y2 = pTunnel->y() + spec.e2;
        break;
    }
    default: {
        pb::msg_error_and_exit(
            std::format("Invalid tunnel entrance side {} specified. Must be 0=North, 1=East, 2=South, or 3=West.",
                spec.side));
    }
    }
}


Tunnel::Tunnel() {
}


void Tunnel::initialise(float x, float y, float width, float height, Environment* pEnv) {
    m_x = x;
    m_y = y;
    m_width = width;
    m_height = height;
    m_pEnv = pEnv;

    // initialize tunnel boundaries
    m_boundaries.clear();
    m_boundaries.push_back(pb::Line2D(pb::Pos2D(m_x, m_y), pb::Pos2D(m_x + m_width, m_y))); // top wall
    m_boundaries.push_back(pb::Line2D(pb::Pos2D(m_x + m_width, m_y), pb::Pos2D(m_x + m_width, m_y + m_height))); // right wall
    m_boundaries.push_back(pb::Line2D(pb::Pos2D(m_x + m_width, m_y + m_height), pb::Pos2D(m_x, m_y + m_height))); // bottom wall
    m_boundaries.push_back(pb::Line2D(pb::Pos2D(m_x, m_y + m_height), pb::Pos2D(m_x, m_y))); // left wall

    // add entrances from Params
    auto numEntrances = Params::tunnelEntranceSpecs.size();
    for (int i = 0; i < numEntrances; ++i) {
        const TunnelEntranceSpec& spec = Params::tunnelEntranceSpecs[i];
        addEntrance(spec);
    }
}


void Tunnel::addEntrance(const TunnelEntranceSpec& spec) {
    TunnelEntranceSpec specCopy = spec;
    // ensure e1 <= e2
    if (specCopy.e2 < specCopy.e1) {
        float tmp = specCopy.e1;
        specCopy.e1 = specCopy.e2;
        specCopy.e2 = tmp;
    }
    m_entrances.push_back({specCopy, this});
}


IntersectInfo Tunnel::intersectsEntrance(float x1, float y1, float x2, float y2) const
{
    assert(m_pEnv != nullptr);

    bool pt1InTunnel = m_pEnv->inTunnel(x1, y1);
    bool pt2InTunnel = m_pEnv->inTunnel(x2, y2);

    // if both points are in the same region (both in tunnel or both outside), no crossing
    //assert (pt1InTunnel != pt2InTunnel);
    if (pt1InTunnel == pt2InTunnel) {
        return {false, false};
    }

    // if we reach this point, the line segment crosses the tunnel boundary somewhere, so
    // we need to check if it crosses at any of the entrances
    pb::Line2D line1(pb::Pos2D(x1, y1), pb::Pos2D(x2, y2));
    for (const TunnelEntranceInfo& entrance : m_entrances)
    {
        pb::Line2D line2(pb::Pos2D{entrance.x1, entrance.y1}, pb::Pos2D{entrance.x2, entrance.y2});

        auto intersectInfo = getLineIntersection(line1, line2);

        if (intersectInfo.intersects && intersectInfo.withinLimits) {
            return intersectInfo;
        }
    }

    // we've checked all entrances and found no intersections
    // Now we check for intersections with the tunnel boundaries themselves, so we can
    // provide an intersection point even if the bee didn't cross at an entrance
    for (const auto& wall : m_boundaries) {
        auto intersectInfo = getLineIntersection(line1, wall);
        if (intersectInfo.withinLimits) {
            return {true, false, intersectInfo.point};
        }
    }
    pb::msg_error_and_exit("Tunnel::intersectsEntrance(): logic error: expected to find an intersection with tunnel walls when crossing boundary outside entrances.");
    return {false, false}; // to satisfy compiler
}


// Returns an IntersectInfo struct indicating whether the two lines intersect,
// whether the intersection is within both line segments, and the intersection point
IntersectInfo Tunnel::getLineIntersection(const pb::Line2D& line1, const pb::Line2D& line2) const
{
    double x1 = line1.start.x, y1 = line1.start.y;
    double x2 = line1.end.x, y2 = line1.end.y;
    double x3 = line2.start.x, y3 = line2.start.y;
    double x4 = line2.end.x, y4 = line2.end.y;

    // Calculate the denominator
    float denominator = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);

    // If denominator is 0, lines are parallel or coincident
    if (std::abs(denominator) < 1e-10) {
        return {false, false};
    }

    // Calculate the parameters t and u
    // t is the parameter for line1, u is the parameter for line2
    // Each of these parameters represent how far along the line segment the intersection occurs
    // If t is 0, the intersection is at the start of line1; if t is 1, it's at the end
    // If u is 0, the intersection is at the start of line2; if u is 1, it's at the end
    // If either t or u is outside the range [0, 1], the intersection does not occur within the line segments
    float t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denominator;
    float u = ((x1 - x3) * (y1 - y2) - (y1 - y3) * (x1 - x2)) / denominator;

    // Calculate intersection point
    pb::Pos2D intersection;
    intersection.x = x1 + t * (x2 - x1);
    intersection.y = y1 + t * (y2 - y1);

    // Check if intersection point is within both line segments
    if (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f) {
        return {true, true, intersection};
    }
    else {
        return {true, false, intersection};
    }
}

