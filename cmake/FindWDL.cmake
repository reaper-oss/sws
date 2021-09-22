if(TARGET wdl)
  return()
endif()

find_path(WDL_INCLUDE_DIR
  NAMES WDL/wdltypes.h
  PATHS ${CMAKE_SOURCE_DIR}/vendor/WDL
  NO_DEFAULT_PATH
)
mark_as_advanced(WDL_INCLUDE_DIR)

set(WDL_DIR "${WDL_INCLUDE_DIR}/WDL")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WDL REQUIRED_VARS WDL_INCLUDE_DIR)

add_library(wdl
  ${WDL_DIR}/projectcontext.cpp

  ${WDL_DIR}/wingui/virtwnd-iconbutton.cpp
  ${WDL_DIR}/wingui/virtwnd-slider.cpp
  ${WDL_DIR}/wingui/virtwnd.cpp
  ${WDL_DIR}/wingui/wndsize.cpp

  $<$<BOOL:${WIN32}>:${WDL_DIR}/win32_utf8.c>
)

target_compile_definitions(wdl PUBLIC _FILE_OFFSET_BITS=64)
target_compile_definitions(wdl INTERFACE WDL_NO_DEFINE_MINMAX)
target_include_directories(wdl INTERFACE ${WDL_INCLUDE_DIR})

find_package(LICE REQUIRED)
target_link_libraries(wdl LICE::LICE)

find_package(SWELL)
if(SWELL_FOUND)
  target_link_libraries(wdl SWELL::swell)
endif()

add_library(WDL::WDL ALIAS wdl)
