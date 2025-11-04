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


// Initialise plants from Params::patchSpecs. Also builds spatial index grid for plant locations
// used for efficient lookup of nearby plants.
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
    m_plantGridCellSize = Params::beeVisualRange; // size of each cell in spatial index grid
    m_plantGridW = static_cast<size_t>(std::ceil(m_width / m_plantGridCellSize));
    m_plantGridH = static_cast<size_t>(std::ceil(m_height / m_plantGridCellSize));
    m_plantGrid.resize(m_plantGridW, std::vector<std::vector<Plant*>>(m_plantGridH));

    // initialise plant patches from Params
    for (const PatchSpec& spec : Params::patchSpecs)
    {
        std::normal_distribution<float> distJitter(0.0f, spec.jitter);

        float first_patch_topleft_plant_x = spec.x + ((spec.w - ((spec.numX - 1) * spec.spacing)) / 2.0f);
        float first_patch_topleft_plant_y = spec.y + ((spec.h - ((spec.numY - 1) * spec.spacing)) / 2.0f);

        for (int i = 0; i < spec.numRepeats; ++i) {
            float this_patch_topleft_plant_x = first_patch_topleft_plant_x + (i * spec.dx);
            float this_patch_topleft_plant_y = first_patch_topleft_plant_y + (i * spec.dy);

            float x = this_patch_topleft_plant_x;
            for (int a = 0; a < spec.numX; ++a) {
                float y = this_patch_topleft_plant_y;
                for (int b = 0; b < spec.numY; ++b) {
                    // add jitter to the individual plant position
                    float plantX = x + distJitter(PolyBeeCore::m_sRngEngine);
                    float plantY = y + distJitter(PolyBeeCore::m_sRngEngine);

                    // create a plant at x,y and add to m_allPlants
                    m_allPlants.emplace_back(plantX, plantY, spec.speciesID);

                    // add pointer to this plant in the spatial grid
                    auto [i,j] = envPosToGridIndex(plantX, plantY);
                    m_plantGrid[i][j].push_back(&m_allPlants.back());

                    y += spec.spacing;
                }
                x += spec.spacing;
            }
        }
    }
}


// Convert environment position (x,y) to grid index for m_plantGrid
pb::Pos2D Environment::envPosToGridIndex(float x, float y) const
{
    //assert(x >= 0.0f && x < m_width + FLOAT_COMPARISON_EPSILON);
    //assert(y >= 0.0f && y < m_height + FLOAT_COMPARISON_EPSILON);

    float gridx = x / m_plantGridCellSize;
    float gridy = y / m_plantGridCellSize;

    int i = std::clamp(static_cast<int>(gridx), 0, static_cast<int>(m_plantGridW)-1);
    int j = std::clamp(static_cast<int>(gridy), 0, static_cast<int>(m_plantGridH)-1);

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


// Reset the environment to its initial state suitable for a new simulation run
void Environment::reset() {
    // TODO - as the class is developed, ensure all relevant state is reset here
    resetBees();
    resetPlants();
    m_heatmap.reset();
}


void Environment::resetPlants() {
    m_plantGrid.clear();
    m_allPlants.clear();
    initialisePlants();
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


// Find the nearest unvisited plant to the given position (x,y).
// Returns std::nullopt if no unvisited plant is found within visual range.
// The 'visited' parameter is a list of plants that have recently been visited by the bee.
// This method considers only plants within visual range, and selects either
// the nearest plant or a random other visible plant based on Params::beeProbVisitNearestFlower.
std::optional<Plant*> Environment::getNearestUnvisitedPlant(float x, float y, const std::vector<Plant*>& visited) const
{
    // TODO - should also check whether the tunnel wall is between the bee and the plant

    Plant* pNearestPlant = nullptr;

    std::vector<Plant*> visiblePlants;

    float nearestDistSq = std::numeric_limits<float>::max();
    float rangeSq = Bee::visualRange() * Bee::visualRange();

    auto nearbyPlants = getNearbyPlants(x, y);
    for (Plant* pPlant : nearbyPlants) {
        if (std::find(visited.begin(), visited.end(), pPlant) != visited.end()) {
            continue;  // Skip already visited plants
        }

        float distSq = pb::distanceSq(x, y, pPlant->x(), pPlant->y()); // squared distance to plant

        if (distSq <= rangeSq) {
            // Plant is within visual range
            visiblePlants.push_back(pPlant);
            if (distSq < nearestDistSq) {
                nearestDistSq = distSq;
                pNearestPlant = pPlant;
            }
        }
    }

    if (pNearestPlant == nullptr) {
        // No unvisited plants found in local area
        return std::nullopt;
    }
    else {
        assert(!visiblePlants.empty());
        if (visiblePlants.size() == 1) {
            // only one visible plant found, so no further considerations required
            return pNearestPlant;
        }
        else {
            // more than one visible plant - decide whether to return the nearest plant
            // or a random other plant based on probability
            float p = PolyBeeCore::m_sUniformProbDistrib(PolyBeeCore::m_sRngEngine);
            if (p < Params::beeProbVisitNearestFlower) {
                // return the nearest plant
                return pNearestPlant;
            }
            else {
                // return a random other plant for the list of visible plants

                // first find the nearest plant in the visiblePlants list and remove it
                for (auto it = visiblePlants.begin(); it != visiblePlants.end(); ++it) {
                    if (*it == pNearestPlant) {
                        it = visiblePlants.erase(it);
                        break;
                    }
                }

                // now pick a random plant for the remaining visible plants
                return visiblePlants[
                    PolyBeeCore::m_sUniformIntDistrib(PolyBeeCore::m_sRngEngine) % visiblePlants.size()];
            }
        }
    }
}


// Return a flat vector of all plants in the local 3x3 grid cells around the given position
std::vector<Plant*> Environment::getNearbyPlants(float x, float y) const
{
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


