cmake_minimum_required(VERSION 3.24)
project(polybee)

# Set the C++ standard (we use C++20 features such as std::format)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS Off)

# Set a build type if none is specified
## Set a default build type. You can override this on the command line as follows:
##  cmake -D CMAKE_BUILD_TYPE=Release ..
## Avaiable build types are [Debug|Release|MinSizeRel|RelWithDebInfo]
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif(NOT CMAKE_BUILD_TYPE)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Dependencies
include(FetchContent)

## Raylib
FetchContent_Declare(
    raylib
    GIT_REPOSITORY "https://github.com/raysan5/raylib.git"
    GIT_TAG c1ab645ca298a2801097931d1079b10ff7eb9df8 # release-5.5
    # Alternatively you can specify a tag, but these can be changed so a commit hash is safer!
    # GIT_TAG 5.5
    GIT_PROGRESS TRUE # force Git to output progress information during the clone step
)

set(BUILD_EXAMPLES OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(raylib)

# Problems with oneTBB and CMake FetchContent - try installing oneTBB manually if needed?
## oneTBB (required by Pagmo)
#FetchContent_Declare(
#    oneTBB
#    GIT_REPOSITORY "https://github.com/oneapi-src/oneTBB.git"
#    GIT_TAG "v2022.2.0" # stable release as of 2023-09-15
#    GIT_PROGRESS TRUE
#)

# Configure oneTBB to build as dynamic library
#set(TBB_BUILD_SHARED ON CACHE INTERNAL "")
#set(TBB_BUILD_STATIC OFF CACHE INTERNAL "")
#FetchContent_MakeAvailable(oneTBB)


## Boost (required by Pagmo and also for command line argument parsing)
if(POLICY CMP0144) # See https://forum.qt.io/topic/158929/install-boost-with-cmake
  cmake_policy(SET CMP0144 NEW)
endif()

find_package(Boost 1.83 REQUIRED COMPONENTS program_options serialization)

# Pagmo (for optimization)
FetchContent_Declare(
    pagmo
    GIT_REPOSITORY "https://github.com/esa/pagmo2.git"
    GIT_TAG "v2.19.1" # stable release as of 2023-09-15
    GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(pagmo)

## LEMON library for graph algorithms (for full EMD)
# Using local modified version with C++20 compatibility fixes
add_subdirectory(3rdparty/lemon-a278d16bd2d0)

# Our Project
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/bin/debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/release)

# Debug symbols and optimization settings
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

set(SOURCE_FILES
	src/main.cpp
    src/PolyBeeCore.cpp
    src/PolyBeeEvolve.cpp
    src/Bee.cpp
    src/Environment.cpp
    src/Params.cpp
    src/Heatmap.cpp
    src/LocalVis.cpp
    src/utils.cpp)


# add the executable
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/3rdparty/lemon-a278d16bd2d0"
    "${CMAKE_BINARY_DIR}/3rdparty/lemon-a278d16bd2d0"
)

# link libraries
#target_link_libraries(${PROJECT_NAME} raylib Boost::program_options lemon pagmo tbb)
target_link_libraries(${PROJECT_NAME} raylib Boost::program_options lemon pagmo)

# Checks if OSX and links appropriate frameworks (Only required on MacOS)
if (APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()
