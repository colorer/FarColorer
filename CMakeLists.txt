cmake_minimum_required (VERSION 2.8.12.2)
project (farcolorer)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
message("Configurations for IDE: ${CMAKE_CONFIGURATION_TYPES}")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type, one of: Release, Debug" FORCE)
endif()
message("Build type: ${CMAKE_BUILD_TYPE}")

#====================================================
# colorer_lib
#====================================================
add_subdirectory(./libs/colorer/)

#====================================================
# farcolorer
#====================================================
add_subdirectory(./src)
