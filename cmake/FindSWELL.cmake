if(WIN32 OR TARGET SWELL)
  return()
endif()

find_path(SWELL_INCLUDE_DIR
  NAMES swell/swell.h
  PATHS vendor/WDL
  PATH_SUFFIXES WDL
)

set(SWELL_DIR "${SWELL_INCLUDE_DIR}/swell")
set(SWELL_RESGEN "${SWELL_DIR}/mac_resgen.php")

add_library(SWELL
  ${WDL_DIR}/swell/swell-modstub$<IF:$<BOOL:${APPLE}>,.mm,-generic.cpp>
)

if(APPLE)
  find_library(APPKIT AppKit)
  target_link_libraries(SWELL PUBLIC ${APPKIT})
endif()

target_compile_definitions(SWELL PUBLIC SWELL_PROVIDED_BY_APP)
target_include_directories(SWELL INTERFACE ${SWELL_INCLUDE_DIR})
