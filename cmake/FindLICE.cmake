if(TARGET lice)
  return()
endif()

find_package(WDL REQUIRED)
find_path(LICE_INCLUDE_DIR
  NAMES lice.h
  PATHS ${WDL_DIR}
  PATH_SUFFIXES lice
  NO_DEFAULT_PATH
)
mark_as_advanced(LICE_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LICE REQUIRED_VARS LICE_INCLUDE_DIR)

add_library(lice
  ${LICE_INCLUDE_DIR}/lice.cpp
  ${LICE_INCLUDE_DIR}/lice_arc.cpp
  ${LICE_INCLUDE_DIR}/lice_line.cpp
  ${LICE_INCLUDE_DIR}/lice_png.cpp
  ${LICE_INCLUDE_DIR}/lice_textnew.cpp
)

target_include_directories(lice INTERFACE ${LICE_INCLUDE_DIR})

find_package(PNG REQUIRED)
target_link_libraries(lice PNG::PNG)

find_package(SWELL)
if(SWELL_FOUND)
  target_link_libraries(lice SWELL::swell)
endif()

add_library(LICE::LICE ALIAS lice)
