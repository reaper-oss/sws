if(WIN32 OR SWELL_FOUND)
  return()
endif()

find_path(SWELL_INCLUDE_DIR
  NAMES swell/swell.h
  PATHS vendor/WDL
  PATH_SUFFIXES WDL
  NO_DEFAULT_PATH
)
mark_as_advanced(SWELL_INCLUDE_DIR)

set(SWELL_DIR "${SWELL_INCLUDE_DIR}/swell")
set(SWELL_RESGEN "${SWELL_DIR}/mac_resgen.php")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SWELL
  REQUIRED_VARS SWELL_DIR SWELL_INCLUDE_DIR
)

add_library(swell
  ${WDL_DIR}/swell/swell-modstub$<IF:$<BOOL:${APPLE}>,.mm,-generic.cpp>
)

if(APPLE)
  find_library(APPKIT AppKit)
  target_link_libraries(swell PUBLIC ${APPKIT})
endif()

target_compile_definitions(swell PUBLIC SWELL_PROVIDED_BY_APP)
target_include_directories(swell INTERFACE ${SWELL_INCLUDE_DIR})

add_library(SWELL::swell ALIAS swell)
