find_path(ZLIB_DIR
  NAMES zlib.h
  PATHS vendor/WDL/WDL/zlib
  NO_DEFAULT_PATH
)
mark_as_advanced(ZLIB_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZLIB
  REQUIRED_VARS ZLIB_DIR
)

add_library(z
  ${WDL_DIR}/zlib/adler32.c
  ${WDL_DIR}/zlib/crc32.c
  ${WDL_DIR}/zlib/inffast.c
  ${WDL_DIR}/zlib/inflate.c
  ${WDL_DIR}/zlib/inftrees.c
  ${WDL_DIR}/zlib/zutil.c
)
