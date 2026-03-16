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
#include <fstream>
#include <sstream>


Environment::Environment() {
}


void Environment::initialise(PolyBeeCore* pCore)
{
    m_pPolyBeeCore = pCore;
    m_width = Params::envW;
    m_height = Params::envH;
    initialiseTunnel();
    initialiseBarriers();
    initialisePlants();
    if (!(Params::bEvolve && Params::evolveSpec.evolveHivePositions)) {
        initialiseHivesAndBees();
    }
    initialiseHeatmap();
    initialiseTargetHeatmap();
}


void Environment::initialiseTunnel()
{
    // initialise tunnel from Params (also adds entrances)
    m_tunnel.initialise(Params::tunnelX, Params::tunnelY, Params::tunnelW, Params::tunnelH, this);
}


void Environment::initialiseBarriers()
{
    // Calculate total number of barriers and max barrier length
    int totalBarriers = 0;
    float maxBarrierLength = 0.0f;

    for (const BarrierSpec& spec : Params::barrierSpecs) {
        totalBarriers += spec.numRepeatsX * spec.numRepeatsY;
        if (spec.length() > maxBarrierLength) {
            maxBarrierLength = spec.length();
        }
    }
    m_allBarriers.reserve(totalBarriers);

    // initialise barrier grid (NB this stores pointers instead of Barrier objects)
    // - first determine the size of each cell in the grid. We want this to be at least as large as
    //   the longest barrier, so that we know that a 3x3 group of cells centered on a bee's current position
    //   will contain all barriers that the bee might possibly come across in its next potential move.
    //   In the (unlikely) case that all barriers are shorter than the bee's visual range, we use the bee's visual range
    //   as the cell size, so that we know that the 3x3 group of cells will contain all barriers near plants that
    //   the bee might be trying to visit.
    m_barrierGridCellSize = std::max(maxBarrierLength, Params::beeVisualRange); // size of each cell in spatial index grid
    m_barrierGridW = static_cast<size_t>(std::ceil(m_width / m_barrierGridCellSize));
    m_barrierGridH = static_cast<size_t>(std::ceil(m_height / m_barrierGridCellSize));
    m_barrierGrid.resize(m_barrierGridW, std::vector<std::vector<Barrier*>>(m_barrierGridH));

    // initialise barriers from Params
    for (const BarrierSpec& spec : Params::barrierSpecs)
    {
        float startX = spec.x1;
        float startY = spec.y1;
        float endX = spec.x2;
        float endY = spec.y2;
        for (int i = 0; i < spec.numRepeatsX; ++i) {
            float thisStartX = startX + (i * spec.dx);
            float thisEndX = endX + (i * spec.dx);

            for (int j = 0; j < spec.numRepeatsY; ++j) {
                float thisStartY = startY + (j * spec.dy);
                float thisEndY = endY + (j * spec.dy);

                // create a barrier and add to m_allBarriers
                m_allBarriers.emplace_back(thisStartX, thisStartY, thisEndX, thisEndY);

                // add pointer to this barrier in the spatial grid (based on the midpoint of the barrier)
                auto [gridi, gridj] = envPosToBarrierGridIndex((thisStartX + thisEndX) / 2.0f, (thisStartY + thisEndY) / 2.0f);
                m_barrierGrid[gridi][gridj].push_back(&m_allBarriers.back());
            }
        }
    }
}


// Initialise plants from Params::patchSpecs. Also builds spatial index grid for plant locations
// used for efficient lookup of nearby plants.
void Environment::initialisePlants()
{
    // Calculate total number of plants across all patches
    int totalPlants = 0;
    for (const PatchSpec& spec : Params::patchSpecs) {
        totalPlants += spec.getNumX() * spec.getNumY() * spec.numRepeats;
    }

    // Reserve space for all plants to prevent reallocation and pointer invalidation
    m_allPlants.reserve(totalPlants);

    // initialise plant grid (NB this stores pointers instead of Plant objects)
    m_plantGridCellSize = Params::beeVisualRange; // size of each cell in spatial index grid
    m_plantGridW = static_cast<size_t>(std::ceil(m_width / m_plantGridCellSize));
    m_plantGridH = static_cast<size_t>(std::ceil(m_height / m_plantGridCellSize));
    m_plantGrid.resize(m_plantGridW, std::vector<std::vector<Plant*>>(m_plantGridH));

    // initialise plant patches from Params
    for (const PatchSpec& spec : Params::patchSpecs)
    {
        std::normal_distribution<float> distJitter(0.0f, spec.jitter);

        float first_patch_topleft_plant_x = spec.x + ((spec.w - ((spec.getNumX() - 1) * spec.spacing)) / 2.0f);
        float first_patch_topleft_plant_y = spec.y + ((spec.h - ((spec.getNumY() - 1) * spec.spacing)) / 2.0f);

        for (int i = 0; i < spec.numRepeats; ++i) {
            float this_patch_topleft_plant_x = first_patch_topleft_plant_x + (i * spec.dx);
            float this_patch_topleft_plant_y = first_patch_topleft_plant_y + (i * spec.dy);

            float x = this_patch_topleft_plant_x;
            for (int a = 0; a < spec.getNumX(); ++a) {
                float y = this_patch_topleft_plant_y;
                for (int b = 0; b < spec.getNumY(); ++b) {
                    // add jitter to the individual plant position
                    float plantX = x + distJitter(m_pPolyBeeCore->m_rngEngine);
                    float plantY = y + distJitter(m_pPolyBeeCore->m_rngEngine);

                    // create a plant at x,y and add to m_allPlants
                    m_allPlants.emplace_back(plantX, plantY, spec.speciesID);

                    // add pointer to this plant in the spatial grid
                    auto [i,j] = envPosToPlantGridIndex(plantX, plantY);
                    m_plantGrid[i][j].push_back(&m_allPlants.back());

                    y += spec.spacing;
                }
                x += spec.spacing;
            }
        }
    }
}


// Convert environment position (x,y) to grid index for m_plantGrid
pb::Pos2D Environment::envPosToPlantGridIndex(float x, float y) const
{
    //assert(x >= 0.0f && x < m_width + FLOAT_COMPARISON_EPSILON);
    //assert(y >= 0.0f && y < m_height + FLOAT_COMPARISON_EPSILON);

    float gridx = x / m_plantGridCellSize;
    float gridy = y / m_plantGridCellSize;

    int i = std::clamp(static_cast<int>(gridx), 0, static_cast<int>(m_plantGridW)-1);
    int j = std::clamp(static_cast<int>(gridy), 0, static_cast<int>(m_plantGridH)-1);

    return pb::Pos2D(i, j);
}


// Convert environment position (x,y) to grid index for m_barrierGrid
pb::Pos2D Environment::envPosToBarrierGridIndex(float x, float y) const
{
    float gridx = x / m_barrierGridCellSize;
    float gridy = y / m_barrierGridCellSize;

    int i = std::clamp(static_cast<int>(gridx), 0, static_cast<int>(m_barrierGridW)-1);
    int j = std::clamp(static_cast<int>(gridy), 0, static_cast<int>(m_barrierGridH)-1);

    return pb::Pos2D(i, j);
}


void Environment::initialiseHivesAndBees(const std::vector<HiveSpec>& hiveSpecs)
{
    m_hives.clear();
    auto numHives = hiveSpecs.size();
    for (int i = 0; i < numHives; ++i) {
        const HiveSpec& spec = hiveSpecs[i];
        m_hives.emplace_back(spec.x, spec.y, spec.direction, this);
    }
    initialiseBees();
}

void Environment::initialiseHivesAndBees()
{
    initialiseHivesAndBees(Params::hiveSpecs);
}


// a private helper method to create the bees at the start of the simulation
void Environment::initialiseBees()
{
    if (m_hives.empty()) {
        pb::msg_error_and_exit("No hives have been defined in the environment, but bees are to be initialised. Cannot create bees without hives.");
    }

    m_bees.clear();

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
    if (!(Params::bEvolve && Params::evolveObjective != EvolveObjective::EMD_TO_TARGET_HEATMAP)) {
        pb::msg_info(std::format("Initial EMD between uniform target and anti-target heatmaps: {:.6f}",
            m_heatmap.high_emd()));
    }
}


void Environment::initialiseTargetHeatmap()
{
    if (Params::strTargetHeatmapFilename.empty()) {
        // no target heatmap specified
        pb::msg_info("No target heatmap specified, so will not calculate EMD at end of run.");
        return;
    }

    // Ensure core heatmap has been initialised
    assert(m_heatmap.size_x() > 0 && m_heatmap.size_y() > 0);

    // Check if target heatmap file exists and can be opened
    std::string filename = Params::strTargetHeatmapFilename;
    std::ifstream file(filename);
    if (!file.is_open()) {
        pb::msg_error_and_exit("Cannot open target heatmap file: " + filename);
    }

    // Get expected dimensions from the core heatmap
    int expectedWidth = m_heatmap.size_x();
    int expectedHeight = m_heatmap.size_y();

    // Read CSV data
    std::vector<std::vector<double>> targetData;
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty()) continue; // Skip empty lines

        std::vector<double> row;
        std::stringstream ss(line);
        std::string cell;

        while (std::getline(ss, cell, ',')) {
            try {
                double value = std::stod(cell);
                row.push_back(value);
            } catch (const std::exception& e) {
                pb::msg_error_and_exit("Invalid numeric value in target heatmap CSV file: " + cell);
            }
        }

        if (!row.empty()) {
            targetData.push_back(row);
        }
    }

    file.close();

    // Check dimensions match
    if (targetData.empty()) {
        pb::msg_error_and_exit("Target heatmap file is empty: " + filename);
    }

    int actualHeight = static_cast<int>(targetData.size());
    int actualWidth = static_cast<int>(targetData[0].size());

    if (actualHeight != expectedHeight || actualWidth != expectedWidth) {
        pb::msg_error_and_exit(
            std::format("Target heatmap dimensions ({}x{} do not match core heatmap dimensions ({}x{})",
                actualWidth, actualHeight, expectedWidth, expectedHeight));
    }

    // Check all rows have the same width
    for (size_t i = 0; i < targetData.size(); ++i) {
        if (static_cast<int>(targetData[i].size()) != expectedWidth) {
            pb::msg_error_and_exit(std::format("Inconsistent row width in CSV file at row {}", i + 1));
        }
    }

    // Transpose the matrix before storing, because we access these matrices as [x][y] elsewhere
    std::vector<std::vector<double>> transposed(expectedWidth, std::vector<double>(expectedHeight));
    for (int i = 0; i < expectedHeight; ++i) {
        for (int j = 0; j < expectedWidth; ++j) {
            transposed[j][i] = targetData[i][j];
        }
    }

    // Store the transposed data
    m_rawTargetHeatmapNormalised = std::move(transposed);
}


// Reset the environment to its initial state suitable for a new simulation run
//
// This method resets are tranistory state of the environment that may have changed during a run,
// such as plant visit counts, bee positions and plant state.
//
// It does NOT reset fixed parameters of the environment such as positions of tunnels and barriers.
//
// Note, when running in optimization mode, tunnel entrance positions and hive positions are
// reset by PolyBeeOptimization::initialiseEntrancesAndHivesFromDecisionVector
//
void Environment::reset() {
    // TODO - as the class is developed, ensure all relevant state is reset here
    resetHivesAndBees();
    resetPlants();
    m_heatmap.reset();
}


void Environment::resetPlants() {
    m_plantGrid.clear();
    m_allPlants.clear();
    initialisePlants();
}


void Environment::resetHivesAndBees() {
    m_hives.clear();
    m_bees.clear();

    // Hives and bees will get re-initialised by initialiseHivesAndBees(), which should be called from outside
    // this method after resetting the environment (this happens in PolyBeeOptimization::fitness() before each run)
    // So we don't need to/should not call initialiseBees() here.
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


// Find an unvisited plant nearby to the given position (x,y).
// Returns std::nullopt if no unvisited plant is found that is both:
// * within visual range, and
// * not not obstructed by a barrier
//
// The 'visited' parameter is a list of plants that have recently been visited by the bee.
//
// This method considers only plants within visual range. If more than one unvisited plant is found,
// it uses a distance-weighted random selection to pick one.
//
std::optional<Plant*> Environment::selectNearbyUnvisitedPlant(float x, float y, const std::vector<Plant*>& visited) const
{
    std::vector<NearbyPlantInfo> visiblePlants;

    float rangeSq = Bee::visualRange() * Bee::visualRange();

    auto nearbyPlants = getNearbyPlants(x, y);
    for (Plant* pPlant : nearbyPlants) {
        if (std::find(visited.begin(), visited.end(), pPlant) != visited.end()) {
            continue;  // Skip already visited plants
        }

        float distSq = pb::distanceSq(x, y, pPlant->x(), pPlant->y()); // squared distance to plant

        if (distSq <= rangeSq) {
            // Plant is within visual range...
            if (!pathObstructedByBarrier(x, y, pPlant->x(), pPlant->y())) {
                // ... and is not obstructed by a barrier. So add it to the list!
                visiblePlants.emplace_back(pPlant, std::sqrt(distSq));
            }
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


// Return a flat vector of all barriers in the local 3x3 grid cells around the given position
// The x and y parameters are experessed in environment coordinates (not grid indices)
//
std::vector<Barrier*> Environment::getNearbyBarriers(float x, float y) const
{
    std::vector<Barrier*> nearbyBarriers;

    auto [i, j] = envPosToBarrierGridIndex(x, y);

    for (int di = -1; di <= 1; ++di) {
        for (int dj = -1; dj <= 1; ++dj) {
            if (i + di >= 0 && i + di < m_barrierGrid.size() &&
                j + dj >= 0 && j + dj < m_barrierGrid[i + di].size()) {
                const auto& cell = m_barrierGrid[i + di][j + dj];
                nearbyBarriers.insert(nearbyBarriers.end(), cell.begin(), cell.end());
            }
        }
    }

    return nearbyBarriers;
}


// Check for barriers in the 3x3 grid cells around the midpoint of the line between (x1,y1) and (x2,y2)
// and return true if any barrier intersects the line between (x1,y1) and (x2,y2)
bool Environment::pathObstructedByBarrier(float x1, float y1, float x2, float y2) const
{
    pb::Line2D pathLine(pb::Pos2D(x1, y1), pb::Pos2D(x2, y2));
    float midX = (x1 + x2) / 2.0f;
    float midY = (y1 + y2) / 2.0f;
    std::vector<Barrier*> nearbyBarriers = getNearbyBarriers(midX, midY);


    for (const Barrier* pBarrier : nearbyBarriers) {
        auto intersectInfo = pBarrier->line.getIntersectInfo(pathLine);
        if (intersectInfo.intersects && intersectInfo.withinBounds) {
            return true; // Line is obstructed by a barrier
        }
    }

    return false; // No barriers obstructing the line
}


// Check for barriers in the 3x3 grid cells around the midpoint of the line between (x1,y1) and (x2,y2)
// and return the distance from point (x1, y1) to the nearest barrier (or std::nullopt if no barriers nearby)
std::optional<float> Environment::distanceToNearestObstructingBarrier(float x1, float y1, float x2, float y2) const
{
    pb::Line2D pathLine(pb::Pos2D(x1, y1), pb::Pos2D(x2, y2));
    float minDistSq = std::numeric_limits<float>::max();
    bool foundBarrier = false;

    float midX = (x1 + x2) / 2.0f;
    float midY = (y1 + y2) / 2.0f;
    std::vector<Barrier*> nearbyBarriers = getNearbyBarriers(midX, midY);

    for (const Barrier* pBarrier : nearbyBarriers) {
        auto intersectInfo = pBarrier->line.getIntersectInfo(pathLine);
        if (intersectInfo.intersects && intersectInfo.withinBounds) {
            float distSq = (intersectInfo.point.x - x1) * (intersectInfo.point.x - x1) +
                           (intersectInfo.point.y - y1) * (intersectInfo.point.y - y1);
            if (distSq < minDistSq) {
                minDistSq = distSq;
                foundBarrier = true;
            }
        }
    }

    return (foundBarrier ? std::optional<float>(std::sqrt(minDistSq)) : std::nullopt);
}


// Return a flat vector of all plants in the local 3x3 grid cells around the given position
//
// These are selected purely based upon being present in the local 3x3 grid. This method
// does NOT take into account whether the plant is within the bee's visual range, nor
// whether or not it is objstructed by a barrier
//
std::vector<Plant*> Environment::getNearbyPlants(float x, float y) const
{
    std::vector<Plant*> nearbyPlants;

    auto [i, j] = envPosToPlantGridIndex(x, y);

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


double Environment::getSuccessfulVisitFraction() const
{
    if (m_allPlants.empty()) {
        return 0.0;
    }

    int successCount = 0;
    for (const Plant& plant : m_allPlants) {
        int vc = plant.visitCount();
        if (vc >= Params::minVisitCountSuccess && vc <= Params::maxVisitCountSuccess) {
            ++successCount;
        }
    }

    return static_cast<double>(successCount) / static_cast<double>(m_allPlants.size());
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
    float randValue = m_pPolyBeeCore->m_uniformProbDistrib(m_pPolyBeeCore->m_rngEngine) * totalWeight;

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


EntranceCrossingStats Environment::getEntranceCrossingStats(EntranceCrossingType type) const {

    EntranceCrossingStats stats;

    int numAttempts = 0;
    int numSuccesses = 0;
    int numRebounds = 0;
    for (const Bee& bee : m_bees) {
        const auto& records = bee.entranceCrossingRecords();
        for (const auto& record : records) {
            if (type == EntranceCrossingType::ALL ||
                (type == EntranceCrossingType::ENTRY && record.isEntry) ||
                (type == EntranceCrossingType::EXIT && !record.isEntry)) {
                numAttempts++;
                numSuccesses += record.success ? 1 : 0;
                numRebounds += record.numRebounds;
            }
        }
    }

    stats.successRate = numAttempts > 0 ? static_cast<float>(numSuccesses) / static_cast<float>(numAttempts) : 0.0f;
    stats.meanRebounds = numAttempts > 0 ? static_cast<float>(numRebounds) / static_cast<float>(numAttempts) : 0.0f;

    return stats;
}
