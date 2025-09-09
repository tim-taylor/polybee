cmake_minimum_required(VERSION 3.24)
project(polybee)

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

## Boost
if(POLICY CMP0144) # See https://forum.qt.io/topic/158929/install-boost-with-cmake
  cmake_policy(SET CMP0144 NEW)
endif()

find_package(Boost 1.83 REQUIRED COMPONENTS program_options)

# Our Project
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${PROJECT_BINARY_DIR}/bin/debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/bin/release)

set(SOURCE_FILES
	src/main.cpp
    src/PolyBee.cpp
    src/Params.cpp
    src/LocalVis.cpp)

# include directories
include_directories(
    "${PROJECT_SOURCE_DIR}/include"
)

# add the executable
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# link libraries
target_link_libraries(${PROJECT_NAME} raylib Boost::program_options)

# Checks if OSX and links appropriate frameworks (Only required on MacOS)
if (APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()
