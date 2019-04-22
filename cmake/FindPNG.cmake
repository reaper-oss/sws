if(PNG_FOUND)
  return()
endif()

find_path(PNG_DIR
  NAMES png.h
  PATHS vendor/WDL/WDL/libpng
  NO_DEFAULT_PATH
)
mark_as_advanced(PNG_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PNG
  REQUIRED_VARS PNG_DIR
)

add_library(png
  ${WDL_DIR}/libpng/png.c
  ${WDL_DIR}/libpng/pngerror.c
  ${WDL_DIR}/libpng/pngget.c
  ${WDL_DIR}/libpng/pngmem.c
  ${WDL_DIR}/libpng/pngpread.c
  ${WDL_DIR}/libpng/pngread.c
  ${WDL_DIR}/libpng/pngrio.c
  ${WDL_DIR}/libpng/pngrtran.c
  ${WDL_DIR}/libpng/pngrutil.c
  ${WDL_DIR}/libpng/pngset.c
  ${WDL_DIR}/libpng/pngtrans.c
)

find_package(ZLIB REQUIRED)
target_link_libraries(png PUBLIC ZLIB::ZLIB)

add_library(PNG::PNG ALIAS png)
