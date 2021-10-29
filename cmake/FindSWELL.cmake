if(WIN32 OR TARGET SWELL::swell)
  return()
endif()

find_package(WDL REQUIRED)

find_path(SWELL_INCLUDE_DIR
  NAMES swell/swell.h
  PATHS ${WDL_DIR}
  NO_DEFAULT_PATH
)
mark_as_advanced(SWELL_INCLUDE_DIR)

set(SWELL_DIR "${SWELL_INCLUDE_DIR}/swell")
set(SWELL_RESGEN "${SWELL_DIR}/swell_resgen.php")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SWELL REQUIRED_VARS SWELL_DIR)

add_library(swell
  ${SWELL_DIR}/swell-modstub$<IF:$<BOOL:${APPLE}>,.mm,-generic.cpp>)

set_property(TARGET swell PROPERTY CXX_STANDARD 98)
target_compile_definitions(swell PUBLIC SWELL_PROVIDED_BY_APP)
target_include_directories(swell INTERFACE ${SWELL_INCLUDE_DIR})
target_link_libraries(swell PUBLIC WDL::WDL)

if(APPLE)
  find_library(APPKIT_LIB AppKit)
  target_link_libraries(swell PUBLIC ${APPKIT_LIB})
endif()

add_library(SWELL::swell ALIAS swell)
