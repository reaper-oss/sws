if(TARGET z)
  return()
endif()

find_package(WDL REQUIRED)
find_path(ZLIB_INCLUDE_DIR
  NAMES zlib.h
  PATHS ${WDL_DIR}
  PATH_SUFFIXES zlib
  NO_DEFAULT_PATH
)
mark_as_advanced(ZLIB_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZLIB REQUIRED_VARS ZLIB_INCLUDE_DIR)

add_library(z
  ${ZLIB_INCLUDE_DIR}/adler32.c
  ${ZLIB_INCLUDE_DIR}/crc32.c
  ${ZLIB_INCLUDE_DIR}/inffast.c
  ${ZLIB_INCLUDE_DIR}/inflate.c
  ${ZLIB_INCLUDE_DIR}/inftrees.c
  ${ZLIB_INCLUDE_DIR}/zutil.c
)

target_include_directories(z INTERFACE ${ZLIB_INCLUDE_DIR})

add_library(ZLIB::ZLIB ALIAS z)
