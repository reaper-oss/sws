cmake_minimum_required(VERSION 3.13)
project(sws VERSION 1 LANGUAGES C CXX)

add_compile_definitions(NO_TAGLIB)

if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
  set(arch_suffix "64")
  set(reascript_python_flags, "-x64")
elseif("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
  set(arch_suffix "32")
else()
  message(FATAL_ERROR "Unknown architecture with sizeof(void*) == ${CMAKE_SIZEOF_VOID_P}")
endif()

if(UNIX)
  # Use the SWELL functions provided at runtime by REAPER rather than building
  # a copy in SWS
  add_compile_definitions(SWELL_PROVIDED_BY_APP)

  # Compile the Win32 resource file
  set(WDL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/WDL/WDL)
  add_custom_command(
    OUTPUT sws_extension.rc_mac_dlg sws_extension.rc_mac_menu
    COMMAND php ${WDL_DIR}/swell/mac_resgen.php sws_extension.rc
    MAIN_DEPENDENCY sws_extension.rc
    DEPENDS ${WDL_DIR}/swell/mac_resgen.php
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
endif()

add_custom_command(
  OUTPUT reascript_vararg.h
  COMMAND php reascript_vararg.php > reascript_vararg.h
  DEPENDS ReaScript.cpp reascript_vararg.php
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/sws_python.py
  COMMAND perl reascript_python.pl ${reascript_python_flags} >
    ${CMAKE_BINARY_DIR}/sws_python.py
  DEPENDS ReaScript.cpp reascript_python.pl reascript_helper.pl
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

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

  libebur128/ebur128.cpp

  Utility/Base64.cpp

  # Generated files
  reascript_vararg.h
  ${CMAKE_BINARY_DIR}/sws_python.py
)

target_include_directories(sws
PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor/WDL
)

target_compile_features(sws PRIVATE cxx_std_11)

target_compile_options(sws PRIVATE
  # Enable C++ exception support
  $<$<CXX_COMPILER_ID:MSVC>:/EHsc>

  # Disable annoying warnings from WDL/MersenneTwister.h which is included into
  # almost every source file via stdafx.h
  $<$<CXX_COMPILER_ID:AppleClang>:-Wno-deprecated-register>
)

target_link_libraries(sws jnetlib png lice wdl wingui zlib)

if(APPLE)
  target_sources(sws
  PRIVATE
    sws_util.mm
  )

  find_library(APPKIT AppKit)
  target_link_libraries(sws ${APPKIT})
elseif(WIN32)
  target_sources(sws
  PRIVATE
    Utility/win32-utf8.cpp
    DragDrop.cpp
  )

  target_link_libraries(sws Shlwapi)
endif()

set_target_properties(sws PROPERTIES
  PREFIX "reaper_"
  OUTPUT_NAME "sws${arch_suffix}"
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
add_subdirectory(vendor)
add_subdirectory(Wol)
add_subdirectory(Xenakios)
