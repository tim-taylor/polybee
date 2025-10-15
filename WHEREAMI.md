## 15/10/25

To do:
* Log config info, pulling info from polybeeConfig.h and opencv/version.hpp
* Calculate high EMD value using Heatmap antiTargetNormalised and print value in evolution output and on screen
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
