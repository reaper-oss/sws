find_path(LICE_INCLUDE_DIR
  NAMES lice/lice.h
  PATHS vendor/WDL
  PATH_SUFFIXES WDL
)

set(LICE_DIR "${LICE_INCLUDE_DIR}/lice")

add_library(LICE
  ${LICE_DIR}/lice.cpp
  ${LICE_DIR}/lice_arc.cpp
  ${LICE_DIR}/lice_line.cpp
  ${LICE_DIR}/lice_png.cpp
  ${LICE_DIR}/lice_textnew.cpp
)

target_include_directories(LICE INTERFACE ${lice_INCLUDE_DIR})

find_package(PNG NO_SYSTEM_ENVIRONMENT_PATH)
find_package(SWELL)

if(NOT PNG_FOUND)
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

  add_library(zlib
    ${WDL_DIR}/zlib/adler32.c
    ${WDL_DIR}/zlib/crc32.c
    ${WDL_DIR}/zlib/inffast.c
    ${WDL_DIR}/zlib/inflate.c
    ${WDL_DIR}/zlib/inftrees.c
    ${WDL_DIR}/zlib/zutil.c
  )

  target_link_libraries(png PUBLIC zlib)
endif()

target_link_libraries(LICE png)

if(TARGET SWELL)
  target_link_libraries(LICE SWELL)
endif()
