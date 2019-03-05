find_path(JNetLib_INCLUDE_DIR
  NAMES jnetlib/jnetlib.h
  PATHS vendor/WDL
  PATH_SUFFIXES WDL
)

set(JNetLib_DIR "${JNetLib_INCLUDE_DIR}/jnetlib")

add_library(JNetLib
  ${JNetLib_DIR}/httpget.cpp
  ${JNetLib_DIR}/util.cpp
  ${JNetLib_DIR}/connection.cpp
  ${JNetLib_DIR}/asyncdns.cpp
)

target_include_directories(JNetLib INTERFACE ${JNetLib_INCLUDE_DIR})
