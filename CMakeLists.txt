cmake_minimum_required(VERSION 3.13)
project(sws VERSION 1 LANGUAGES C CXX)

option(USE_TAGLIB    "Link with TagLib to add tagging support"     ON)
option(SYSTEM_TAGLIB "Try to use the system-provided TagLib first" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  if(WIN32)
    set(DEFAULT_INSTALL_DIR "$ENV{APPDATA}/REAPER/UserPlugins")
  elseif(APPLE)
    set(DEFAULT_INSTALL_DIR "~/Library/Application Support/REAPER/UserPlugins")
  else()
    set(DEFAULT_INSTALL_DIR "~/.config/REAPER/UserPlugins")
  endif()

  set(CMAKE_INSTALL_PREFIX "${DEFAULT_INSTALL_DIR}" CACHE PATH
    "Install SWS in this directory" FORCE)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL "8")
  set(ARCH_SUFFIX "64")
  set(REASCRIPT_PYTHON_FLAGS, "-x64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL "4")
  set(ARCH_SUFFIX "32")
else()
  message(FATAL_ERROR "Unknown architecture with sizeof(void*) == ${CMAKE_SIZEOF_VOID_P}")
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE        ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL     ON)

set(CMAKE_CXX_VISIBILITY_PRESET "hidden")

add_custom_command(
  OUTPUT  reascript_vararg.h
  COMMAND php reascript_vararg.php > ${CMAKE_CURRENT_BINARY_DIR}/reascript_vararg.h
  DEPENDS ReaScript.cpp reascript_vararg.php
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_command(
  OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/sws_python.py
  COMMAND perl reascript_python.pl ${REASCRIPT_PYTHON_FLAGS} >
    ${CMAKE_CURRENT_BINARY_DIR}/sws_python.py
  DEPENDS ReaScript.cpp reascript_python.pl reascript_helper.pl
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

if(UNIX)
  add_compile_options(
    -fstack-protector-strong
  )
elseif(MSVC)
  add_compile_definitions(sws _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)

  foreach(arg
    CMAKE_C_FLAGS_DEBUG CMAKE_CXX_FLAGS_DEBUG
    CMAKE_C_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELEASE
    CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_RELWITHDEBINFO
    CMAKE_C_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_MINSIZEREL
  )
    # Use the static CRT runtime library
    string(REPLACE "/MD" "/MT" ${arg} "${${arg}}")

    # Embed debug information in the object files
    string(REPLACE "/Zi" "/Z7" ${arg} "${${arg}}")
  endforeach()

  # Global compiler flags (all targets)
  add_compile_options(
    # Buffer security check
    /GS

    # Enhance optimized debugging
    $<$<NOT:$<CONFIG:Debug>>:/Zo>

    # Eliminate duplicate strings
    $<$<NOT:$<CONFIG:Debug>>:/GF>

    # Enable function-level linking
    $<$<NOT:$<CONFIG:Debug>>:/Gy>

    # Remove unreferenced COMDAT
    $<$<NOT:$<CONFIG:Debug>>:/Zc:inline>
  )
endif()

add_library(sws SHARED
  Menus.cpp
  Prompt.cpp
  reaper/reaper.cpp
  ReaScript.cpp
  sws_about.cpp
  sws_extension.cpp
  sws_extension.rc
  sws_util.cpp
  sws_waitdlg.cpp
  sws_wnd.cpp
  Zoom.cpp
  Utility/Base64.cpp
  libebur128/ebur128.cpp

  # Platform-specific code
  $<$<BOOL:${APPLE}>:sws_util.mm>
  $<$<BOOL:${WIN32}>:
    Utility/win32-utf8.cpp
    DragDrop.cpp
  >

  # Generated files
  reascript_vararg.h
  ${CMAKE_CURRENT_BINARY_DIR}/sws_python.py
)

target_compile_features(sws PRIVATE cxx_std_11)

target_include_directories(sws PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor
  ${CMAKE_CURRENT_BINARY_DIR}
)

if(USE_TAGLIB)
  find_package(TagLib)
endif()

if(TagLib_FOUND)
  target_link_libraries(sws TagLib::TagLib)
else()
  target_compile_definitions(sws PRIVATE NO_TAGLIB)
endif()

find_package(WDL REQUIRED)
find_package(LICE REQUIRED)
find_package(JNetLib REQUIRED)
target_link_libraries(sws JNetLib::JNetLib LICE::LICE WDL::WDL)

find_package(SWELL)
if(SWELL_FOUND)
  target_link_libraries(sws SWELL::swell)

  # Transpile the Win32 resource file
  set(WDL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/WDL/WDL)
  add_custom_command(
    OUTPUT  sws_extension.rc_mac_dlg sws_extension.rc_mac_menu
    COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_CURRENT_SOURCE_DIR}/sws_extension.rc ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${SWELL_RESGEN} sws_extension.rc
    DEPENDS ${SWELL_RESGEN}
    MAIN_DEPENDENCY sws_extension.rc
  )
endif()

if(APPLE)
  target_compile_options(sws PRIVATE
    # Disable annoying warnings from including WDL/MersenneTwister.h
    -Wno-deprecated-register

    # Ignore deprecated stat64 from including WDL/dirscan.h
    -Wno-deprecated-declarations
  )
endif()

if(MSVC)
  target_link_libraries(sws Shlwapi)

  target_link_options(sws PRIVATE $<$<NOT:$<CONFIG:Debug>>:
    # Remove unreferenced code
    /OPT:REF

    # Remove duplicate sections
    /OPT:ICF
  >)

  # Use the precompiled header for all files
  set(PCH_INPUT  "stdafx.h")
  set(PCH_OUTPUT "stdafx.pch")
  target_compile_options(sws PRIVATE /Yu${PCH_INPUT} /Fp${PCH_OUTPUT})

  # ...except for these files
  set_source_files_properties(
    reaper/reaper.cpp
    PROPERTIES COMPILE_OPTIONS "/Y-"
  )

  # Build the precompiled header
  set_source_files_properties(stdafx.cpp
  PROPERTIES
    COMPILE_OPTIONS "/Yc${PCH_INPUT};/Fp${PCH_OUTPUT}"
    OBJECT_OUTPUTS "${PCH_OUTPUT}"
  )
  add_library(pch OBJECT stdafx.cpp)
  target_include_directories(pch PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor
    ${WDL_INCLUDE_DIR}
  )
  add_dependencies(sws pch)
  target_link_libraries(sws pch)
endif()

set_target_properties(sws PROPERTIES
  PREFIX "" # disable 'lib' prefix
  OUTPUT_NAME "reaper_sws${ARCH_SUFFIX}"
)

# Install the .dylib/.so/.dll directly in CMAKE_INSTALL_PREFIX without creating
# additional bin/lib subdirectories
install(TARGETS sws
  RUNTIME DESTINATION "."
  LIBRARY DESTINATION "."
)

add_subdirectory(Autorender)
add_subdirectory(Breeder)
add_subdirectory(cfillion)
add_subdirectory(Color)
add_subdirectory(Console)
add_subdirectory(Fingers)
add_subdirectory(Freeze)
add_subdirectory(IX)
add_subdirectory(MarkerActions)
add_subdirectory(MarkerList)
add_subdirectory(Misc)
add_subdirectory(nofish)
add_subdirectory(ObjectState)
add_subdirectory(Padre)
add_subdirectory(Projects)
add_subdirectory(Snapshots)
add_subdirectory(SnM)
add_subdirectory(snooks)
add_subdirectory(TrackList)
add_subdirectory(Wol)
add_subdirectory(Xenakios)
