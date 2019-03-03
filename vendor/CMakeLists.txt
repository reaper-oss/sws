set(WDL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/WDL/WDL)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(WDL STATIC
  ${WDL_DIR}/projectcontext.cpp
)

if(APPLE)
  target_sources(WDL
  PRIVATE
    ${WDL_DIR}/SWELL/swell-modstub.mm
  )
elseif(WIN32)
  target_sources(WDL
  PRIVATE
    ${WDL_DIR}/win32_utf8.c
  )
endif()

add_library(jnetlib STATIC
  ${WDL_DIR}/jnetlib/httpget.cpp
  ${WDL_DIR}/jnetlib/util.cpp
  ${WDL_DIR}/jnetlib/connection.cpp
  ${WDL_DIR}/jnetlib/asyncdns.cpp
)

add_library(LICE STATIC
  ${WDL_DIR}/lice/lice.cpp
  ${WDL_DIR}/lice/lice_arc.cpp
  ${WDL_DIR}/lice/lice_line.cpp
  ${WDL_DIR}/lice/lice_png.cpp
  ${WDL_DIR}/lice/lice_textnew.cpp
)

add_library(wingui STATIC
  ${WDL_DIR}/wingui/virtwnd-iconbutton.cpp
  ${WDL_DIR}/wingui/virtwnd-slider.cpp
  ${WDL_DIR}/wingui/virtwnd.cpp
  ${WDL_DIR}/wingui/virtwnd.h
  ${WDL_DIR}/wingui/wndsize.cpp
)

add_library(libpng STATIC
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

add_library(zlib STATIC
  ${WDL_DIR}/zlib/adler32.c
  ${WDL_DIR}/zlib/crc32.c
  ${WDL_DIR}/zlib/inffast.c
  ${WDL_DIR}/zlib/inflate.c
  ${WDL_DIR}/zlib/inftrees.c
  ${WDL_DIR}/zlib/zutil.c
)
