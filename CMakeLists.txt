cmake_minimum_required(VERSION 3.15)
# enable CMAKE_MSVC_RUNTIME_LIBRARY
cmake_policy(SET CMP0091 NEW)
project (farcolorer CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type, one of: Release, Debug" FORCE)
endif()
message("Build type: ${CMAKE_BUILD_TYPE}")

if(NOT MSVC_IDE)
  set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
else()
  #target_compile_options cannot set parameters for all configurations
  set(CMAKE_CONFIGURATION_TYPES "${CMAKE_BUILD_TYPE}" CACHE STRING "" FORCE)
endif()
message("FarColorer configurations for IDE: ${CMAKE_CONFIGURATION_TYPES}")       

# set global Visual C++ runtime
if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
elseif ()
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif ()
#====================================================
# colorer_lib
#====================================================
set(COLORER_INTERNAL_BUILD ON CACHE BOOL "call colorer`s CMakeLists.txt")
set(COLORER_TOOLS_BUILD OFF CACHE BOOL "build colorer tool")
add_subdirectory(./libs/colorer/)

#====================================================
# farcolorer
#====================================================
add_subdirectory(./src)
