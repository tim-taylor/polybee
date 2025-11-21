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
        m_hives.emplace_back(spec.x, spec.y, spec.direction, this);
    }
}


// a private helper method to create the bees at the start of the simulation
void Environment::initialiseBees()
{
    if (m_hives.empty()) {
        pb::msg_error_and_exit("No hives have been defined in the environment, but bees are to be initialised. Cannot create bees without hives.");
    }

    int numHives = static_cast<int>(m_hives.size());
    int numBeesPerHive = Params::numBees / numHives;

    for (Hive& hive : m_hives) {
        for (int j = 0; j < numBeesPerHive; ++j) {
            m_bees.emplace_back(&hive, this);
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
        bee.update();
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
    // TODO - should also check whether the tunnel wall is between the bee and the plant?

    std::vector<NearbyPlantInfo> visiblePlants;

    float rangeSq = Bee::visualRange() * Bee::visualRange();

    auto nearbyPlants = getNearbyPlants(x, y);
    for (Plant* pPlant : nearbyPlants) {
        if (std::find(visited.begin(), visited.end(), pPlant) != visited.end()) {
            continue;  // Skip already visited plants
        }

        float distSq = pb::distanceSq(x, y, pPlant->x(), pPlant->y()); // squared distance to plant

        if (distSq <= rangeSq) {
            // Plant is within visual range
            visiblePlants.emplace_back(pPlant, std::sqrt(distSq));
        }
    }

    if (visiblePlants.empty()) {
        // No unvisited plants found in local area
        return std::nullopt;
    }
    else {
        if (visiblePlants.size() == 1) {
            // only one visible plant found, so no further considerations required
            return visiblePlants[0].pPlant;
        }
        else {
            return pickRandomPlantWeightedByDistance(visiblePlants);
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


// Select a plant randomly from the given list, with probability weighted by distance
// (closer plants have higher probability of being selected)
// Assumes that the plants vector is non-empty and that all plants in the vector are within visual range.
Plant* Environment::pickRandomPlantWeightedByDistance(const std::vector<NearbyPlantInfo>& plants) const
{
    assert(!plants.empty());

    // Calculate total weight
    // We assume that the maximum possible distance for a plant will be Params::beeVisualRange,
    // because the plants vector should only contain plants within visual range.
    // The weight assigned to a plant is then (maxPossibleDistance - distanceToPlant),
    // so that closer plants have higher weight.
    float maxPossibleDistance = Params::beeVisualRange * 1.1f; // slight buffer to avoid zero weight
    float totalWeight = 0.0f;
    for (const NearbyPlantInfo& info : plants) {
        totalWeight += std::max(0.0f, (maxPossibleDistance - info.dist));
    }

    // Generate a random value between 0 and totalWeight
    float randValue = PolyBeeCore::m_sUniformProbDistrib(PolyBeeCore::m_sRngEngine) * totalWeight;

    // Select a plant based on the random value
    float cumulativeWeight = 0.0f;
    for (const NearbyPlantInfo& info : plants) {
        cumulativeWeight += std::max(0.0f, (maxPossibleDistance - info.dist));
        if (randValue <= cumulativeWeight) {
            return info.pPlant;
        }
    }

    // Fallback to keep compiler happy (should not reach here)
    assert(false);
    return plants.back().pPlant;
}


