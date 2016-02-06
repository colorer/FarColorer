cmake_minimum_required (VERSION 2.8.9)
project (farcolorer)

#====================================================
# user settings
#====================================================
set(COLORER_FEATURE_HTTPINPUTSOURCE OFF CACHE BOOL "If defined, HTTP InputSource is implemented")
#====================================================
# init variables
#====================================================

# get source root
GET_FILENAME_COMPONENT(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

SET(SRC_ROOT ${PROJECT_ROOT}/../src)

IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Release)
ENDIF()

#====================================================
# colorer_lib
#====================================================
add_subdirectory(../libs/colorer/src/ ./build)
include_directories(../libs/colorer/src/)
include_directories (../libs/colorer/libs/xercesc/src)
set(CMAKE_CXX_FLAGS "-DXERCES_STATIC_LIBRARY")
#====================================================
# far sdk
#====================================================

# add ./ to fix build on farmanager server
include_directories (./ ../libs/far3sdk/) 

set(SRC_CPP 
  pcolorer.cpp pcolorer.h
  FarEditorSet.cpp FarEditorSet.h
  FarEditor.cpp FarEditor.h
  ChooseTypeMenu.cpp ChooseTypeMenu.h
  FarHrcSettings.cpp FarHrcSettings.h
  SettingsControl.cpp SettingsControl.h
  tools.cpp tools.h
  pcolorer3.rc
  version.h
)

if(MSVC)
  set(SRC_DEF pcolorer3.def)
else()
  set(SRC_DEF pcolorer3.gcc.def)
endif()

if(${CMAKE_GENERATOR} STREQUAL "NMake Makefiles")
  if(${CMAKE_CL_64} STREQUAL "0")
    find_program (MASM_EXECUTABLE ml)

    set(ASM_OBJECTS)
    foreach(src vc_crt_fix)
      set(ASM_SOURCE ${SRC_ROOT}/${src}.asm)
      set(ASM_OBJECT ${CMAKE_CURRENT_BINARY_DIR}/${src}.obj)
      set(ASM_OBJECTS ${ASM_OBJECTS} ${ASM_OBJECT})
      add_custom_command(
        OUTPUT ${ASM_OBJECT}
        COMMAND ${MASM_EXECUTABLE}
        ARGS   /c /Fo ${ASM_OBJECT} ${ASM_SOURCE}
        DEPENDS ${ASM_SOURCE}
      )
    endforeach(src)

    set(SRC_CPP ${SRC_CPP}
      vc_crt_fix_impl.cpp
      ${ASM_OBJECTS}
    )
  endif()
endif()
#====================================================
# common flags
#====================================================

if (COLORER_FEATURE_HTTPINPUTSOURCE AND WIN32)
  set(WININETLIB wininet)
endif()

add_definitions(-D_UNICODE -DUNICODE)

set(FLAGS_RELEASE "-U_DEBUG")
set(FLAGS_DEBUG "-D_DEBUG")

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
  add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)

  set(LINK_FLAGS "/MAP /MANIFEST:NO")
  set(LINK_FLAGS_RELEASE "/LTCG")

  set(COLORER_WARNING_FLAGS "")
  set(FLAGS_CXX_DEFAULT "${CMAKE_CXX_FLAGS} /EHsc /Zi -D_WINDLL -D_USRDLL /Zc:threadSafeInit-")
  set(FLAGS_C_DEFAULT "${FLAGS_C_DEFAULT}")

  set(FLAGS_RELEASE "${FLAGS_RELEASE} /O2 /Oi /Ot /Oy /GL /GF /GS- /Gy /MT")
  set(FLAGS_DEBUG "${FLAGS_DEBUG} /Od /MTd /GS /RTC1 /W3")

  if(${CMAKE_CL_64} STREQUAL "0")
    set(FLAGS_CXX_DEFAULT "${FLAGS_CXX_DEFAULT} /arch:IA32")
  endif()
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
  set(COLORER_WARNING_FLAGS )
  set(FLAGS_CXX_DEFAULT "${FLAGS_CXX_DEFAULT} -fpermissive -std=c++11")

  set(FLAGS_RELEASE "${FLAGS_RELEASE} -s -O3")
  set(FLAGS_DEBUG "${FLAGS_DEBUG}")
  set(CMAKE_LINK_FLAGS "-Xlinker --Map -Xlinker libcolorer.map")
  set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif()

set(CMAKE_C_FLAGS "${FLAGS_C_DEFAULT} ${COLORER_WARNING_FLAGS}")
set(CMAKE_CXX_FLAGS "${FLAGS_CXX_DEFAULT} ${COLORER_WARNING_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${FLAGS_RELEASE}")
set(CMAKE_C_FLAGS_RELEASE "${FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_DEBUG "${FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_DEBUG "${FLAGS_DEBUG}")

#====================================================
# build
#====================================================

set(LIBRARIES colorer_lib xerces-c_3_1)
set(SRC_FILES ${SRC_CPP} ${SRC_DEF})
add_library(colorer SHARED ${SRC_FILES} )
target_link_libraries(colorer ${LIBRARIES} ${WININETLIB})
set_target_properties(colorer
  PROPERTIES
  LINK_FLAGS "${LINK_FLAGS}"
  LINK_FLAGS_RELEASE "${LINK_FLAGS_RELEASE}"
)
