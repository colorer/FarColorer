cmake_minimum_required(VERSION 3.15)

#====================================================
# Enable policy
#====================================================
# enable CMAKE_MSVC_RUNTIME_LIBRARY
cmake_policy(SET CMP0091 NEW)

#====================================================
# vcpkg settings
#====================================================
# set before declare project(...)
set(COLORER_BUILD_ARCH x64 CACHE STRING "Build architecture")
if(NOT VCPKG_TARGET_TRIPLET)
  set(VCPKG_TARGET_TRIPLET "${COLORER_BUILD_ARCH}-windows-static-rel" CACHE STRING "vcpkg triplet")
endif()
message(STATUS "Using vcpkg triplet: ${VCPKG_TARGET_TRIPLET}")

option(FARCOLORER_LEGACY "Build to work on older platforms" ON)
if(NOT FARCOLORER_LEGACY)
  set(VCPKG_MANIFEST_DIR "${CMAKE_HOME_DIRECTORY}/manifest/full" CACHE STRING "Use own implementation for standard library")
endif()
message(STATUS "Using vcpkg manifest: ${VCPKG_MANIFEST_DIR}/vcpkg.json")

#====================================================
project(farcolorer CXX)

#====================================================
# Set default build to release
#====================================================
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type, one of: Release, Debug" FORCE)
endif()
message("Build type for FarColorer: ${CMAKE_BUILD_TYPE}")

#====================================================
# Set configuration types
#====================================================
if(NOT MSVC_IDE)
  set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
else()
  #target_compile_options cannot set parameters for all configurations
  set(CMAKE_CONFIGURATION_TYPES "${CMAKE_BUILD_TYPE}" CACHE STRING "" FORCE)
endif()
message("FarColorer configurations for IDE: ${CMAKE_CONFIGURATION_TYPES}")

#====================================================
# global settings
#====================================================

if(FARCOLORER_LEGACY)
  set(COLORER_BUILD_OLD_COMPILERS ON CACHE STRING "Use own implementation for standard library")
  set(COLORER_USE_ICU_STRINGS OFF CACHE STRING "Use ICU library for strings")
endif()

if(MSVC)
  # set global Visual C++ runtime
  if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
  else()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
  endif()

  if(FARCOLORER_LEGACY)
    # support old AMD on winxp
    if("${COLORER_BUILD_ARCH}" STREQUAL "x86")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:IA32")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:IA32")
    endif()

    # for vc_crt_fix_impl.cpp
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:threadSafeInit-")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Zc:threadSafeInit-")
  endif()

endif()

#====================================================
# find dependences
#====================================================
# core library
if(NOT FARCOLORER_LEGACY)
  find_package(ICU COMPONENTS uc data REQUIRED)
endif()

find_package(LibXml2 REQUIRED)
if(COLORER_USE_ZIPINPUTSOURCE)
  find_package(ZLIB REQUIRED)
  find_package(unofficial-minizip REQUIRED)
endif()

#====================================================
# colorer
#====================================================
set(COLORER_BUILD_TOOLS OFF CACHE BOOL "Build colorer tools")
set(COLORER_BUILD_INSTALL OFF CACHE BOOL "Make targets for install")
add_subdirectory(./external/colorer)

#====================================================
# farcolorer
#====================================================
add_subdirectory(./src)

#====================================================
# install
#====================================================
install(TARGETS farcolorer RUNTIME DESTINATION bin)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/misc/ DESTINATION bin)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/src/colorer.map
    DESTINATION bin)
install(FILES
    ${PROJECT_SOURCE_DIR}/LICENSE
    ${PROJECT_SOURCE_DIR}/README.md
    ${PROJECT_SOURCE_DIR}/docs/history.ru.txt
    DESTINATION .)

