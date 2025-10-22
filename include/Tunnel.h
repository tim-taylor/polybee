/**
 * @file
 *
 * Declaration of the Tunnel class
 */

#ifndef _TUNNEL_H
#define _TUNNEL_H

#include "Params.h"

class Environment;

/**
 * The Tunnel class ...
 */
class Tunnel {

public:
    Tunnel();
    ~Tunnel() {}

    void initialise(float x, float y, float width, float height, Environment* pEnv);
    void addEntrance(const TunnelEntranceSpec& spec);

    /// @brief Check if a line segment passes through any tunnel entrance
    /// @param x1 x position of point 1 in environment coordinates
    /// @param y1 y position of point 1 in environment coordinates
    /// @param x2 x position of point 2 in environment coordinates
    /// @param y2 y position of point 2 in environment coordinates
    /// @return true if the line segment intersects any tunnel entrance, false otherwise
    bool passesThroughEntrance(float x1, float y1, float x2, float y2) const;

    float x() const { return m_x; }
    float y() const { return m_y; }
    float width() const { return m_width; }
    float height() const { return m_height; }
    float thickness() const { return m_thickness; }
    const std::vector<TunnelEntranceSpec>& getEntrances() const { return m_entrances; }

private:
    bool intersectRayRectangle(
        const pb::Vector2 ray_origin,
        const pb::Vector2 ray_dir,
        const pb::Rectangle targetRect,
        pb::Vector2* pContactPoint = nullptr,
        pb::Vector2* pContactNormal = nullptr,
        float* pNearContactTime = nullptr,
        std::vector<pb::Vector2>* pProbableContactPoints = nullptr) const;

    float m_x;                  // top-left x position of tunnel in environment coordinates
    float m_y;                  // top-left y position of tunnel in environment coordinates
    float m_width;              // width of the tunnel
    float m_height;             // height of the tunnel
    float m_thickness {0.5f};   // thickness of the tunnel walls
    std::vector<TunnelEntranceSpec> m_entrances;
    Environment* m_pEnv {nullptr};
};

#endif /* _TUNNEL_H */
