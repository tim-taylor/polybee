/**
 * @file
 *
 * Implementation of the PolyBeeEvolve class
 */

#include "PolyBeeEvolve.h"
#include "PolyBeeCore.h"
#include "Params.h"
#include "utils.h"
#include <numbers>
#include <pagmo/types.hpp>
#include <pagmo/problem.hpp>
#include <pagmo/population.hpp>
#include <pagmo/archipelago.hpp>
#include <pagmo/topology.hpp>
#include <pagmo/topologies/ring.hpp>
#include <pagmo/topologies/unconnected.hpp>
#include <pagmo/r_policy.hpp>
#include <pagmo/r_policies/fair_replace.hpp>
#include <pagmo/s_policy.hpp>
#include <pagmo/s_policies/select_best.hpp>
#include <pagmo/island.hpp>
#include <pagmo/algorithm.hpp>
#include <pagmo/algorithms/sga.hpp>
//#include <pagmo/algorithms/de1220.hpp> // not suitable for stochastic problems
//#include <pagmo/algorithms/sade.hpp>   // not suitable for stochastic problems
#include <pagmo/algorithms/gaco.hpp>
#include <pagmo/algorithms/pso_gen.hpp>
//#include <pagmo/algorithms/cmaes.hpp> // produces an error (tries to use side=5) at end of first generation
//#include <pagmo/algorithms/xnes.hpp>  // produces an error (tries to use side=5) at end of first generation
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <format>
#include <cassert>
#include <numeric>
#include <algorithm>
#include <random>


// Constructor
PolyBeeOptimization::PolyBeeOptimization(PolyBeeEvolve* ptr, const EvolveSpec& spec, std::size_t islandNum)
: m_pPolyBeeEvolve(ptr),
  m_islandNum(islandNum),
  m_evolveEntrancePositions(spec.evolveEntrancePositions),
  m_evolveHivePositions(spec.evolveHivePositions),
  m_evolveBridgePositions(spec.evolveBridgePositions),
  m_evolveBarrierPositions(spec.evolveBarrierPositions),
  m_numEntrances(spec.numEntrances),
  m_entranceWidth(spec.entranceWidth),
  m_numHivesInsideTunnel(spec.numHivesInsideTunnel),
  m_numHivesOutsideTunnel(spec.numHivesOutsideTunnel),
  m_numHivesFree(spec.numHivesFree),
  m_numBridges(spec.numBridges),
  m_bridgeWidth(spec.bridgeWidth),
  m_numBarriers(spec.numBarriers),
  m_barrierWidth(spec.barrierWidth)
{
    // first do some checks to make sure the parameters are consistent with each other
    if ((!m_evolveEntrancePositions) && (m_numEntrances != 0)) {
        pb::msg_error_and_exit("PolyBeeOptimization constructor: evolveEntrancePositions is false but numEntrances is not zero");
    }

    if ((!m_evolveHivePositions) && (m_numHivesInsideTunnel + m_numHivesOutsideTunnel + m_numHivesFree != 0)) {
        pb::msg_error_and_exit("PolyBeeOptimization constructor: evolveHivePositions is false but numHivesInsideTunnel, numHivesOutsideTunnel, and numHivesFree are not all zero");
    }

    if ((!m_evolveBridgePositions) && (m_numBridges != 0)) {
        pb::msg_error_and_exit("PolyBeeOptimization constructor: evolveBridgePositions is false but numBridges is not zero");
    }

    if ((!m_evolveBarrierPositions) && (m_numBarriers != 0)) {
        pb::msg_error_and_exit("PolyBeeOptimization constructor: evolveBarrierPositions is false but numBarriers is not zero");
    }

    if (!m_evolveEntrancePositions && !m_evolveHivePositions && !m_evolveBridgePositions && !m_evolveBarrierPositions) {
        pb::msg_error_and_exit("PolyBeeOptimization constructor: all evolve positions flags cannot be false");
    }

    // We need to establish how many decision variables we have, which ones are integers (as opposed to float)
    // and what their bounds are

    // For each tunnel entrance we need one float and one integer decision variable, where the float variable
    // determines the position of the entrance [0.0-1.0] along the side of the tunnel, and the integer variable
    // determines which side of the tunnel the entrance is on (0=North, 1=East, 2=South, 3=West)
    // => 1F + 1I per entrance

    // For each hive inside the tunnel we need two float decision variables that determine the position of the hive
    // [0.0-1.0] along the horizontal and vertical sides of the tunnel repectively and an int to specify the
    // direction of hive exit (0=North, 1=East, 2=South, 3=West, 4=Random)
    // => 2F + 1I per hive inside tunnel

    // For each hive outside the tunnel we need two float and two integer decision variables. The float
    // variables determine the position of the hive [0.0-1.0] along the horizontal and vertical axes of
    // that sector of the environment, the first integer specifies the direction of hive exit (0=North,
    // 1=East, 2=South, 3=West, 4=Random), and the second integer specifies the sector of the environment
    // that the hive is in (0=North, 1=East, 2=South, 3=West)
    // => 2F + 2I per hive outside tunnel

    // For each free hive we need two float decision variables that determine the position of the hive [0.0-1.0]
    // along the horizontal and vertical axes of the entire environment, and one integer variable to specify the
    // direction of hive exit (0=North, 1=East, 2=South, 3=West, 4=Random)
    // => 2F + 1I per free hive

    // For each bridge we need two float decision variables that determine the position of the bridge [0.0-1.0]
    // along the horizontal and vertical axes of the entire environment
    // => 2F per bridge

    // For each barrier we need two float and one integer decision variables, where the two floats determine the
    // position of the barrier [0.0-1.0] along the horizontal and vertical axes of the entire environment, and the
    // int variable third variable determines the orientation of the barrier [0-35] mapping to discretized
    // orientations in the range [0 - 2PI] (0,10,20,...350 degrees)
    // => 2F + 1I per barrier

    m_numFloatVars = m_numEntrances * 1 + m_numHivesInsideTunnel * 2 + m_numHivesOutsideTunnel * 2 + m_numHivesFree * 2 + m_numBridges * 2 + m_numBarriers * 2;
    m_numIntegerVars = m_numEntrances * 1 + m_numHivesInsideTunnel * 1 + m_numHivesOutsideTunnel * 2 + m_numHivesFree * 1 + m_numBarriers * 1;

    // Set bounds for decision variables.
    // Decision variables are laid out as: [float vars...][integer vars...]
    // within each section the order matches the order used in initialiseEnvironmentFromDecisionVector().
    m_lowerBounds.clear();
    m_upperBounds.clear();

    // Helper: append n copies of [lb, ub] to the bound vectors
    auto addBounds = [&](int n, double lb, double ub) {
        for (int i = 0; i < n; ++i) {
            m_lowerBounds.push_back(lb);
            m_upperBounds.push_back(ub);
        }
    };

    // --- Float variables [0.0, 1.0] ---
    addBounds(m_numEntrances,            0.0, 1.0); // entrance: position along side
    addBounds(m_numHivesInsideTunnel*2,  0.0, 1.0); // inside-tunnel hive: x, y
    addBounds(m_numHivesOutsideTunnel*2, 0.0, 1.0); // outside-tunnel hive: x, y
    addBounds(m_numHivesFree*2,          0.0, 1.0); // free hive: x, y
    addBounds(m_numBridges*2,            0.0, 1.0); // bridge: x, y
    addBounds(m_numBarriers*2,           0.0, 1.0); // barrier: x, y

    // --- Integer variables ---
    addBounds(m_numEntrances,          0.0, 3.0); // entrance side          (0=N, 1=E, 2=S, 3=W)
    addBounds(m_numHivesInsideTunnel,  0.0, 4.0); // inside-tunnel hive dir (0=N..3=W, 4=Random)
    for (int i = 0; i < m_numHivesOutsideTunnel; ++i) {
        addBounds(1, 0.0, 4.0); // outside-tunnel hive direction (0=N..3=W, 4=Random)
        addBounds(1, 0.0, 3.0); // outside-tunnel hive sector    (0=N, 1=E, 2=S, 3=W)
    }
    addBounds(m_numHivesFree, 0.0, 4.0);  // free hive direction     (0=N..3=W, 4=Random)
    addBounds(m_numBarriers,  0.0, 35.0); // barrier orientation     (0–35, maps to 0°–350° in 10° steps)

    assert(m_lowerBounds.size() == m_numFloatVars + m_numIntegerVars);
    assert(m_upperBounds.size() == m_numFloatVars + m_numIntegerVars);
}


// Implementation of the objective function.
pagmo::vector_double PolyBeeOptimization::fitness(const pagmo::vector_double &dv) const
{
    std::vector<double> fitnessValues;
    PolyBeeCore& core = m_pPolyBeeEvolve->polyBeeCore(m_islandNum);
    const Environment& env = core.getEnvironment();
    bool firstCall = (core.isMasterCore() && core.evaluationCount() == 0);

    assert(dv.size() == m_numFloatVars + m_numIntegerVars);

    std::vector<float> tunnelLengths = {
        Params::tunnelW - m_entranceWidth, // North
        Params::tunnelH - m_entranceWidth, // East
        Params::tunnelW - m_entranceWidth, // South
        Params::tunnelH - m_entranceWidth  // West
    };

    // define local vectors to hold the entrance, hive, bridge, and barrier specs that will be derived from the
    // decision vector (in initialiseEnvironmentFromDecisionVector) and used, where appropriate, to initialise
    // the environment for this run
    std::vector<EntranceSpec> entranceSpecs;
    std::vector<HiveSpec> hiveSpecs;
    std::vector<PatchSpec> bridgeSpecs;
    std::vector<BarrierSpec> barrierSpecs;

    initialiseEnvironmentFromDecisionVector(core, dv, tunnelLengths,
        entranceSpecs, hiveSpecs, bridgeSpecs, barrierSpecs);

    // Write config file for this configuration once at the start of the run
    if (firstCall) {
        core.writeConfigFile();
        std::cout << "~~~~~~~~~~" << std::endl;
    }

    for (int i = 0; i < Params::numTrialsPerConfig; ++i)
    {
        core.incrementEvaluationCount();
        // for each replicate run we need to reset all parts of the simulation that have changing state,
        // i.e. hives, bees and plants
        core.resetForNewRun(
            //
            // if we're evolving hive positions, use the hive specs derived from the decision vector, otherwise
            // use the regular hive specs from Params
            (Params::evolveSpec.evolveHivePositions ? hiveSpecs : Params::hiveSpecs),
            //
            // env.resetForNewRun() will treat these bridge specs as additional to the regular plant patches in
            // Params::patchSpecs, so no need to do anything special with them here.
            bridgeSpecs
        );
        core.run(false); // false = do not log output files during the run

        double objValue = 0.0;
        switch (Params::evolveObjective) {
            case EvolveObjective::EMD_TO_TARGET_HEATMAP: {
                const Heatmap& runHeatmap = core.getHeatmap();
                objValue = runHeatmap.emd(env.getRawTargetHeatmapNormalised());
                break;
            }
            case EvolveObjective::FRACTION_FLOWERS_SUCCESSFUL_VISIT_RANGE: {
                // N.B. we negate the fraction of successfully visited flowers here because pagmo minimizes
                // the objective function, but we want to maximize this fraction
                objValue = -(core.getSuccessfulVisitFraction());
                break;
            }
            default: {
                pb::msg_error_and_exit(std::format("Invalid evolve objective {} specified in Params::evolveObjective",
                    Params::evolveObjectivePvt));
            }
        }

        fitnessValues.push_back(objValue);
    }

    const double medianObjValue = pb::median(fitnessValues);

    int num_evals_per_gen = Params::numConfigsPerGen * Params::numTrialsPerConfig;
    int gen = (core.evaluationCount()-1) / num_evals_per_gen;
    int eval_in_gen = (core.evaluationCount()-1) % num_evals_per_gen;
    int config_num = eval_in_gen / Params::numTrialsPerConfig;

    // Output some info about the current configuration and its fitness value
    std::string msg = std::format("isl {} gen {} evl {} cnf {} mdF {:.5f} ",
        core.getIslandNum(), gen, core.evaluationCount(), config_num, medianObjValue);
    if (Params::evolveSpec.evolveEntrancePositions) {
        msg += "/e/ ";
        for (int i = 0; i < entranceSpecs.size(); ++i) {
            msg += std::format("e{} {:.1f},{:.1f}:{} ", i, entranceSpecs[i].e1, entranceSpecs[i].e2, entranceSpecs[i].side);
        }
    }
    if (Params::evolveSpec.evolveHivePositions) {
        msg += "/h/ ";
        for (int i = 0; i < hiveSpecs.size(); ++i) {
            msg += std::format("h{} {:.1f},{:.1f}:{} ", i, hiveSpecs[i].x, hiveSpecs[i].y, hiveSpecs[i].direction);
        }
    }
    if (Params::evolveSpec.evolveBridgePositions) {
        msg += "/b/ ";
        for (int i = 0; i < bridgeSpecs.size(); ++i) {
            msg += std::format("b{} {:.1f},{:.1f} ", i, bridgeSpecs[i].x, bridgeSpecs[i].y);
        }
    }
    if (Params::evolveSpec.evolveBarrierPositions) {
        msg += "/x/ ";
        for (int i = 0; i < barrierSpecs.size(); ++i) {
            msg += std::format("r{} {:.1f},{:.1f},{:.1f},{:.1f} ", i, barrierSpecs[i].x1, barrierSpecs[i].y1, barrierSpecs[i].x2, barrierSpecs[i].y2);
        }
    }
    pb::msg_info(msg);

    return {medianObjValue};
}


// a private helper method for PolyBeeOptimization::fitness() that reads the decision vector and initialises
// Translates a decision vector into entrance, hive, bridge, and barrier specifications, then
// applies them to the environment ready for the next evaluation run.
void PolyBeeOptimization::initialiseEnvironmentFromDecisionVector(
    PolyBeeCore& core, const pagmo::vector_double& dv, const std::vector<float>& tunnelLengths,
    std::vector<EntranceSpec>& entranceSpecs, std::vector<HiveSpec>& hiveSpecs,
    std::vector<PatchSpec>& bridgeSpecs, std::vector<BarrierSpec>& barrierSpecs) const
{
    // Float vars occupy dv[0 .. m_numFloatVars-1]; integer vars follow immediately after.
    // The four helpers below advance these indices in the same order as the bounds were built
    // in the constructor, so the cursor moves correctly across all four groups.
    std::size_t floatIdx = 0;
    std::size_t intIdx   = m_numFloatVars;

    initialiseEntrancesFromDV(core, dv, tunnelLengths, floatIdx, intIdx, entranceSpecs);
    initialiseHivesFromDV    (core, dv,                floatIdx, intIdx, hiveSpecs);
    initialiseBridgesFromDV  (core, dv,                floatIdx,         bridgeSpecs);
    initialiseBarriersFromDV (core, dv,                floatIdx, intIdx, barrierSpecs);
}


void PolyBeeOptimization::initialiseEntrancesFromDV(
    PolyBeeCore& core, const pagmo::vector_double& dv, const std::vector<float>& tunnelLengths,
    std::size_t& floatIdx, std::size_t& intIdx,
    std::vector<EntranceSpec>& entranceSpecs) const
{
    if (!m_evolveEntrancePositions) {
        // Entrances were already set up from Params::entranceSpecs during environment initialisation.
        assert(!core.getTunnel().getEntrances().empty());
        return;
    }

    for (int i = 0; i < m_numEntrances; ++i) {
        int   side = static_cast<int>(dv[intIdx++]);
        float e1   = static_cast<float>(dv[floatIdx++]) * tunnelLengths[side];
        float e2   = e1 + m_entranceWidth;
        entranceSpecs.emplace_back(e1, e2, side);
    }
    core.getTunnel().initialiseEntrances(entranceSpecs);
}


void PolyBeeOptimization::initialiseHivesFromDV(
    PolyBeeCore& core, const pagmo::vector_double& dv,
    std::size_t& floatIdx, std::size_t& intIdx,
    std::vector<HiveSpec>& hiveSpecs) const
{
    if (!m_evolveHivePositions) {
        // Hives were already set up from Params::hiveSpecs during environment initialisation.
        assert(!core.getHives().empty());
        return;
    }

    // sf/mf add a small safety margin so positions never land right on a wall or border
    const float sf = 0.98f;
    const float mf = (1.0f - sf) / 2.0f;

    // Inside-tunnel hives
    for (int i = 0; i < m_numHivesInsideTunnel; ++i) {
        float lx  = mf * Params::tunnelW + static_cast<float>(dv[floatIdx++]) * Params::tunnelW * sf;
        float ly  = mf * Params::tunnelH + static_cast<float>(dv[floatIdx++]) * Params::tunnelH * sf;
        int   dir = static_cast<int>(dv[intIdx++]);
        hiveSpecs.emplace_back(Params::tunnelX + lx, Params::tunnelY + ly, dir);
    }

    // Outside-tunnel hives – the sector integer picks which region of the environment to place the hive in
    const float RregionLeft   = Params::tunnelX + Params::tunnelW;
    const float RregionWidth  = Params::envW - RregionLeft;
    const float TregionHeight = Params::tunnelY;
    const float BregionTop    = Params::tunnelY + Params::tunnelH;
    const float BregionHeight = Params::envH - BregionTop;

    for (int i = 0; i < m_numHivesOutsideTunnel; ++i) {
        float _x    = static_cast<float>(dv[floatIdx++]);
        float _y    = static_cast<float>(dv[floatIdx++]);
        int   dir    = static_cast<int>(dv[intIdx++]);
        int   sector = static_cast<int>(dv[intIdx++]);

        float x, y;
        switch (sector) {
            case 0: // North (top)
                x = mf * Params::envW    + _x * Params::envW    * sf;
                y = mf * TregionHeight   + _y * TregionHeight   * sf;
                break;
            case 1: // East (right)
                x = RregionLeft + mf * RregionWidth  + _x * RregionWidth  * sf;
                y = Params::tunnelY + mf * Params::tunnelH + _y * Params::tunnelH * sf;
                break;
            case 2: // South (bottom)
                x = mf * Params::envW    + _x * Params::envW    * sf;
                y = BregionTop + mf * BregionHeight  + _y * BregionHeight  * sf;
                break;
            case 3: // West (left)
                x = mf * Params::tunnelX + _x * Params::tunnelX * sf;
                y = Params::tunnelY + mf * Params::tunnelH + _y * Params::tunnelH * sf;
                break;
            default:
                pb::msg_error_and_exit(std::format("Invalid sector value {} for outside-tunnel hive in decision vector", sector));
        }
        hiveSpecs.emplace_back(x, y, dir);
    }

    // Free hives (can be placed anywhere in the environment)
    for (int i = 0; i < m_numHivesFree; ++i) {
        float x   = mf * Params::envW + static_cast<float>(dv[floatIdx++]) * Params::envW * sf;
        float y   = mf * Params::envH + static_cast<float>(dv[floatIdx++]) * Params::envH * sf;
        int   dir = static_cast<int>(dv[intIdx++]);
        hiveSpecs.emplace_back(x, y, dir);
    }
    // Hives are not initialised in the environment here: they are re-initialised along with the bees
    // in the call to core.resetForNewRun() in fitness().
}


void PolyBeeOptimization::initialiseBridgesFromDV(
    PolyBeeCore& core, const pagmo::vector_double& dv,
    std::size_t& floatIdx,
    std::vector<PatchSpec>& bridgeSpecs) const
{
    if (!m_evolveBridgePositions) return;

    const float sz    = m_bridgeWidth;
    const float delta = 20.0f; // shift distance for Moore-neighbourhood repair

    // 8 Moore-neighbourhood offsets listed in a fixed order so we can rotate the
    // starting offset by bridge index i to avoid a consistent directional bias
    const std::pair<float,float> mooreOffsets[8] = {
        {-delta, -delta}, { 0.0f, -delta}, { delta, -delta},
        {-delta,  0.0f },                  { delta,  0.0f },
        {-delta,  delta}, { 0.0f,  delta}, { delta,  delta}
    };

    const Tunnel& tunnel = core.getTunnel();

    // Returns true if the bridge rect [bx, bx+sz] x [by, by+sz] clips a tunnel wall segment
    // (i.e. any of its 4 edges crosses the tunnel boundary outside of an entrance opening).
    // A crossing through an entrance is allowed (withinBounds=true); a wall hit is not (withinBounds=false).
    auto clipsWall = [&](float bx, float by) -> bool {
        float r = bx + sz;
        float b = by + sz;
        auto hitsWall = [&](float x1, float y1, float x2, float y2) {
            auto info = tunnel.intersectsTunnelBoundary(x1, y1, x2, y2);
            return info.intersects && !info.withinBounds;
        };
        return hitsWall(bx, by, r,  by)   // top edge of bridge
            || hitsWall(r,  by, r,  b )   // right edge of bridge
            || hitsWall(r,  b,  bx, b )   // bottom edge of bridge
            || hitsWall(bx, b,  bx, by);  // left edge of bridge
    };

    // Returns true if the bridge rect [bx, bx+sz] x [by, by+sz] overlaps a given PatchSpec,
    // checking every repeat of the patch.
    auto overlapsPatch = [&](float bx, float by, const PatchSpec& patch) -> bool {
        for (int k = 0; k < patch.numRepeats; ++k) {
            float px = patch.x + k * patch.dx;
            float py = patch.y + k * patch.dy;
            if (bx < px + patch.w &&    // if the top of p is above the bottom of b
                px < bx + sz &&         // and the bottom of p is below the top of b
                by < py + patch.h &&    // and the left of p is to the left of the right of b
                py < by + sz) {         // and the right of p is to the right of the left of b
                return true;            // then the rects overlap
            }
        }
        return false;
    };

    // Returns true if (bx, by) is a valid placement: no tunnel wall clip and no overlap with
    // any existing plant patch or previously placed bridge.
    auto isValid = [&](float bx, float by) -> bool {
        if (clipsWall(bx, by)) return false;
        for (const auto& patch : Params::patchSpecs) {
            if (overlapsPatch(bx, by, patch)) return false;
        }
        for (const auto& bridge : bridgeSpecs) {
            if (overlapsPatch(bx, by, bridge)) return false;
        }
        return true;
    };

    for (int i = 0; i < m_numBridges; ++i) {
        float fx = static_cast<float>(dv[floatIdx++]); // x as fraction in [0,1]
        float fy = static_cast<float>(dv[floatIdx++]); // y as fraction in [0,1]
        float x  = fx * (Params::envW - sz);           // convert to env coordinates
        float y  = fy * (Params::envH - sz);

        if (isValid(x, y)) {
            bridgeSpecs.emplace_back(x, y, sz, Params::plantDefaultSpacing, Params::plantDefaultJitter, true);
            continue;
        }

        // Try the 8 Moore-neighbourhood positions, rotating the starting point by i
        // so that no single direction is consistently favoured across all bridges
        bool placed = false;
        for (int j = 0; j < 8; ++j) {
            const auto& [ox, oy] = mooreOffsets[(i + j) % 8];
            if (isValid(x + ox, y + oy)) {
                bridgeSpecs.emplace_back(x + ox, y + oy, sz, Params::plantDefaultSpacing, Params::plantDefaultJitter, true);
                placed = true;
                break;
            }
        }
        // If no valid position was found in the neighbourhood, the bridge is ditched
        /*
        if (!placed) {
            pb::msg_info(std::format("Bridge {} ditched: no valid placement found near ({:.1f},{:.1f})", i, x, y));
        }
        */
    }
    // Bridge patches are not initialised in the environment here: they are re-initialised along
    // with all patches in the call to core.resetForNewRun() in fitness().
}


void PolyBeeOptimization::initialiseBarriersFromDV(
    PolyBeeCore& core, const pagmo::vector_double& dv,
    std::size_t& floatIdx, std::size_t& intIdx,
    std::vector<BarrierSpec>& barrierSpecs) const
{
    if (!m_evolveBarrierPositions) return;

    float halfWidth = m_barrierWidth / 2.0f;

    for (int i = 0; i < m_numBarriers; ++i) {
        float fx          = static_cast<float>(dv[floatIdx++]); // x expressed as a fraction
        float fy          = static_cast<float>(dv[floatIdx++]); // y expressed as a fraction
        int   discOri     = static_cast<int>(dv[intIdx++]);     // discrete orientation in [0,35] mapping to 0°–350° in 10° steps

        float x           = halfWidth + (fx * (Params::envW - m_barrierWidth));   // convert to actual env coordinates
        float y           = halfWidth + (fy * (Params::envH - m_barrierWidth));   // convert to actual env coordinates
        float orientation = (discOri / 36.0f) * std::numbers::pi_v<float> * 2.0f; // map integer in [0,35] to radians in [0,2PI]

        barrierSpecs.emplace_back(FromMidpoints{}, x, y, m_barrierWidth, orientation);
    }
    core.getEnvironment().initialiseBarriers(barrierSpecs);
}


// Implementation of the box bounds.
std::pair<pagmo::vector_double, pagmo::vector_double> PolyBeeOptimization::get_bounds() const
{
    return {m_lowerBounds, m_upperBounds};
    //return {{0.0, 0.0, 0.0, 0.0, 0, 0, 0, 0}, {1.0, 1.0, 1.0, 1.0, 3, 3, 3, 3}};
}


// Define the number of integer (as opposed to continuous) decision variables
pagmo::vector_double::size_type PolyBeeOptimization::get_nix() const {
    return m_numIntegerVars;
    //return 4; // last 4 decision variables are integers
}


// Implementation of replace_random replacement policy
pagmo::individuals_group_t replace_random::replace(
    const pagmo::individuals_group_t &inds,
    const pagmo::vector_double::size_type &/*nx*/,
    const pagmo::vector_double::size_type &/*nix*/,
    const pagmo::vector_double::size_type &/*nobj*/,
    const pagmo::vector_double::size_type &/*nec*/,
    const pagmo::vector_double::size_type &/*nic*/,
    const pagmo::vector_double &/*tol*/,
    const pagmo::individuals_group_t &mig
) const
{
    // inds is a tuple of (IDs, decision vectors, fitness vectors) for current population
    // mig is the same structure for incoming migrants

    const auto &[ids, dvs, fvs] = inds;
    const auto &[mig_ids, mig_dvs, mig_fvs] = mig;

    // If no migrants or empty population, return unchanged
    if (mig_ids.empty() || ids.empty()) {
        return inds;
    }

    // Determine how many replacements to make
    const auto pop_size = ids.size();
    const auto n_mig = mig_ids.size();
    const auto n_replace = std::min({static_cast<std::size_t>(m_rate), n_mig, pop_size});

    if (n_replace == 0) {
        return inds;
    }

    // Make a copy of the population to modify
    pagmo::individuals_group_t result = inds;
    auto &[res_ids, res_dvs, res_fvs] = result;

    // Create indices for random selection without replacement
    std::vector<std::size_t> pop_indices(pop_size);
    std::iota(pop_indices.begin(), pop_indices.end(), 0);

    // Shuffle using PolyBeeCore's RNG engine
    std::shuffle(pop_indices.begin(), pop_indices.end(), m_pCore->m_rngEngine);

    // Replace n_replace randomly selected individuals with migrants
    for (std::size_t i = 0; i < n_replace; ++i) {
        std::size_t replace_idx = pop_indices[i];
        res_ids[replace_idx] = mig_ids[i];
        res_dvs[replace_idx] = mig_dvs[i];
        res_fvs[replace_idx] = mig_fvs[i];
    }

    return result;
}


std::string replace_random::get_extra_info() const
{
    return "Max replacement rate: " + std::to_string(m_rate);
}


// Implementation of select_random selection policy
pagmo::individuals_group_t select_random::select(
    const pagmo::individuals_group_t &inds,
    const pagmo::vector_double::size_type &/*nx*/,
    const pagmo::vector_double::size_type &/*nix*/,
    const pagmo::vector_double::size_type &/*nobj*/,
    const pagmo::vector_double::size_type &/*nec*/,
    const pagmo::vector_double::size_type &/*nic*/,
    const pagmo::vector_double &/*tol*/
) const
{
    // inds is a tuple of (IDs, decision vectors, fitness vectors) for current population

    const auto &[ids, dvs, fvs] = inds;

    // If empty population, return empty result
    if (ids.empty()) {
        return inds;
    }

    // Determine how many individuals to select
    const auto pop_size = ids.size();
    const auto n_select = std::min(static_cast<std::size_t>(m_rate), pop_size);

    if (n_select == 0) {
        return {{}, {}, {}};
    }

    // Create indices for random selection without replacement
    std::vector<std::size_t> pop_indices(pop_size);
    std::iota(pop_indices.begin(), pop_indices.end(), 0);

    // Shuffle using PolyBeeCore's RNG engine
    std::shuffle(pop_indices.begin(), pop_indices.end(), m_pCore->m_rngEngine);

    // Build result with n_select randomly chosen individuals
    pagmo::individuals_group_t result;
    auto &[res_ids, res_dvs, res_fvs] = result;

    res_ids.reserve(n_select);
    res_dvs.reserve(n_select);
    res_fvs.reserve(n_select);

    for (std::size_t i = 0; i < n_select; ++i) {
        std::size_t idx = pop_indices[i];
        res_ids.push_back(ids[idx]);
        res_dvs.push_back(dvs[idx]);
        res_fvs.push_back(fvs[idx]);
    }

    return result;
}


std::string select_random::get_extra_info() const
{
    return "Max selection rate: " + std::to_string(m_rate);
}


PolyBeeEvolve::PolyBeeEvolve(PolyBeeCore& core) : m_masterPolyBeeCore(core)
{}


void PolyBeeEvolve::evolve()
{
    if (Params::numIslands <= 1) {
        evolveSinglePop();
    } else {
        evolveArchipelago();
    }
}


PolyBeeCore& PolyBeeEvolve::polyBeeCore(std::size_t islandNum)
{
    if (islandNum == 0) {
        return m_masterPolyBeeCore;
    } else {
        // NB Indices in this vector are offset by 1 compared to island number (island 0 is the master PolyBeeCore)!
        assert(islandNum - 1 < m_islandPolyBeeCores.size());
        return *m_islandPolyBeeCores[islandNum - 1];
    }
}


void PolyBeeEvolve::evolveSinglePop() {
    // 1 - Instantiate a pagmo problem constructing it from a UDP
    // (user defined problem).
    pagmo::problem prob{PolyBeeOptimization{this, Params::evolveSpec}};

    // 2 - Instantiate a pagmo algorithm
    // For info on available algorithms see: https://esa.github.io/pagmo2/overview.html
    int numGensForAlgo = Params::numGenerations - 1; // -1 because Pagmo doesn't count the initial population evaluation as a generation
    //pagmo::algorithm algo{pagmo::de1220(numGensForAlgo)}; // not suitable for stochastic problems
    //pagmo::algorithm algo{pagmo::sade(numGensForAlgo)};   // not suitable for stochastic problems
    //pagmo::algorithm algo {pagmo::gaco(numGensForAlgo)};
    //pagmo::algorithm algo {pagmo::pso_gen(numGensForAlgo)};
    pagmo::algorithm algo {pagmo::sga(numGensForAlgo)};
    //pagmo::algorithm algo {pagmo::cmaes(numGensForAlgo)}; // produces an error (tries to use side=5) at end of first generation
    //pagmo::algorithm algo {pagmo::xnes(numGensForAlgo)};  // produces an error (tries to use side=5) at end of first generation

    // ensure that the Pagmo RNG seed is determined by our own RNG, so runs can be reproduced
    // by just ensuring we use the same seed for our own RNG
    unsigned int algo_seed = static_cast<unsigned int>(m_masterPolyBeeCore.m_uniformIntDistrib(m_masterPolyBeeCore.m_rngEngine));
    algo.set_seed(algo_seed);

    // 3 - Instantiate a population
    // (again, taking care to seed the population RNG from our own RNG)
    unsigned int pop_seed = static_cast<unsigned int>(m_masterPolyBeeCore.m_uniformIntDistrib(m_masterPolyBeeCore.m_rngEngine));

    pagmo::population pop{prob, static_cast<unsigned int>(Params::numConfigsPerGen), pop_seed};

    // 4 - Evolve the population
    pop = algo.evolve(pop);

    // 5 - Output the population
    writeResultsFile(algo, pop, true);
}


void PolyBeeEvolve::evolveArchipelago()
{
    assert(Params::numIslands > 0);
    assert(Params::migrationPeriod > 0);

    // 1. Create an archipelago with multiple islands
    pagmo::archipelago arc;

    // 2. Set up topology for migration (how islands are connected)
    // Using a ring topology: each island connects to its neighbors
    //
    // Topology options are:
    // * Unconnected
    // * Fully connected
    // * Base BGL (Boost Graph Libary) - requires further implementation
    // * Ring
    // * Free-form
    arc.set_topology(pagmo::topology{pagmo::ring{}});
    //
    // Migration type options are:
    // * p2p
    // * broadcast
    arc.set_migration_type(pagmo::migration_type::p2p);
    //
    // Migration handing options are:
    // * preserve (a single migrant can get copied to multiple other islands)
    // * evict    (a single migrant can only get copied to one different island)
    arc.set_migrant_handling(pagmo::migrant_handling::evict);

    // 3. Create islands and add them to the archipelago
    for (size_t i = 0; i < Params::numIslands; ++i)
    {
        if (i > 0) {
            // Create a copy of the master PolyBeeCore for this island
            // First, create a unique seed string for this island, but derived from the master RNG seed string
            std::string islandSeedStr = Params::strRngSeed + std::to_string(i);
            // NB Indices in this vector are offset by 1 compared to island number (island 0 is the master PolyBeeCore) -
            // this is handled by the polyBeeCore() method
            m_islandPolyBeeCores.push_back(std::make_unique<PolyBeeCore>(m_masterPolyBeeCore, islandSeedStr));
        }

        // 3a - Instantiate a pagmo problem constructing it from a UDP (user defined problem).
        pagmo::problem prob{PolyBeeOptimization{this, Params::evolveSpec, i}};

        // 3b - Instantiate a pagmo algorithm
        pagmo::algorithm algo;

        if (!Params::useDiverseAlgorithms) {
            algo = pagmo::algorithm{ pagmo::sga(1) }; // we will be evolving one generation at a time in the main loop below
        }
        else {
            switch (i % 3) {
            case 0:
                algo = pagmo::algorithm{ pagmo::sga(1) }; // we will be evolving one generation at a time in the main loop below
                break;
            case 1:
                algo = pagmo::algorithm{ pagmo::pso_gen(1) };
                break;
            case 2:
                algo = pagmo::algorithm{ pagmo::gaco(1, static_cast<unsigned int>(Params::numConfigsPerGen / 5)) }; // smaller population for gaco
                break;
            default:
                pb::msg_error_and_exit("PolyBeeEvolve::evolveArchipelago - unexpected algorithm selection case");
            }
        }

        // ensure that the Pagmo RNG seed is determined by our own RNG, so runs can be reproduced
        // by just ensuring we use the same seed for our own RNG
        unsigned int algo_seed = static_cast<unsigned int>(m_masterPolyBeeCore.m_uniformIntDistrib(m_masterPolyBeeCore.m_rngEngine));
        algo.set_seed(algo_seed);

        // 3c - Instantiate a population
        // (again, taking care to seed the population RNG from our own RNG)
        unsigned int pop_seed = static_cast<unsigned int>(m_masterPolyBeeCore.m_uniformIntDistrib(m_masterPolyBeeCore.m_rngEngine));

        // Note - when we create the population in the following line, an initial round of fitness evaluations
        // will be performed for all individuals in the population. We'll refer to this as generation 0
        pagmo::population pop{prob, static_cast<unsigned int>(Params::numConfigsPerGen), pop_seed};

        // 3d - Add the island to the archipelago
        //
        // Selection policy options are:
        // * Best (pagmo::select_best) - selects best individuals for migration
        // * Random (select_random) - selects random individuals for migration regardless of fitness
        //
        // Replacement policy options are:
        // * Fair (pagmo::fair_replace) - replaces worst individuals only if migrant is better
        // * Random (replace_random) - replaces randomly selected individuals regardless of fitness
        arc.push_back(pagmo::island{algo,
            pop,
            replace_random{m_masterPolyBeeCore, static_cast<pagmo::pop_size_t>(Params::migrationNumReplace)}, // randomly selected individuals can be replaced by migrants
            select_random{m_masterPolyBeeCore, static_cast<pagmo::pop_size_t>(Params::migrationNumSelect)}   // randomly selected individuals can be selected for migration
        });
    }

    // Print connections for ring
    if (!Params::bCommandLineQuiet) {
        std::string msg = "Topology info:\n";
        msg += std::format(" type = {}\n",arc.get_topology().get_name());
        //
        for (size_t i = 0; i < arc.size(); ++i) {
            auto weights_and_destinations = arc.get_topology().get_connections(i);
            msg += std::format("Island {} [using alg: {}] connects to:\n", i, arc[i].get_algorithm().get_name());
            size_t num_connections = weights_and_destinations.first.size();
            for (size_t j = 0; j < num_connections; ++j) {
                msg += std::format(" Island {} (weight {}) ", weights_and_destinations.first[j], weights_and_destinations.second[j]);
            }
            msg += "\n";
        }
        //
        pb::msg_info(msg);
    }

    // 4 - Evolve the archipelago
    int numCycles = Params::numGenerations / Params::migrationPeriod;
    int extraGens = Params::numGenerations - (numCycles * Params::migrationPeriod);
    int numGensBetweenMigrations = Params::migrationPeriod - 1;

    if (extraGens > 0) {
        numCycles += 1;
    }

    int globalGen = 1; // start at generation 1 (generation 0 is the initial population evaluation)
    bool allDone = false;
    std::size_t firstNewMigration = 0;

    for (int cycle = 0; cycle < numCycles && !allDone; ++cycle) {
        pb::msg_info(std::format("Achipelago evolution cycle {}", cycle + 1));

        // Phase 1: Local evolution (no migration)
        pb::msg_info(std::format("  Running {} local generations...", numGensBetweenMigrations));
        for (int localGen = 0; localGen < numGensBetweenMigrations; ++localGen) {
            for (size_t i = 0; i < arc.size(); ++i) {
                pb::msg_info(std::format("    Initiating generation {} on island {}...", globalGen, i));
                arc[i].evolve();
            }
            for (size_t i = 0; i < arc.size(); ++i) {
                arc[i].wait_check();
            }

            showBestIndividuals(arc, globalGen);

            if (++globalGen >= Params::numGenerations) {
                allDone = true;
                break;
            }
        }

        // Phase 2: Migration event
        if (!allDone) {
            pb::msg_info("  Performing a generation with migration...");
            arc.evolve();
            arc.wait_check();
            showBestIndividuals(arc, globalGen);
            ++globalGen;

            pb::msg_info("Migration stats:");
            auto log = arc.get_migration_log();
            std::size_t num_entries = log.size();

            for (std::size_t entry_idx = firstNewMigration; entry_idx < num_entries; ++entry_idx) {
                auto [ts, id, dv, fv, source, dest] = log[entry_idx];
                double median_fitness = pb::median(fv);
                pb::msg_info(std::format("  At time {:.2f} individual {} (median fitness {:.5f}) migrated from Island {} -> {}",
                    ts, id, median_fitness, source, dest));
            }
            firstNewMigration = num_entries;
        }
    }

    // 5 - Output the population
    writeResultsFileArchipelago(arc, false);
}


void PolyBeeEvolve::showBestIndividuals(const pagmo::archipelago& arc, int gen) const
{
    double best_f = std::numeric_limits<double>::max();
    size_t best_i = 0;

    pb::msg_info(std::format("Generation {} best individuals:", gen));

    for (size_t i = 0; i < arc.size(); ++i)
    {
        const auto* pPBO = arc[i].get_population().get_problem().extract<PolyBeeOptimization>();
        assert(pPBO != nullptr);

        double island_best = arc[i].get_population().champion_f()[0];
        pb::msg_info(std::format("  Best fitness for island {}: {:.5f}", i, island_best));

        auto champ = arc[i].get_population().champion_x();
        std::string best_indiv;
        for (size_t i = 0; i < champ.size(); ++i) {
            if (i > 0) { best_indiv += ", "; }
            best_indiv += (i < pPBO->m_numFloatVars) ? std::format("{:.5f}", champ[i]) : std::format("{}", champ[i]);
        }
        pb::msg_info(std::format("  Best individual for island {}: {}", i, best_indiv));

        if (island_best < best_f) {
            best_f = island_best;
            best_i = i;
            champ = arc[i].get_population().champion_x();
        }
    }

    pb::msg_info(std::format("  Overall best fitness: {:.5f} (island {})", best_f, best_i));
}


void PolyBeeEvolve::writeResultsFileArchipelago(const pagmo::archipelago& arc, bool alsoToStdout) const
{
    // write evolution results to file
    std::string resultsFilename = std::format("{0}/{1}evo-results-{2}.txt",
        Params::logDir,
        Params::logFilenamePrefix.empty() ? "" : (Params::logFilenamePrefix + "-"),
        m_masterPolyBeeCore.getTimestampStr());

    std::ofstream resultsFile(resultsFilename);

    if (!resultsFile) {
        if (!resultsFile) {
            pb::msg_warning(
                std::format("Unable to open evol-results output file {} for writing. Results will not be saved to file, printing to stdout instead.",
                    resultsFilename));
        }
        std::cout << "~~~~~~~~~~ EVOLUTION RESULTS ~~~~~~~~~~" << std::endl;
        writeResultsFileArchipelagoHelper(std::cout, arc);
    }
    else {
        writeResultsFileArchipelagoHelper(resultsFile, arc);
        resultsFile.close();
        pb::msg_info(std::format("Evolution results written to file: {}", resultsFilename));
        if (alsoToStdout) {
            std::cout << "~~~~~~~~~~ EVOLUTION RESULTS ~~~~~~~~~~" << std::endl;
            writeResultsFileArchipelagoHelper(std::cout, arc);
        }
    }
}


// Helper method to write results to a given output stream
void PolyBeeEvolve::writeResultsFileArchipelagoHelper(std::ostream& os, const pagmo::archipelago& arc) const
{
    double best_champ_fitness = std::numeric_limits<double>::max();
    pagmo::vector_double best_champ;

    for (std::size_t i = 0; i < arc.size(); ++i) {
        os << "\n*** Island " << i << " ***" << std::endl;
        auto algo = arc[i].get_algorithm();
        auto pop = arc[i].get_population();

        os << "Using algorithm: " << algo.get_name() << std::endl;
        os << "The population: \n" << pop;
        os << "\nIsland " << i << " champion individual: ";
        auto island_champ = pop.champion_x();
        auto island_champ_fitness = pop.champion_f()[0];
        if (island_champ_fitness < best_champ_fitness) {
            best_champ_fitness = island_champ_fitness;
            best_champ = island_champ;
        }
        for (size_t i = 0; i < island_champ.size(); ++i) {
            if (i > 0) { os << ", "; }
            os << island_champ[i];
        }
        os << "\n";
        os << "Island " << i << " champion fitness: " << island_champ_fitness << std::endl;
    }

    os << "\n~~~~~~~~~~ Overall Results ~~~~~~~~~~" << std::endl;
    os << "Overall best champion individual: ";
    for (size_t i = 0; i < best_champ.size(); ++i) {
        if (i > 0) { os << ", "; }
        os << best_champ[i];
    }
    os << "\nOverall best champion fitness: " << best_champ_fitness << std::endl;
}


void PolyBeeEvolve::writeResultsFile(const pagmo::algorithm& algo, const pagmo::population& pop, bool alsoToStdout) const
{
    // write evolution results to file
    std::string resultsFilename = std::format("{0}/{1}evo-results-{2}.txt",
        Params::logDir,
        Params::logFilenamePrefix.empty() ? "" : (Params::logFilenamePrefix + "-"),
        m_masterPolyBeeCore.getTimestampStr());

    std::ofstream resultsFile(resultsFilename);

    if (!resultsFile) {
        if (!resultsFile) {
            pb::msg_warning(
                std::format("Unable to open evol-results output file {} for writing. Results will not be saved to file, printing to stdout instead.",
                    resultsFilename));
        }
        std::cout << "~~~~~~~~~~ EVOLUTION RESULTS ~~~~~~~~~~" << std::endl;
        writeResultsFileHelper(std::cout, algo, pop);
    }
    else {
        writeResultsFileHelper(resultsFile, algo, pop);
        resultsFile.close();
        pb::msg_info(std::format("Evolution results written to file: {}", resultsFilename));
        if (alsoToStdout) {
            std::cout << "~~~~~~~~~~ EVOLUTION RESULTS ~~~~~~~~~~" << std::endl;
            writeResultsFileHelper(std::cout, algo, pop);
        }
    }
}


// Helper method to write results to a given output stream
void PolyBeeEvolve::writeResultsFileHelper(std::ostream& os, const pagmo::algorithm& algo, const pagmo::population& pop) const
{
    os << "Using algorithm: " << algo.get_name() << std::endl;
    os << "The population: \n" << pop;
    os << "\n";
    os << "Champion individual: ";
    auto champ = pop.champion_x();
    for (size_t i = 0; i < champ.size(); ++i) {
        if (i > 0) { os << ", "; }
        os << champ[i];
    }
    os << "\n";
    os << "Champion fitness: " << pop.champion_f()[0] << std::endl;
}
