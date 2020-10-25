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
option(COLORER_WIN64_BUILD "Build for x64, else x86" ON)
option(COLORER_WIN05_SUPPORT "Build for WinXP, Win2003" ON)

if (MSVC)
  # set global Visual C++ runtime
  if (CMAKE_BUILD_TYPE MATCHES Debug)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
  else ()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
  endif ()

  # support old AMD on winxp
  if (NOT COLORER_WIN64_BUILD)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:IA32")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:IA32")
  endif ()

  # for vc_crt_fix_impl.cpp
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:threadSafeInit-")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Zc:threadSafeInit-")

  # support winxp
  if (COLORER_WIN05_SUPPORT)
    # definition for spdlog in winxp and win2003
    add_definitions(-D_WIN32_WINNT=0x0500)
  endif ()
endif ()


#====================================================
# find dependences
#====================================================
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

#====================================================
# colorer_lib
#====================================================
set(COLORER_INTERNAL_BUILD ON CACHE BOOL "call colorer`s CMakeLists.txt")
set(COLORER_TOOLS_BUILD OFF CACHE BOOL "build colorer tool")
set(COLORER_USE_CONAN ON CACHE BOOL "Use Conan for dependencies")
if(COLORER_WIN64_BUILD)
  set(COLORER_WINx86 OFF CACHE BOOL "Set on if building for Windows x86")
else()
  set(COLORER_WINx86 ON CACHE BOOL "Set on if building for Windows x86")
endif()

add_subdirectory(./libs/colorer/)

#====================================================
# farcolorer
#====================================================
add_subdirectory(./src)
