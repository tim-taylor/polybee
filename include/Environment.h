/**
 * @file
 *
 * Declaration of the Environment class
 */

#ifndef _ENVIRONMENT_H
#define _ENVIRONMENT_H

#include "Bee.h"
#include "Hive.h"
#include "Tunnel.h"
#include "Plant.h"
#include "Heatmap.h"
#include "utils.h"
#include <vector>
#include <optional>


// struct to hold information about a plant near a bee
struct NearbyPlantInfo {
    Plant* pPlant;
    float dist; // distance of plant from bee

    NearbyPlantInfo(Plant* plant, float distance) : pPlant(plant), dist(distance) {}
};


/**
 * The Environment class ...
 */
class Environment {

public:
    Environment();
    ~Environment() {}

    void initialise();
    void update();
    void reset(); // reset environment to initial state

    bool inTunnel(float x, float y) const;

    const Tunnel& getTunnel() const { return m_tunnel; }
    const Heatmap& getHeatmap() const { return m_heatmap; }
    const std::vector<Bee>& getBees() const { return m_bees; }
    const std::vector<Plant>& getAllPlants() const { return m_allPlants; }
    std::vector<Plant*> getNearbyPlants(float x, float y) const;
    std::optional<Plant*> getNearestUnvisitedPlant(float x, float y, const std::vector<Plant*>& visited) const; // get nearest plant to a given position within maxDistance

private:
    void initialiseTunnel();
    void initialisePlants();
    void initialiseHives();
    void initialiseBees();
    void initialiseHeatmap();
    void resetBees();
    void resetPlants();
    pb::Pos2D envPosToGridIndex(float x, float y) const;
    Plant* pickRandomPlantWeightedByDistance(const std::vector<NearbyPlantInfo>& plants) const;

    float m_width;
    float m_height;
    std::vector<Bee> m_bees;
    std::vector<Hive> m_hives;
    Tunnel m_tunnel;
    std::vector<Plant> m_allPlants;  // Owns all Plant objects
    std::vector<std::vector<std::vector<Plant*>>> m_plantGrid;  // Spatial index with pointers into m_allPlants
    float m_plantGridCellSize {1.0f};
    size_t m_plantGridW {1};
    size_t m_plantGridH {1};
    Heatmap m_heatmap;
};

#endif /* _ENVIRONMENT_H */
