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
#include "Flowmap.h"
#include "utils.h"
#include <vector>
#include <optional>
#include <cassert>

class PolyBeeCore;


// struct to hold information about a plant near a bee
struct NearbyPlantInfo {
    Plant* pPlant;
    float dist; // distance of plant from bee

    NearbyPlantInfo(Plant* plant, float distance) : pPlant(plant), dist(distance) {}
};


struct EntranceCrossingStats {
    float successRate {0.0f};   // fraction of crossing attempts that were successful (one "attempt" might involve multiple rebounds)
    float meanRebounds {0.0f};  // mean number of rebounds
};


struct Barrier {
    pb::Line2D line;

    Barrier(float x1, float y1, float x2, float y2) : line(pb::Line2D(pb::Pos2D(x1, y1), pb::Pos2D(x2, y2))) {}

    float x1() const { return line.start.x; }
    float y1() const { return line.start.y; }
    float x2() const { return line.end.x; }
    float y2() const { return line.end.y; }
};


enum class EntranceCrossingType {
    ENTRY,
    EXIT,
    ALL
};


/**
 * The Environment class ...
 */
class Environment {

public:
    Environment();
    ~Environment() {}

    void initialise(PolyBeeCore* pCore);
    void update();
    void resetForNewRun(const std::vector<HiveSpec>& hiveSpecs, const std::vector<PatchSpec>& bridgeSpecs);

    bool inTunnel(float x, float y) const;

    Tunnel& getTunnel() { return m_tunnel; }
    const Heatmap& getHeatmap() const { return m_heatmap; }
    Flowmap& getFlowmap() { return m_flowmap; }
    const std::vector<std::vector<double>>& getRawTargetHeatmapNormalised() const { return m_rawTargetHeatmapNormalised; }
    const std::vector<Bee>& getBees() const { return m_bees; }
    const std::vector<Hive>& getHives() const { return m_hives; }

    const std::vector<Plant>& getAllPlants() const { return m_allPlants; }
    const std::vector<Plant*>& getPlantsForSVFCalc() const { return m_plantsForSVFCalc; }
    std::vector<Plant*> getNearbyPlants(float x, float y) const;
    std::optional<Plant*> selectNearbyUnvisitedPlant(float x, float y, const std::vector<Plant*>& visited) const; // get nearest plant to a given position within maxDistance

    const std::vector<Barrier>& getAllBarriers() const { return m_allBarriers; }
    std::vector<Barrier*> getNearbyBarriers(float x, float y) const;
    bool pathObstructedByBarrier(float x1, float y1, float x2, float y2) const;
    std::optional<float> distanceToNearestObstructingBarrier(float x1, float y1, float x2, float y2) const;

    double getSuccessfulVisitFraction() const;  // fraction of plants that have a visit count in the successful visit range
                                                // (between minVisitCountSuccess and maxVisitCountSuccess, inclusive)

    PolyBeeCore* getPolyBeeCore() { assert(m_pPolyBeeCore != nullptr); return m_pPolyBeeCore; }

    void initialiseBarriers(const std::vector<BarrierSpec>& barrierSpecs);
    void initialiseHivesAndBees(const std::vector<HiveSpec>& hiveSpecs);
    void initialisePlants(const std::vector<PatchSpec>& patchSpecs, bool extraPlantsForEvolvingBridges = false);

    EntranceCrossingStats getEntranceCrossingStats(EntranceCrossingType type) const; // get stats on tunnel entrance crossing attempts across all bees, for current state of the environment

private:
    void initialiseTunnel();
    void initialiseBarriers();
    void initialisePlants();
    void initialiseHivesAndBees();
    void initialiseBees(); // this should only be called from initialiseHivesAndBees
    void initialiseHeatmap();
    void initialiseTargetHeatmap();
    void initialiseFlowmap();
    void resetHivesAndBees(const std::vector<HiveSpec>& hiveSpecs);
    void resetPlants(const std::vector<PatchSpec>& bridgeSpecs);
    pb::Pos2D envPosToPlantGridIndex(float x, float y) const;
    pb::Pos2D envPosToBarrierGridIndex(float x, float y) const;
    Plant* pickRandomPlantWeightedByDistance(const std::vector<NearbyPlantInfo>& plants) const;

    float m_width;
    float m_height;
    std::vector<Bee> m_bees;
    std::vector<Hive> m_hives;
    Tunnel m_tunnel;

    std::vector<Plant> m_allPlants;                                 // Owns all Plant objects
    std::vector<Plant*> m_plantsForSVFCalc;                         // Pointers to plants for Successful Visit Fraction calculation
    std::vector<std::vector<std::vector<Plant*>>> m_plantGrid;      // Spatial index for plants, with pointers into m_allPlants
    float m_plantGridCellSize {1.0f};
    size_t m_plantGridW {1};
    size_t m_plantGridH {1};

    std::vector<Barrier> m_allBarriers;                             // Owns all Barrier objects
    std::vector<std::vector<std::vector<Barrier*>>> m_barrierGrid;  // Spatial index for barriers, with pointers into m_allBarriers
    float m_barrierGridCellSize {1.0f};
    size_t m_barrierGridW {1};
    size_t m_barrierGridH {1};

    Heatmap m_heatmap;
    std::vector<std::vector<double>> m_rawTargetHeatmapNormalised;  // target heatmap for use in PolyBeeEvolve, and for calculating EMD in one-off runs

    Flowmap m_flowmap;

    PolyBeeCore* m_pPolyBeeCore { nullptr };
};

#endif /* _ENVIRONMENT_H */
