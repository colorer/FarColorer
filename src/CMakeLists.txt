#====================================================
# farcolorer
#====================================================

set(SRC_CPP
    ChooseTypeMenu.cpp
    ChooseTypeMenu.h
    FarEditor.cpp
    FarEditor.h
    FarEditorSet.cpp
    FarEditorSet.h
    FarHrcSettings.cpp
    FarHrcSettings.h
    HrcSettingsForm.cpp
    HrcSettingsForm.h
    SettingsControl.cpp
    SettingsControl.h
    const_strings.h
    pcolorer.cpp
    pcolorer.h
    pcolorer3.rc
    tools.cpp
    tools.h
    version.h)

set(SRC_DEF pcolorer3.def)

#====================================================
# common flags
#====================================================

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  # defaut for msvc
  # CMAKE_CXX_FLAGS= /DWIN32 /D_WINDOWS /GR /EHsc
  # CMAKE_CXX_FLAGS_DEBUG= /MDd /Zi /Ob0 /Od /RTC1
  # CMAKE_CXX_FLAGS_RELEASE= /MD /O2 /Ob2 /DNDEBUG

  set(FLAGS_CXX_DEFAULT /Zi)
  set(FLAGS_CXX_RELEASE /W3 /Oi /Ot /GL /GS-)
  set(FLAGS_CXX_DEBUG /W4 /GS)

  set(LINK_FLAGS "/MAP /MANIFEST:NO")

  # build with debug info in pdb
  if(CMAKE_BUILD_TYPE MATCHES "Release")
    set(LINK_FLAGS "${LINK_FLAGS} /incremental:no /OPT:REF /OPT:ICF /debug /ltcg")
  endif()

  if("${COLORER_BUILD_ARCH}" STREQUAL "x64")
    set(CMAKE_RC_FLAGS -D_WIN64)
  endif()

  if("${COLORER_BUILD_ARCH}" STREQUAL "ARM64")
    # Fix build on GitHub agent (VSCMD_VER: 17.10.5 , VCToolsVersion: 14.40.33807)
    set(LINK_FLAGS "${LINK_FLAGS} /ALTERNATENAME:__imp___std_init_once_begin_initialize=__imp_InitOnceBeginInitialize")
    set(LINK_FLAGS "${LINK_FLAGS} /ALTERNATENAME:__imp___std_init_once_complete=__imp_InitOnceComplete")
  endif()

  # xerces-c vs msxml
  add_definitions(-DNOGDI)
  # old functions
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
endif()

# set options for target
set(MAIN_FLAGS_CXX_DEBUG ${FLAGS_CXX_DEBUG} ${FLAGS_CXX_DEFAULT})
set(MAIN_FLAGS_CXX_RELEASE ${FLAGS_CXX_RELEASE} ${FLAGS_CXX_DEFAULT})
#====================================================
# build
#====================================================

set(SRC_FILES ${SRC_CPP} ${SRC_DEF})
add_library(farcolorer SHARED ${SRC_FILES})
target_link_libraries(farcolorer colorer_lib)
target_include_directories(farcolorer
    PRIVATE ${PROJECT_SOURCE_DIR}/external/far3sdk/
    )

set_target_properties(farcolorer PROPERTIES OUTPUT_NAME "colorer")
set_target_properties(farcolorer PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    )

if(${CMAKE_BUILD_TYPE} MATCHES Debug)
  target_compile_options(farcolorer PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${MAIN_FLAGS_CXX_DEBUG}>)
else()
  target_compile_options(farcolorer PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${MAIN_FLAGS_CXX_RELEASE}>)
endif()

set_target_properties(farcolorer PROPERTIES LINK_FLAGS "${LINK_FLAGS}")
