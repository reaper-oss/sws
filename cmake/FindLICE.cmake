if(LICE_FOUND)
  return()
endif()

find_path(LICE_INCLUDE_DIR
  NAMES lice/lice.h
  PATHS vendor/WDL
  PATH_SUFFIXES WDL
  NO_DEFAULT_PATH
)
mark_as_advanced(LICE_INCLUDE_DIR)

set(LICE_DIR "${LICE_INCLUDE_DIR}/lice")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LICE
  REQUIRED_VARS LICE_DIR LICE_INCLUDE_DIR
)

add_library(lice
  ${LICE_DIR}/lice.cpp
  ${LICE_DIR}/lice_arc.cpp
  ${LICE_DIR}/lice_line.cpp
  ${LICE_DIR}/lice_png.cpp
  ${LICE_DIR}/lice_textnew.cpp
)

target_include_directories(lice INTERFACE ${lice_INCLUDE_DIR})

find_package(PNG REQUIRED)
target_link_libraries(lice PNG::PNG)

find_package(SWELL)
if(SWELL_FOUND)
  target_link_libraries(lice SWELL::swell)
endif()

add_library(LICE::LICE ALIAS lice)
