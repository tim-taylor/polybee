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
