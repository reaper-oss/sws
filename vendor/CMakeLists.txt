set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(WDL_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/WDL)
set(WDL_DIR ${WDL_INCLUDE_PATH}/WDL)

add_library(WDL
  ${WDL_DIR}/projectcontext.cpp

  ${WDL_DIR}/wingui/virtwnd-iconbutton.cpp
  ${WDL_DIR}/wingui/virtwnd-slider.cpp
  ${WDL_DIR}/wingui/virtwnd.cpp
  ${WDL_DIR}/wingui/virtwnd.h
  ${WDL_DIR}/wingui/wndsize.cpp
)

target_compile_options(WDL PUBLIC
  # Disable annoying warnings from WDL/MersenneTwister.h which is included into
  # almost every source file via stdafx.h
  -Wno-deprecated-register

  # Ignore deprecated stat64 from included WDL/dirscan.h
  -Wno-deprecated-declarations
)

target_include_directories(WDL INTERFACE ${WDL_INCLUDE_PATH})
target_compile_definitions(WDL PUBLIC WDL_NO_DEFINE_MINMAX)

if(WIN32)
  target_sources(WDL
  PRIVATE
    ${WDL_DIR}/win32_utf8.c
  )
else()
  if(APPLE)
    add_library(SWELL
      ${WDL_DIR}/swell/swell-modstub.mm
    )

    find_library(APPKIT AppKit)
    target_link_libraries(SWELL PUBLIC ${APPKIT})
  else()
    add_library(SWELL
      ${WDL_DIR}/swell/swell-modstub-generic.cpp
    )
  endif()

  target_compile_definitions(SWELL PUBLIC SWELL_PROVIDED_BY_APP)
  target_include_directories(SWELL INTERFACE ${WDL_DIR})

  target_link_libraries(WDL SWELL)
endif()

add_library(jnetlib
  ${WDL_DIR}/jnetlib/httpget.cpp
  ${WDL_DIR}/jnetlib/util.cpp
  ${WDL_DIR}/jnetlib/connection.cpp
  ${WDL_DIR}/jnetlib/asyncdns.cpp
)

add_library(LICE
  ${WDL_DIR}/lice/lice.cpp
  ${WDL_DIR}/lice/lice_arc.cpp
  ${WDL_DIR}/lice/lice_line.cpp
  ${WDL_DIR}/lice/lice_png.cpp
  ${WDL_DIR}/lice/lice_textnew.cpp
)

find_package(PNG NO_SYSTEM_ENVIRONMENT_PATH)

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
