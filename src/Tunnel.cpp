/**
 * @file
 *
 * Implementation of the Tunnel class
 */

#include "Tunnel.h"
#include "Environment.h"
#include "utils.h"
#include <cassert>

Tunnel::Tunnel() {
}

void Tunnel::initialise(float x, float y, float width, float height, Environment* pEnv) {
    m_x = x;
    m_y = y;
    m_width = width;
    m_height = height;
    m_pEnv = pEnv;

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
    m_entrances.push_back(specCopy);
}


bool Tunnel::passesThroughEntrance(float x1, float y1, float x2, float y2) const {
    assert(m_pEnv != nullptr);

    bool pt1InTunnel = m_pEnv->inTunnel(x1, y1);
    bool pt2InTunnel = m_pEnv->inTunnel(x2, y2);

    // if both points are in the same region (both in tunnel or both outside), no crossing
    if (pt1InTunnel == pt2InTunnel) {
        return false;
    }

    // if we reach this point, the line segment crosses the tunnel
    for (const TunnelEntranceSpec& entrance : m_entrances) {
        // TODO: implement proper intersection test
        // use code from https://gist.github.com/Pikachuxxxx/0dda4b70bf71b794b08923df34961844
        // RayCollisions.c
        // See below for implementation details - TODO define pb::pb::Vector2 and pb::pb::Rectangle in utils.h
        /*
        if (entrance.intersects(x1, y1, x2, y2)) {
            return true;
        }
        */
    }

    return false;
}


/*
    DESCRIPTION : A function to detect Collisions between a Ray and a Rectangle.
    - Also provides additonal data such as the contact point and contact normal and the probable collision points
    - PARAMETER ray_origin : A pb::Vector2 defining the origin/start point of the Ray
    - PARAMETER ray_dir : A pb::Vector2 defining the direction of the Ray
    - PARAMETER targetRect : The target pb::Rectangle against which we are checking the collision
    - PARAMETER pContactPoint (Pointer) : A pb::Vector2 that stores the contact point if the collision occurs with the rectangle
    - PARAMETER pContactNormal (Pointer) : A pb::Vector2 that stores the contact normal of the rectangle surface if the collision occurs with the rectangle
    - PARAMETER pNearContactTime (Pointer) : A float that stores the parametric contact time of the rectangle surface if the collision occurs with the rectangle
    - PARAMETER pProbableContactPoints (Pointer): A vector of pb::Vector2's that stores the probale near and far contact points with the ray and rectanlge if the there is a chance of collision
    RETURNS : A bool indicating wether the collision has occured or not.
    - Also updates the pContactPoint, pContactNormal and the pProbableContactPoints pointers

    This code is a slightly modified version of the code from the following source:
    Source: https://gist.github.com/Pikachuxxxx/0dda4b70bf71b794b08923df34961844
*/
bool Tunnel::intersectRayRectangle(
    const pb::Vector2 ray_origin,
    const pb::Vector2 ray_dir,
    const pb::Rectangle targetRect,
    pb::Vector2* pContactPoint,
    pb::Vector2* pContactNormal,
    float* pNearContactTime,
    std::vector<pb::Vector2>* pProbableContactPoints) const
{
    assert(pContactPoint != nullptr);
    assert(pContactNormal != nullptr);
    assert(pNearContactTime != nullptr);
    assert(pProbableContactPoints != nullptr);
    assert(pProbableContactPoints->empty());

    pProbableContactPoints->clear();

    /*
    * The t in the P(t) = P + D.t
    * Where t is the parametric variable to plot the near collison point using the parametric line equation (P(t) = P + D.t)
    * Where P is the Position Vector of the Ray and D is the Direciton Vector of the Ray
    */
    float t_hit_near;

    /*
    * Calculate intersection points with rectangle bounding axes
    * Parametric 't' for Near (X,Y) and Far (X,Y)
    */
    float delta_t1_X = targetRect.x - ray_origin.x;
    float t_hit_near_X = (delta_t1_X / ray_dir.x);

    float delta_t1_Y = targetRect.y - ray_origin.y;
    float t_hit_near_Y = (delta_t1_Y / ray_dir.y);

    float delta_t2_X = targetRect.x + targetRect.width - ray_origin.x;
    float t_hit_far_X = (delta_t2_X / ray_dir.x);

    float delta_t2_Y = targetRect.y + targetRect.height - ray_origin.y;
    float t_hit_far_Y = (delta_t2_Y / ray_dir.y);

    /*
    * Sort the distances to maintain Affine uniformity
    * i.e. sort the near and far axes of the rectangle in the appropritate order from the POV of ray_origin
    */
    if (t_hit_near_X > t_hit_far_X) std::swap(t_hit_near_X, t_hit_far_X);
    if (t_hit_near_Y > t_hit_far_Y) std::swap(t_hit_near_Y, t_hit_far_Y);

    // As there is no chance of collision i.e the paramteric line cannot pass throguh the rectangle the probable points are empty
    pProbableContactPoints->push_back({0, 0});
    pProbableContactPoints->push_back({0, 0});

    /*
    * Check the order of the near and far points
    * if they satisfy the below condition the line will pass through the rectanle (It didn't yet)
    * if not return out of the function as it will not pass through
    */
    if(!(t_hit_near_X < t_hit_far_Y && t_hit_near_Y < t_hit_far_X)) return false;

    /*
    * If the parametric line passes through the rectangle calculate the parametric 't'
    * the 't' should be such that it must lie on both the Line/Ray and the Rectangle
    * Use the condition below to calculate the 'tnear' and 'tfar' that gives the near and far collison parametric t
    */
    *pNearContactTime = std::max(t_hit_near_X, t_hit_near_Y);
    float t_hit_far = std::min(t_hit_far_X, t_hit_far_Y);

    // Use the parametric t values calculated above and subsitute them in the parametric equation to get the near and far contact points
    float Hit_Near_X_Position = ray_origin.x + (ray_dir.x * (*pNearContactTime));
    float Hit_Near_Y_Position = ray_origin.y + (ray_dir.y * (*pNearContactTime));

    float Hit_Far_X_Position = ray_origin.x + (ray_dir.x * t_hit_far);
    float Hit_Far_Y_Position = ray_origin.y + (ray_dir.y * t_hit_far);

    // Debugging the calculations
    #ifdef RAY_COLLISION_CALCULATION_DEBUG_STATS
        printf("The delta_t1_X is : %f | t_hit_near_X is : %f | ray_dir.x is : %f | rayOrigin.x is : %f | Hit_Near_X_Position is : %f \n", delta_t1_X, t_hit_near_X, ray_dir.x, ray_origin.x, Hit_Near_X_Position);
        printf("The delta_t1_Y is : %f | t_hit_near_Y is : %f | ray_dir.y is : %f | rayOrigin.y is : %f | Hit_Near_Y_Position is : %f \n", delta_t1_Y, t_hit_near_Y, ray_dir.y, ray_origin.y, Hit_Near_Y_Position);
        printf("t_hit_near is : %f \n", *pNearContactTime);
        printf("The delta_t2_X is : %f | t_hit_far_X is : %f | ray_dir.x is : %f | rayOrigin.x is : %f | Hit_Far_X_Position is : %f \n", delta_t2_X, t_hit_far_X, ray_dir.x, ray_origin.x, Hit_Far_X_Position);
        printf("The delta_t2_Y is : %f | t_hit_far_Y is : %f | ray_dir.y is : %f | rayOrigin.y is : %f | Hit_Far_Y_Position is : %f \n", delta_t2_Y, t_hit_far_Y, ray_dir.y, ray_origin.y, Hit_Far_Y_Position);
        printf("t_hit_far is : %f \n", t_hit_far);
    #endif

    // Generate Vectors using the near and far collision points
    pb::Vector2 Near_Hit_Vector = (pb::Vector2){Hit_Near_X_Position, Hit_Near_Y_Position};
    pb::Vector2 Far_Hit_Vector = (pb::Vector2){Hit_Far_X_Position, Hit_Far_Y_Position};
    // Since we are sure that the line will pass through the rectanle upadte the probable near and far points
    pProbableContactPoints->push_back({Near_Hit_Vector});
    pProbableContactPoints->push_back({Far_Hit_Vector});

    /*
    * Check wether the parametric 't' values are withing certain bounds to guarantee that the probable collision has actually occured
    * i.e. If the below conditions are true only then the Ray has passed through the Rectangle
    * if not it still can pass but it did not yet
    */
    if(t_hit_far < 0 || t_hit_near > 1) return false;

    // Now Update the actual contact point
    *pContactPoint = (pb::Vector2){Hit_Near_X_Position, Hit_Near_Y_Position};

    // Update the contact normal of the Ray with the Rectangle surface
    if(t_hit_near_X > t_hit_near_Y){
        if(ray_dir.x < 0) *pContactNormal = (pb::Vector2){1, 0};
        else *pContactNormal = (pb::Vector2){-1, 0};
    }
    else if(t_hit_near_X < t_hit_near_Y){
        if(ray_dir.y < 0) *pContactNormal = (pb::Vector2){0, 1};
        else *pContactNormal = (pb::Vector2){0, -1};
    }
    // Since the contact has definitely occured return true
    return true;
}
