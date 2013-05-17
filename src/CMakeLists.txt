cmake_minimum_required (VERSION 2.8.9)
project (farcolorer)

#====================================================
# user settings
#====================================================
set(COLORER_FEATURE_HTTPINPUTSOURCE OFF CACHE BOOL "If defined, HTTP InputSource is implemented")

#====================================================
# init variables
#====================================================

STRING(REPLACE "\\" "/" PROJECT_ROOT ${PROJECT_ROOT})
SET(SRC_ROOT ${PROJECT_ROOT}/src)

IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Release)
ENDIF()

if(NOT DEFINED CONF)
  set(CONF "x86")
endif()
        
if(CONF STREQUAL "x86")
  set(PLATFORM "Win32")
else()
  set(PLATFORM "x64")
endif()

#====================================================
# colorer_lib
#====================================================
add_subdirectory(../libs/colorer/src/ ./build)
include_directories(../libs/colorer/src/)

#====================================================
# far sdk
#====================================================

include_directories (../libs/far3sdk/)

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

if(MSVC)
if(PLATFORM STREQUAL "Win32")
  find_program (MASM_EXECUTABLE ml)
  # message("masm: ${MASM_EXECUTABLE}")
  set(ASM_OBJECTS)
  foreach(src vc10)
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
    vc10wrapper.cpp
    ${ASM_OBJECTS}
  )
endif(PLATFORM STREQUAL "Win32")
endif(MSVC)
#====================================================
# common flags
#====================================================

if (COLORER_FEATURE_HTTPINPUTSOURCE AND WIN32)
  set(WININETLIB wininet)
endif()

if(PLATFORM STREQUAL "Win32")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DWIN32")
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DWIN64")
endif()

set(FLAGS_RELEASE "-U_DEBUG -DNDEBUG -D_UNICODE -DUNICODE")
set(FLAGS_DEBUG "-D_DEBUG -DDEBUG -D_UNICODE -DUNICODE ")

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
  add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)

  set(CMAKE_LINK_FLAGS "/MAP")

  set(COLORER_WARNING_FLAGS "")
  set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} /EHsc -D_WINDLL -D_USRDLL")

  set(FLAGS_RELEASE "${FLAGS_RELEASE} /O2 /Oi /Ot /Oy /GL /GF /GS- /Gy /MT")
  set(FLAGS_DEBUG "${FLAGS_DEBUG} /Od /MTd /GS /RTC1")

endif()

if(CMAKE_COMPILER_IS_GNUCXX)
  set(COLORER_WARNING_FLAGS )
  set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fpermissive -std=c++11")

  set(FLAGS_RELEASE "${FLAGS_RELEASE} -s -O3")
  set(FLAGS_DEBUG "${FLAGS_DEBUG}")
  set(CMAKE_LINK_FLAGS "-Xlinker --Map -Xlinker libcolorer.map")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COLORER_WARNING_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COLORER_WARNING_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${FLAGS_RELEASE}")
set(CMAKE_C_FLAGS_RELEASE "${FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_DEBUG "${FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_DEBUG "${FLAGS_DEBUG}")

#====================================================
# build
#====================================================

set(LIBRARIES colorer_lib)
set(SRC_FILES ${SRC_CPP} ${SRC_DEF})
add_library(colorer SHARED ${SRC_FILES} )
target_link_libraries(colorer ${LIBRARIES} ${WININETLIB})
set_target_properties(colorer
  PROPERTIES
  LINK_FLAGS "${CMAKE_LINK_FLAGS}"
)
