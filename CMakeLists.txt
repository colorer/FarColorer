cmake_minimum_required(VERSION 3.15)

#====================================================
# Enable policy
#====================================================
# enable CMAKE_MSVC_RUNTIME_LIBRARY
cmake_policy(SET CMP0091 NEW)

#====================================================
project(farcolorer CXX)

#====================================================
# Set default build to release
#====================================================
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type, one of: Release, Debug" FORCE)
endif ()
message("Build type for FarColorer: ${CMAKE_BUILD_TYPE}")

#====================================================
# Set configuration types
#====================================================
if (NOT MSVC_IDE)
  set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
else ()
  #target_compile_options cannot set parameters for all configurations
  set(CMAKE_CONFIGURATION_TYPES "${CMAKE_BUILD_TYPE}" CACHE STRING "" FORCE)
endif ()
message("FarColorer configurations for IDE: ${CMAKE_CONFIGURATION_TYPES}")

#====================================================
# global settings
#====================================================
if (MSVC)
  # set global Visual C++ runtime
  if (CMAKE_BUILD_TYPE MATCHES Debug)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
  else ()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
  endif ()

  # support winxp
  if (${CMAKE_CL_64} STREQUAL "0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:IA32 /Zc:threadSafeInit-")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:IA32 /Zc:threadSafeInit-")

    # definition for spdlog in winxp
    add_definitions(-D_WIN32_WINNT=0x0500)
  endif ()
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
