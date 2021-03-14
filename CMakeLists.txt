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
set(COLORER_BUILD_ARCH x64 CACHE STRING "Build architecture")

if(MSVC)
  # set global Visual C++ runtime
  if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
  else()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
  endif()
endif()

#====================================================
# find dependences
#====================================================
find_package(colorer CONFIG REQUIRED)

#====================================================
# farcolorer
#====================================================
add_subdirectory(./src)

#====================================================
# install
#====================================================
install(TARGETS colorer RUNTIME DESTINATION bin)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/misc/ DESTINATION bin)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/src/colorer.map
    DESTINATION bin)
install(FILES
    ${PROJECT_SOURCE_DIR}/LICENSE
    ${PROJECT_SOURCE_DIR}/README.md
    ${PROJECT_SOURCE_DIR}/docs/history.ru.txt
    DESTINATION .)

