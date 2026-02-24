## 24/2/26

Crossing net code now debugged and working correctly.

## 23/2/26

Have finished coding the new implementation of TryingToCrossEntranceState, Bee::continueTryingToCrossEntrance(), etc.
Now need to debug it.

## 18/2/26

Have ditched previous approach becayse it was too complicated
Now working on a simpler new implementation with a reduced TryingToCrossEntranceState

## 17/2/26

In process of implementing revised code for Bee bounce behaviour. Have updated TryingToCrossEntranceState
with new/replaced data members. IntersectInfo now has intersectedWall member as well as intersectedLine

TODO

* Complete implementation of void TryingToCrossEntranceState::set() in Bee.cpp
* Complete implementaion of void TryingToCrossEntranceState::update() in Bee.cpp
* Complete revision of void Bee::continueTryingToCrossEntrance()

## 16/2/26

DONE

* when evol run finished, using the champion config, re-run the net-expts to get a bar chart showing effect of nets when we use visit count objective rather then EMD -- for non-evol runs, will need to output the final visit count fraction in the run-info log file

* Add fraction successfully visited measure to visualiser output

* Add help screen to visualiser

## 11/2/26

DONE

* need to update/rename the plot_emd_scores_islands.py script to plot_fitness_islands.py, with a command line
flag to indicate which objective function is being used - this will determine the labels on the graph
(EMD vs Fraction Successful Visits)

* fixed - when running with evolve-objective 1 specified on command line, looks like the internal evolveObjective
param is not getting set correctly - it seems to be 0

## 4/2/26

DONE

* check that the "new trying to cross entrance" code is working properly, e.g. in Bee::continueTryingToCrossEntrance()
  Debugging and some refactoring required
* refactoring of Bee class to use pb::Pos2D for bee position rather than two floats
* refactor Hive class to use pb::Pos2D for bee position rather than two floats
* add comments to explain how default values of net crossing params have been derived

## 3/2/26

DONE

* have calculated "per attempt" probabilities of exiting bird and hail nets
* finished coding code in Bee class to have it in a "trying to cross entrance" pseudo-state
when foraging

## 2/2/26

DONE

* Implemented and testing net transit probability code in Bee::forage and associated methods
* ALso added net transit probability logic to the waypoint code when bees return to hive

## 30/1/26
Added new param useDiverseAlgorithms for use in PolyBeeEvolve.

Implementing net exit probablilities based upon net type, and new optional specification of net type in input parameters.

TODO:
* new exit prob parameters are added, but rest of code now has to make use of them specifically, we need to consider this in Bee::forage() - see the TODO comment in that method

## 27/1/26
Zoom call with Alan and Hazel. Planning extension of code to use antibird and antihail net parameters as found in
Sonter, Rader et al paper ("Protective covers impact honey bee colong performance...")

## 26/1/26

TODO:

Add parameters to allow user to select which algorithms run on which islands

DONE:

Implemented different algorithms on different islands and ran experiments (do-island-test-4 and -4b)

## 21/1/26

DONE:

* do-evol-with-energetics used 100 bees, 150 trials per config, 100 configs per gen, 50 gens
* try a test of island model with 100 bees, 150 trials per config, 50 configs per island, 5 islands, 10 migration period, 40 gens (island-model-test-3)

## 20/1/26

More work and improvements on Island model. Added new random selection and replacement code.
Also improvements on output data during an Island model run.

## 14/1/26

TODO:

DONE:

* Improve output of run. Show migrations at each migration step rather than
  all at end. Also, be sure to output full details of champion vector
  at end (and best from each island). Do this periodically throughout
  run as well.

* Fixed problems with Tunnel::initialiseEntrances in fitness method. This now
  creates a local specs object rather than using Params, for thread safety.

## 13/1/26

TODO:

The archipelago/island code is now implemented and compiling, but some problems
are encountered when running the code (e.g. using the test script do-island-test).
Need to look into this. (Now done)

DONE:

* now have to change PolyBeeEvolve constructor and evolveArchipelago so that PBE creates
  and stores a new PBC object for each island (using the new PBC copy constructor to create
  all but the first one - Island 0 can use the original PBC)

## 12/1/26

In process of implementing Island model. Have to think carefully about thread safety.
Currently we initialise a single PolyBeeCore and PolyBeeEvolve object in main.cpp
and PolyBeeHeatmapOptimization has a pointer to the sole PolyBeeEvolve object.

Perhaps PolyBeeEvolve should make a new copy of PolyBeeCore for each Island?

Also be careful of static members in classes. Okay if these are const, but not if they
are counters [e.g. PolyBeeEvolve::eval_counter] (or RNG-related?). Check through
what statics there are and how used.

Re RNGs, see https://stackoverflow.com/questions/21237905/how-do-i-generate-thread-safe-uniform-random-numbers
- so create one generator per thread (which is consistent with having one PolyBeeCore object per thread)
- careful about the seed - don't want all cores using the same one!
  - so, create each core with a seed derived from the master seed defined by the parameters

## 8/1/26

TODO:

* Continue implementing island model
  - Implement migration every n generaions (see Claude example code on web)
  - Do a test run and compare results to previous ones
* Get defendable real-world values for amount of nectar stored by flower, given to bee, used by bee, etc.
* Also get defendable values of tunnel dimensions, patch dimensions, patch density, bee visual radius, bee step length, etc.

* Do Alan's suggested sanity check (see notepad notes from 9/12/25) of running 2 diff configs on the same env (a square env) to see how much difference there really is - i.e. how much of results is driven by crop following behaviour - try with (A)
entrances all at bottom vs (B) entrances all at one side

## 7/1/26

DONE:

* switch evolve code to use median EMD value rather than mean

* For consistency, do the following renaming:
bee-num-steps-on-flower -> bee-on-flower-duration
beeNumStepsOnFlower -> beeOnFlowerDuration

## 10/12/25

DONE:

* Run SGA using new energetics - meld params in evolve-entrance-positions-4-rows.cfg and test-bee-energetics-1.cfg
- might be handy to re-run tests of best numBees and numRepeatsPerConfig.
Does this improve the results? (see Alan's comments in email of 4/12 - see entry below for 8/12)

## 8/12/25

Email from Alan on 4/12:

> …in *our* current model, nothing is actually “attracting” the bees to prefer a region of food to one where there is no food except the type of hops they make, right? I.e. we aren't actually “exhausting" our bees who find no food and “replenishing” the energy levels of bees who do find food. That would drive bees lost outside back to the hive earlier than bees finding nectar while they are outside the hive. It would also drive bees exploring areas already explored by other bees back to the hive earlier than bees in new areas with lots of nectar. Implementing this would drastically swing the behaviour away from areas of no food into areas where there is food. Right? Should we try this next, before building more simulations?

My reply on 5/12:

> That sounds good, Alan. You're right, at the moment there is nothing about nectar or energy levels in the simulation.  I'll start designing and coding this on Monday, and we can talk more about the details in our next zoom call (on Tuesday if that works for you?).

So, need to give flowers nectar, and give bees an energy level - refilled when they leave hive. If bees have N successive landings without getting a nectar reward, they do a Levy flight to somewhere else. When bee has collected X amount of nectar, returns to hive. If a bee's energy level is below a threshold N it returns to hive.
- Add a feeding state when bee is on flower - remains their for T timesteps
- Possibly have Levy flight instead of random if no target found?

## 26/11/25

Maybe change PolyBeeEvolve fitness code to use median EMD for a config rather than mean EMD

Have added target heatmap to Environment so this can be loaded for normal (non evolve) runs.
Can now remove target heatmap code from PolyBeeEvolve and use the code in Environment instead.
Next up, check new code is working, and write final EMD between Env m_heatmap and m_targetHeatmap
to run info file at end of run.

SGA algorithm seems to be the best. Next step is to try this with Island / Archipelago features with
migration to see if this maintains diversity for longer and results in better final solutions.

Modify python script so it takes a second command line arg that is added to the title of the chart

Also try evolving with target heatmap even across all of inside of tunnel

Then try allowing bees to exit from nearest visible exit if it gives a shorter route back to hive
than the entrance it used first.

## 25/11/25

The plot of the results from the 3rd run (out3.png) shows that very little evolution is occuring in the minimum EMD per generation over time.  Try looking at some other algorithms in Pagmo to see if they perform any better and/or look at implementing an Island model (or is the Island/archipelego stuff more for running distributed processes rather than for maintaining diversity per se? Perhaps we just need to find an algorithm that does this?? Look into this some more)

## 20/11/25

Next up:

* Try doing some optimisation now the return to hive code is complete
* Change bee's trail colours to show what state they are in?

## 19/11/25

Done the following:

* When following waypoints keep bee trail data up to date
* Debug waypoints

## 17/11/2025

First attempt at a return to hive algorithm:

* when a bee is foraging, it RECORDS which entrance it enters the tunnel by
* when it stops foraging to return to hive:
  * if outside tunnel, calc waypoints to go around tunnel and return to hive (RETURN_TO_HIVE_OUTSIDE_TUNNEL)
  * if inside tunnel (RETURN_TO_HIVE_INSIDE_TUNNEL)
    * first, return directly to same entrace/exit and exit tunnel
    * then follow "outside tunnel" behaviour as above

## 5/11/2025

TO DO
* implement Bee:returnToHive()
* Show bees in different colour (or in outline only?) when returning to hive

DONE
* Implemented Bee::stayInHive()
* Changed Bee::move() to Bee::forage() and added new master iteration method Bee::update() that gets called by the environment at each iteration.
* In LocalVis add toggle to show/hide plants and patches - actually just changed code so shows outline of patches only, and no plants, if the heatmap is being dispayed

## 4/11/2025

Debugging forage nearest flower

DONE
* Bee forageNearestPlant - should draw up list of all flowers more or less the shortest distance away, then
  pick one of these at random -> now implemented as a prob that defines whether nearest plant will be returned.
  If not, a plant is picked at random of list of all visible plants

* Environment m_plantGrid is currently same dimension as env, so each cell is 1x1 and bee searches 3x3 cells,
  so won't see any neighbouring plants if they are spaced at 10cm intervals - need to fix this!! - this should
  be set according to the bee-visual-range and/or bee-step-length params

* In Bee::forageNearestPlant - was moving beeStepLength distance even if flower was closer than that -
  have now fixed so if plant is closer, we just move directly to it and do not overshoot

## 28/10/25

Next up:
* implement FNF/random walk (also add state to bee? foraging, returning to hive, in hive)
* add jitter to plant position in Environment::initialisePlants()
* when displaying heatmap, maybe just show outlines of patches (don't show filled in rect or individual plants)
* (also add help screen that lists all key and mouse controls)

Done:
* add ability to pan as well as zoom in UI

## 27/10/25

Specification of entrances in tunnel, and collision detection with tunnel taking into account the defined entrances, is now complete.

Next up:

When this is all done, next need to do the following:
* add plants/flowers (have flexible spec of these in config file, inc multiple species, flexible layout, etc)

## 21/10/25

In process of implementing proper collision testing of bees with tunnel borders, taking into account entraces to tunnel. This is work in the Environment, Tunnel and Bee classes. Have borrowed a intersectRayRectangle method from the web, and now need to integrate and test this. Also (maybe) need to ensure that bees outside the tunnel do not get within tunnel-thickness of the tunnel when moving.

## 20/10/25

Working on major refactoring of code to properly deal with tunnels and possibility of environment space outside of tunnels. Probably best to move m_bees from PolyBeeCore to Environment class.

Will need a new Tunnel class (and Environment has-a tunnel), and ability to specify entries/exits in the tunnel sides

Heatmap applies to interior of tunnel, not to whole environment?? - ask Alan

Will have to add collision detection code for bees colliding with sides of tunnel inside the environment

Add helper methods in LocalVis to convert between display, environment, and tunnel coordinate frames

## 15/10/25

Done:
* Log config info, pulling info from polybeeConfig.h and opencv/version.hpp
* Calculate high EMD value using Heatmap antiTargetNormalised and print value in evolution output and on screen

Next steps:
* add step-length as a parameter
* try evolving step length with target being a dot in middle -> should evolve towards zero step length
* Then start adding complexity to optimisation runs (as discussed with Alan and Hazel)

## 14/10/25

Have now implemented OpenCV EMD measure.  Tidy up code by just having one point of entry Heatmap::emd() method, which calls the implementation we want. Move all implementations to the Heatmap class and out of utils.cpp.

Next, have Heatmap class calculate a ceiling for EMD at start of run by measuring distance between having all bees in one square (in corner) vs a uniform spread. Print this in output somewhere.

## 1/10/25

Have done evolutionary runs using EMDhat, and also with also evolving numBees as well as DirDelta. Look at charts on Google Sheet. So optimization looks like it's more or less working, but these runs probably don't give the bees enough time to get a fully uniform distribution, and the scope of changes the system can make (just 1 or 2 params) means that some lucky individuals even in the first 1 or 2 gens perform very well, so don't see a great deal of improvement over time.

So next step is maybe introduce some more complicated scenarios and/or allow evolution to adjust a greater set of parameters (e..g. could also evolve hive positions)


## 30/9/25

To do: do runs with diff emd, also optimise num bees as well as max delta?

Have checked we get the same results when rerunning evolution with the same seed - we do!

Look at results of evolutionary runs with DE 1220: https://docs.google.com/spreadsheets/d/1Moa9Zsc2-9B20Mie0D0M0mP5gDugH_b7p-nTQsh-mm8/edit?gid=591442890#gid=591442890

Have tried SADE instead of DE 1220 - results were a little worse

---

For evaluating a given config, we need to run the sim a certain number of times, and derive an overall fitness value from the collected results

## 29/9/25:

Working on implementation of optimization algorithm. Look at code labelled TODO in PolyBeeEvolve.cpp and PolyBeeCore.cpp for where to continue working.

Have just installed libtbb-dev package and it all compiles now. If we end up not using the Lemon implementation of EMD then we can remove Lemon from the CMakeLists.txt


## 24/9/25:

In process of trying to build polybee with pagmo library, which requires the oneTBB library (and Boost). I can get the oneTBB lib to compile, but Pagmo does not find it when it tries to compile. Maybe simpler just to build oneTBB separately?? Look more into this...

So **Boost** has an implementation of **Differential Evolution**:
https://www.boost.org/doc/libs/1_89_0_beta1/libs/math/doc/html/math_toolkit/differential_evolution.html

Examples of usage:
- basic: https://www.boost.org/doc/libs/1_89_0_beta1/libs/math/example/differential_evolution.cpp
- more complex: https://www.boost.org/doc/libs/1_89_0_beta1/libs/math/test/differential_evolution_test.cpp

NB how does it handle RNG seeding etc> -> Answer, it's the third param passed into the diff evol fn

As an interesting alternative, see the **PAGMO library**, which has an academic background and has been updated frequently (most recently in 2024), plus support for parallel processing
https://esa.github.io/pagmo2/index.html
https://esa.github.io/pagmo2/docs/cpp/algorithms/de.html

Other possibilities:
* https://www.alglib.net/ (free + commerical versions; free is single-threaded, no commercial use)
* https://www.amichel.com/de/doc/html/dd/d53/overview.html / https://github.com/adrianmichel/differential-evolution (last updated 5 years ago)

If needed, there is also a C++ optimisation package called **OSTRICH** that implements **Shuffled Complex Evolution**:
https://www.civil.uwaterloo.ca/envmodelling/Ostrich.html


## 23/9/25:

Have now implemented emd_hat and it's working (unlike the LEMON version). Now need to convert all heatmap-related matrices from floats to doubles so we don't have to create double versions every time we want to calculate emd_hat. This will entail converting the other emd functions too.

Would be good at somepoint to profile the speed of emd_hat vs the other implementations, and the investigate the numbers they produce for different target heatmaps

Have also implemented pause in the visualiser

Next up, it would be handy to implement another option (4) for hive config, which means bees can exit in any direction



## 17/9/25:

Added a modified version of the LEMON library under the 3rdparty folder. This has been modified to run with C++20 as its original code used some deprecated and removed features of the language. Also added Pele's FastEMD code in the include file lemon_thresholded_emd.hpp, and a new function in utils.cpp earthMoversDistanceLemon that uses it.

Next step is to actually use the earthMoversDistanceLemon functon to calculate the distance between two heatmaps, and then to implement an optimisation method (evolutionary algorithm) to optimise simulation param(s) based on a target heatmap.

Could also compare against the other implementations of EMD in utils.h, but probably best from a scientific perspective to stick with the one provided by Pele.


## 16/9/25:

Looking for a reliable and fast implementation of **Earth Mover's Distance**

Best approach seems to be the following:

Use **Pele's new C++ implementation that uses the Lemon library**:
https://github.com/ofirpele/FastEMD/tree/master
-> file lemon_thresholded_emd.hpp

Then need to install the Lemon library
https://lemon.cs.elte.hu/trac/lemon/wiki/Downloads

Most portable way of doing the above is likely to have them both as source code in my project, under a 3rdparty dir in the repo

Chat GPT says:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The LEMON graph/network library
 is a C++ template library, so it doesn’t need linking to a compiled .so/.a unless you specifically install it system-wide. Most of the time you just need its headers. How you specify it in CMake depends on how you installed it.

Option 1: Use system-installed LEMON

If you’ve installed LEMON with apt install (on Debian/Ubuntu: sudo apt install liblemon-dev) or built it with make install, then CMake can usually find it with its lemon.pc pkg-config file.

```
cmake_minimum_required(VERSION 3.10)
project(MyProject CXX)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LEMON REQUIRED lemon)

add_executable(myprog main.cpp)

target_include_directories(myprog PRIVATE ${LEMON_INCLUDE_DIRS})
target_link_libraries(myprog PRIVATE ${LEMON_LIBRARIES})
```

That’s the cleanest way if you have system packages.

Option 2: Use CMake’s find_package (if available)

LEMON also provides a LEMONConfig.cmake if you built and installed it from source with CMake. Then you can do:

```
cmake_minimum_required(VERSION 3.10)
project(MyProject CXX)

find_package(LEMON REQUIRED)

add_executable(myprog main.cpp)
target_link_libraries(myprog PRIVATE LEMON::lemon)
```

This works if LEMONConfig.cmake is installed under /usr/local/lib/cmake/lemon or similar.

Option 3: Vendor it into your project (header-only use)

If you don’t want to install it system-wide, just include it as a subdirectory:

Clone into your project:

```
git submodule add https://github.com/networkx/lemon.git 3rdparty/lemon
```


In your CMakeLists.txt:

```
add_subdirectory(3rdparty/lemon)

add_executable(myprog main.cpp)
target_link_libraries(myprog PRIVATE lemon)
```

This works because LEMON’s repo has its own CMake build system.

✅ Quickest/easiest if you don’t want to think too much:

On Linux: sudo apt install liblemon-dev then use Option 1.

If building portable: use Option 3 (subdirectory) so everything builds together.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

But note that the github repo that ChatGPT mentions does not exist! So will just need to pull in the latest version of Lemon from https://lemon.cs.elte.hu/trac/lemon/wiki/Downloads

- can maybe do the CMake import from URL stuff?
- or does CMake offer any help with Mercurial repos? - https://lemon.cs.elte.hu/hg/lemon
