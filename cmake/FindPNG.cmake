if(TARGET png)
  return()
endif()

find_package(WDL REQUIRED)
find_path(PNG_INCLUDE_DIR
  NAMES png.h
  PATHS ${WDL_DIR}
  PATH_SUFFIXES libpng
  NO_DEFAULT_PATH
)
mark_as_advanced(PNG_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PNG REQUIRED_VARS PNG_INCLUDE_DIR)

add_library(png
  ${PNG_INCLUDE_DIR}/png.c
  ${PNG_INCLUDE_DIR}/pngerror.c
  ${PNG_INCLUDE_DIR}/pngget.c
  ${PNG_INCLUDE_DIR}/pngmem.c
  ${PNG_INCLUDE_DIR}/pngpread.c
  ${PNG_INCLUDE_DIR}/pngread.c
  ${PNG_INCLUDE_DIR}/pngrio.c
  ${PNG_INCLUDE_DIR}/pngrtran.c
  ${PNG_INCLUDE_DIR}/pngrutil.c
  ${PNG_INCLUDE_DIR}/pngset.c
  ${PNG_INCLUDE_DIR}/pngtrans.c
)

target_include_directories(png INTERFACE ${PNG_INCLUDE_DIR})

find_package(ZLIB REQUIRED)
target_link_libraries(png PUBLIC ZLIB::ZLIB)

add_library(PNG::PNG ALIAS png)
