cmake_minimum_required(VERSION 3.15)
project (farcolorer CXX)

#====================================================
# init variables
#====================================================

# get source root
GET_FILENAME_COMPONENT(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

SET(SRC_ROOT ${PROJECT_ROOT}/../src)

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
  set(CMAKE_LINK_FLAGS "-Xlinker --Map -Xlinker libcolorer.map")
  set(FLAGS_RELEASE)
  set(FLAGS_DEBUG)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  # defaut for msvc
  # CMAKE_CXX_FLAGS= /DWIN32 /D_WINDOWS /GR /EHsc
  # CMAKE_CXX_FLAGS_DEBUG= /MDd /Zi /Ob0 /Od /RTC1
  # CMAKE_CXX_FLAGS_RELEASE= /MD /O2 /Ob2 /DNDEBUG

  set(FLAGS_CXX_DEFAULT /Zi)
  set(FLAGS_CXX_RELEASE /W3 /Oi /Ot /GL /GS-)
  set(FLAGS_CXX_DEBUG /W4 /GS)

  set(LINK_FLAGS "/MAP /MANIFEST:NO")

  if(${CMAKE_CL_64} STREQUAL "0")
    set(FLAGS_CXX_DEFAULT ${FLAGS_CXX_DEFAULT} /arch:IA32)
  endif()

endif()

# set options for target
set(MAIN_FLAGS_CXX_DEBUG ${FLAGS_CXX_DEBUG} ${FLAGS_CXX_DEFAULT})
set(MAIN_FLAGS_CXX_RELEASE ${FLAGS_CXX_RELEASE} ${FLAGS_CXX_DEFAULT})
set(MAIN_FLAGS_C_DEBUG ${FLAGS_C_DEBUG} ${FLAGS_C_DEFAULT})
set(MAIN_FLAGS_C_RELEASE ${FLAGS_C_RELEASE} ${FLAGS_C_DEFAULT})
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
)
