find_path(JNetLib_INCLUDE_DIR
  NAMES jnetlib/jnetlib.h
  PATHS vendor/WDL
  PATH_SUFFIXES WDL
  NO_DEFAULT_PATH
)
mark_as_advanced(JNetLib_INCLUDE_DIR)

set(JNetLib_DIR "${JNetLib_INCLUDE_DIR}/jnetlib")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JNetLib
  REQUIRED_VARS JNetLib_DIR JNetLib_INCLUDE_DIR
)

add_library(jnetlib
  ${JNetLib_DIR}/httpget.cpp
  ${JNetLib_DIR}/util.cpp
  ${JNetLib_DIR}/connection.cpp
  ${JNetLib_DIR}/asyncdns.cpp
)

target_include_directories(jnetlib INTERFACE ${JNetLib_INCLUDE_DIR})
