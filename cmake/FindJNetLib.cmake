if(TARGET jnetlib)
  return()
endif()

find_package(WDL REQUIRED)
find_path(JNetLib_INCLUDE_DIR
  NAMES jnetlib.h
  PATHS ${WDL_DIR}
  PATH_SUFFIXES jnetlib
  NO_DEFAULT_PATH
)
mark_as_advanced(JNetLib_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JNetLib REQUIRED_VARS JNetLib_INCLUDE_DIR)

add_library(jnetlib
  ${JNetLib_INCLUDE_DIR}/httpget.cpp
  ${JNetLib_INCLUDE_DIR}/util.cpp
  ${JNetLib_INCLUDE_DIR}/connection.cpp
  ${JNetLib_INCLUDE_DIR}/asyncdns.cpp
)

target_include_directories(jnetlib INTERFACE ${JNetLib_INCLUDE_DIR})

add_library(JNetLib::JNetLib ALIAS jnetlib)
