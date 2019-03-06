if(SYSTEM_TAGLIB)
  find_path(TagLib_INCLUDE_DIR NAMES taglib.h PATH_SUFFIXES taglib)
  find_library(TagLib_LIBRARY tag)
  mark_as_advanced(TagLib_INCLUDE_DIR TagLib_LIBRARY)
endif()

# Build TagLib from source
if(NOT TagLib_INCLUDE_DIR)
  set(TagLib_SOURCE_DIR "${CMAKE_SOURCE_DIR}/vendor/taglib")
endif()

if(TagLib_SOURCE_DIR AND EXISTS "${TagLib_SOURCE_DIR}/CMakeLists.txt")
  set(BUILD_BINDINGS OFF CACHE BOOL "Build TagLib C bindings")

  # Prevent TagLib from attempting to link against boost (v1.11.1 and older)
  set(CMAKE_DISABLE_FIND_PACKAGE_Boost ON)

  set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)
  set(CMAKE_POLICY_DEFAULT_CMP0063 NEW)
  set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
  add_subdirectory(${TagLib_SOURCE_DIR} EXCLUDE_FROM_ALL)

  # TagLib does not export its CMake targets, so we must do it ourselves.
  set(TagLib_INCLUDE_DIR "${TagLib_SOURCE_DIR}/taglib/toolkit") # path to taglib.h

  # just for a prettier find_package_handle_standard_args's default "found" message
  set(TagLib_LIBRARY "${TagLib_SOURCE_DIR}")

  target_compile_definitions(tag INTERFACE TAGLIB_STATIC)

  get_target_property(TagLib_INCLUDE_DIR_INTERFACE tag INCLUDE_DIRECTORIES)
  target_include_directories(tag INTERFACE
    ${TagLib_SOURCE_DIR}
    ${TagLib_SOURCE_DIR}/taglib
    ${TagLib_INCLUDE_DIR_INTERFACE}
  )
endif()

# Detect TagLib's version
if(EXISTS "${TagLib_INCLUDE_DIR}/taglib.h")
  file(STRINGS "${TagLib_INCLUDE_DIR}/taglib.h" TagLib_H REGEX "${TagLib_VERSION_REGEX}")

  foreach(segment MAJOR MINOR PATCH)
    set(regex "#define TAGLIB_${segment}_VERSION ([0-9]+)")
    string(REGEX MATCH "${regex}" match "${TagLib_H}")
    string(REGEX REPLACE "${regex}" "\\1" TagLib_VERSION_${segment} "${match}")
  endforeach()

  set(TagLib_VERSION
    "${TagLib_VERSION_MAJOR}.${TagLib_VERSION_MINOR}.${TagLib_VERSION_PATCH}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TagLib
  REQUIRED_VARS TagLib_LIBRARY TagLib_INCLUDE_DIR
  VERSION_VAR   TagLib_VERSION
)

if(TagLib_FOUND AND NOT TARGET TagLib::TagLib)
  if(TagLib_SOURCE_DIR)
    add_library(TagLib::TagLib ALIAS tag)
  else()
    add_library(TagLib::TagLib UNKNOWN IMPORTED)
    set_target_properties(TagLib::TagLib PROPERTIES
      IMPORTED_LOCATION             "${TagLib_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${TagLib_INCLUDE_DIR}"
    )
  endif()
endif()
