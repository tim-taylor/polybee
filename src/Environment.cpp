/**
 * @file
 *
 * Implementation of the Environment class
 */

#include "Environment.h"
#include "Bee.h"
#include "PolyBeeCore.h"
#include "Params.h"
#include "utils.h"
#include <format>
#include <cassert>
#include <algorithm>


Environment::Environment() {
}


void Environment::initialise() {
    m_width = Params::envW;
    m_height = Params::envH;
    initialiseTunnel();
    initialisePlants();
    initialiseHives();
    initialiseBees();
    initialiseHeatmap();
}


void Environment::initialiseTunnel()
{
    // initialise tunnel from Params (also adds entrances)
    m_tunnel.initialise(Params::tunnelX, Params::tunnelY, Params::tunnelW, Params::tunnelH, this);
}


void Environment::initialisePlants()
{
    // Calculate total number of plants across all patches
    int totalPlants = 0;
    for (const PatchSpec& spec : Params::patchSpecs) {
        totalPlants += spec.numX * spec.numY * spec.numRepeats;
    }

    // Reserve space for all plants to prevent reallocation and pointer invalidation
    m_allPlants.reserve(totalPlants);

    // initialise plant grid (now stores pointers instead of Plant objects)
    m_plantGrid.resize(static_cast<size_t>(m_width), std::vector<std::vector<Plant*>>(static_cast<size_t>(m_height)));

    // initialise plant patches from Params
    for (const PatchSpec& spec : Params::patchSpecs)
    {
        float first_patch_topleft_plant_x = spec.x + ((spec.w - ((spec.numX - 1) * spec.spacing)) / 2.0f);
        float first_patch_topleft_plant_y = spec.y + ((spec.h - ((spec.numY - 1) * spec.spacing)) / 2.0f);

        for (int i = 0; i < spec.numRepeats; ++i) {
            float this_patch_topleft_plant_x = first_patch_topleft_plant_x + (i * spec.dx);
            float this_patch_topleft_plant_y = first_patch_topleft_plant_y + (i * spec.dy);

            float x = this_patch_topleft_plant_x;
            for (int a = 0; a < spec.numX; ++a) {
                float y = this_patch_topleft_plant_y;
                for (int b = 0; b < spec.numY; ++b) {
                    // TODO add jitter if needed
                    // create a plant at x,y and add to m_allPlants
                    m_allPlants.emplace_back(x, y, spec.speciesID);

                    // add pointer to this plant in the spatial grid
                    auto [i,j] = envPosToGridIndex(x, y);
                    m_plantGrid[i][j].push_back(&m_allPlants.back());

                    y += spec.spacing;
                }
                x += spec.spacing;
            }
        }

        // TODO - work in progress
        // We designate that 1 unit of length in environment coordinates
        // corresponds to 1m in real-world measurement.
        //
        // When we generate plants according to the patch spec, we store them in
        // a 2D array where each element corresponds to an area of ground in the environment.
        // That way, when bees move around we can efficiently check whether they are over
        // any plants by checking only the plants in the array element(s) they are currently over,
        // and its immediate neighbours.
    }
}


// Convert environment position (x,y) to grid index for m_plantGrid
pb::Pos2D Environment::envPosToGridIndex(float x, float y) const {
    // for debugging purposes, check for out-of-bounds access
    if (x < 0.0f || x > m_width + FLOAT_COMPARISON_EPSILON || y < 0.0f || y > m_height + FLOAT_COMPARISON_EPSILON) {
        pb::msg_error_and_exit(std::format("Environment::envPosToGridIndex: position ({},{}) is out of bounds (env size {}x{})",
            x, y, m_width, m_height));
    }

    //assert(x >= 0.0f && x < m_width + FLOAT_COMPARISON_EPSILON);
    //assert(y >= 0.0f && y < m_height + FLOAT_COMPARISON_EPSILON);

    int i = std::clamp(static_cast<int>(x), 0, static_cast<int>(m_width)-1);
    int j = std::clamp(static_cast<int>(y), 0, static_cast<int>(m_height)-1);

    return pb::Pos2D(i, j);
}


void Environment::initialiseHives()
{
    auto numHives = Params::hiveSpecs.size();
    for (int i = 0; i < numHives; ++i) {
        const HiveSpec& spec = Params::hiveSpecs[i];
        m_hives.emplace_back(spec.x, spec.y, spec.direction);
    }
}


// a private helper method to create the bees at the start of the simulation
void Environment::initialiseBees()
{
    auto numHives = Params::hiveSpecs.size();
    int numBeesPerHive = Params::numBees / numHives;
    for (int i = 0; i < numHives; ++i) {
        const HiveSpec& hive = Params::hiveSpecs[i];
        float angle = 0.0f;
        switch (hive.direction) {
        case 0: angle = -std::numbers::pi_v<float> / 2.0f; break; // North
        case 1: angle = 0.0f; break; // East
        case 2: angle = std::numbers::pi_v<float> / 2.0f; break; // South
        case 3: angle = std::numbers::pi_v<float>; break; // West
        case 4: angle = 0.0f; break; // Random (will be set per bee below)
        default:
            pb::msg_error_and_exit(std::format("Invalid hive direction {} specified for hive at ({},{}). Must be 0=North, 1=East, 2=South, 3=West, or 4=Random.",
                hive.direction, hive.x, hive.y));
        }

        for (int j = 0; j < numBeesPerHive; ++j) {
            float beeAngle = angle;
            if (hive.direction == 4) {
                // Random direction: uniform random angle between 0 and 2π
                beeAngle = PolyBeeCore::m_sAngle2PiDistrib(PolyBeeCore::m_sRngEngine);
            }
            m_bees.emplace_back(pb::Pos2D(hive.x, hive.y), beeAngle, this);
        }
    }

    if (numBeesPerHive * numHives < Params::numBees) {
        pb::msg_warning(std::format("Number of bees ({0}) is not a multiple of number of hives ({1}). Created {2} bees instead of the requested {0}.",
            Params::numBees, numHives, numBeesPerHive * numHives));
    }
}


void Environment::initialiseHeatmap() {
    m_heatmap.initialise(&m_bees);
    pb::msg_info(std::format("Initial EMD between uniform target and anti-target heatmaps: {:.6f}\n", m_heatmap.high_emd()));
}


void Environment::reset() {
    // TODO - work in progress
    resetBees();
    m_heatmap.reset();
}


void Environment::resetBees() {
    m_bees.clear();
    initialiseBees();
}

void Environment::update() {
    // update bee positions
    for (Bee& bee : m_bees) {
        bee.move();
    }

    m_heatmap.update();
}


bool Environment::inTunnel(float x, float y) const {
    return (x >= m_tunnel.x() &&
            x <= m_tunnel.x() + m_tunnel.width() &&
            y >= m_tunnel.y() &&
            y <= m_tunnel.y() + m_tunnel.height());
}


std::optional<Plant*> Environment::getNearestUnvisitedPlant(float x, float y, const std::vector<Plant*>& visited) const
{
    // TODO - should also check whether the tunnel wall is between the bee and the plant

    Plant* pNearestPlant = nullptr;
    float nearestDistSq = std::numeric_limits<float>::max();
    float rangeSq = Bee::visualRange() * Bee::visualRange();

    auto nearbyPlants = getNearbyPlants(x, y);

    for (Plant* pPlant : nearbyPlants) {
        if (std::find(visited.begin(), visited.end(), pPlant) != visited.end()) {
            continue;  // Skip already visited plants
        }

        float distSq = pb::distanceSq(x, y, pPlant->x(), pPlant->y());
        if (distSq < nearestDistSq && distSq <= rangeSq && distSq > 0.001f) {
            nearestDistSq = distSq;
            pNearestPlant = pPlant;
        }
    }

    if (pNearestPlant == nullptr) {
        return std::nullopt;
    }
    else {
        return pNearestPlant;
    }
}


std::vector<Plant*> Environment::getNearbyPlants(float x, float y) const
{
    // TODO - work in progress
    std::vector<Plant*> nearbyPlants;

    auto [i, j] = envPosToGridIndex(x, y);

    for (int di = -1; di <= 1; ++di) {
        for (int dj = -1; dj <= 1; ++dj) {
            if (i + di >= 0 && i + di < m_plantGrid.size() &&
                j + dj >= 0 && j + dj < m_plantGrid[i + di].size()) {
                const auto& cell = m_plantGrid[i + di][j + dj];
                nearbyPlants.insert(nearbyPlants.end(), cell.begin(), cell.end());
            }
        }
    }

    return nearbyPlants;
}


