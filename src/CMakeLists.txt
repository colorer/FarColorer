cmake_minimum_required (VERSION 2.8.12.2)
project (farcolorer)

#====================================================
# init variables
#====================================================

# get source root
GET_FILENAME_COMPONENT(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

SET(SRC_ROOT ${PROJECT_ROOT}/../src)

IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Release)
ENDIF()

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

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  # defaut for gcc
  # CMAKE_CXX_FLAGS=
  # CMAKE_CXX_FLAGS_DEBUG= -g
  # CMAKE_CXX_FLAGS_RELEASE= -O3 -DNDEBUG

  # set only new flags
  set(FLAGS_CXX_DEFAULT -fpermissive -Wall)
  set(FLAGS_RELEASE)
  set(FLAGS_DEBUG)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  # defaut for msvc
  # CMAKE_C_FLAGS= /DWIN32 /D_WINDOWS /W3
  # CMAKE_C_FLAGS_DEBUG= /MDd /Zi /Ob0 /Od /RTC1
  # CMAKE_C_FLAGS_RELEASE= /MD /O2 /Ob2 /DNDEBUG

  # CMAKE_CXX_FLAGS= /DWIN32 /D_WINDOWS /W3 /GR /EHsc
  # CMAKE_CXX_FLAGS_DEBUG= /MDd /Zi /Ob0 /Od /RTC1
  # CMAKE_CXX_FLAGS_RELEASE= /MD /O2 /Ob2 /DNDEBUG

  # set all flags and clean default, because Visual Studio project don`t overwrite default parameters
  # https://stackoverflow.com/questions/45995784/how-to-set-compiler-options-with-cmake-in-visual-studio-2017
  set(FLAGS_C_DEFAULT /DWIN32 /D_WINDOWS /W3)
  set(FLAGS_C_RELEASE /MT /O2 /Ob2 /DNDEBUG)
  set(FLAGS_C_DEBUG /MTd /Zi /Ob0 /Od /RTC1)
  set(FLAGS_CXX_DEFAULT /DWIN32 /D_WINDOWS /W3 /GR /EHsc)
  set(FLAGS_CXX_RELEASE /MT /O2 /Ob2 /DNDEBUG)
  set(FLAGS_CXX_DEBUG /MTd /Zi /Ob0 /Od /RTC1)

  set(CMAKE_C_FLAGS "")
  set(CMAKE_C_FLAGS_DEBUG "")
  set(CMAKE_C_FLAGS_RELEASE "")
  set(CMAKE_CXX_FLAGS "")
  set(CMAKE_CXX_FLAGS_RELEASE "")
  set(CMAKE_CXX_FLAGS_DEBUG "")
endif()

#if(MSVC)
#  add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
#  add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)

 # set(LINK_FLAGS "/MAP /MANIFEST:NO")
 # set(LINK_FLAGS_RELEASE "/LTCG")

#  set(COLORER_WARNING_FLAGS "")
#  set(FLAGS_CXX_DEFAULT "${CMAKE_CXX_FLAGS} /EHsc /Zi -D_WINDLL -D_USRDLL /Zc:threadSafeInit-")
#  set(FLAGS_C_DEFAULT "${FLAGS_C_DEFAULT}")

  #set(FLAGS_RELEASE "${FLAGS_RELEASE} /O2 /Oi /Ot /Oy /GL /GF /GS- /Gy /MT")
  #set(FLAGS_DEBUG "${FLAGS_DEBUG} /Od /MTd /GS /RTC1 /W3")

  #if(${CMAKE_CL_64} STREQUAL "0")
  #  set(FLAGS_CXX_DEFAULT "${FLAGS_CXX_DEFAULT} /arch:IA32")
  #  set(FLAGS_C_DEFAULT "${FLAGS_C_DEFAULT} /arch:IA32")
  #endif()
#endif()

#if(CMAKE_COMPILER_IS_GNUCXX)
#  set(COLORER_WARNING_FLAGS )
#  set(FLAGS_CXX_DEFAULT "${FLAGS_CXX_DEFAULT} -fpermissive -std=c++11")

#  set(FLAGS_RELEASE "${FLAGS_RELEASE} -s -O3")
#  set(FLAGS_DEBUG "${FLAGS_DEBUG}")
#  set(CMAKE_LINK_FLAGS "-Xlinker --Map -Xlinker libcolorer.map")
#  set(CMAKE_SHARED_LIBRARY_PREFIX "")
#endif()

#====================================================
# build
#====================================================

set(SRC_FILES ${SRC_CPP} ${SRC_DEF})
add_library(colorer SHARED ${SRC_FILES} )
target_link_libraries(colorer colorer_lib)
target_include_directories(colorer
    PUBLIC ../libs/far3sdk/
    )
if(${CMAKE_BUILD_TYPE} MATCHES Debug)
  target_compile_options(colorer
      PUBLIC $<$<COMPILE_LANGUAGE:C>:${MAIN_FLAGS_C_DEBUG}>
      PUBLIC $<$<COMPILE_LANGUAGE:CXX>:${MAIN_FLAGS_CXX_DEBUG}>
      )
else()
  target_compile_options(colorer
      PUBLIC $<$<COMPILE_LANGUAGE:C>:${MAIN_FLAGS_C_RELEASE}>
      PUBLIC $<$<COMPILE_LANGUAGE:CXX>:${MAIN_FLAGS_CXX_RELEASE}>
      )
endif()

set_target_properties(colorer
  PROPERTIES
  LINK_FLAGS "${LINK_FLAGS}"
  LINK_FLAGS_RELEASE "${LINK_FLAGS_RELEASE}"
)
