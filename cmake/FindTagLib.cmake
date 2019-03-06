if(SYSTEM_TAGLIB)
  find_path(TagLib_INCLUDE_DIR NAMES taglib.h PATH_SUFFIXES taglib)
  find_library(TagLib_LIBRARY tag)
  mark_as_advanced(TagLib_INCLUDE_DIR TagLib_LIBRARY)
endif()

# Build TagLib from source
if(NOT TagLib_INCLUDE_DIR OR NOT EXISTS ${TagLib_INCLUDE_DIR})
  set(BUILD_BINDINGS OFF CACHE BOOL "Build TagLib C bindings")

  # Prevent TagLib v1.11.1 and older from attempting to linking against boost
  set(Boost_NO_SYSTEM_PATHS ON)
  set(Boost_INCLUDE_DIR     "")

  include(FetchContent)
  FetchContent_Declare(taglib
    GIT_REPOSITORY https://github.com/taglib/taglib.git
    GIT_TAG        v1.11.1
  )

  # FIXME: CMake 3.14+
  # FetchContent_MakeAvailable(TagLib)

  FetchContent_GetProperties(taglib)
  if(NOT taglib_POPULATED)
    FetchContent_Populate(taglib)

    set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)
    set(CMAKE_POLICY_DEFAULT_CMP0063 NEW)
    set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
    add_subdirectory(${taglib_SOURCE_DIR} ${taglib_BINARY_DIR})

    # TagLib does not export its CMake targets, so we must do it ourselves.
    set(TagLib_INCLUDE_DIR "${taglib_SOURCE_DIR}/taglib/toolkit") # path to taglib.h
    set(TagLib_LIBRARY "<from source>")

    target_compile_definitions(tag INTERFACE TAGLIB_STATIC)

    get_target_property(TagLib_INCLUDE_DIR_INTERFACE tag INCLUDE_DIRECTORIES)
    target_include_directories(tag INTERFACE
      ${taglib_SOURCE_DIR}
      ${taglib_SOURCE_DIR}/taglib
      ${TagLib_INCLUDE_DIR_INTERFACE}
    )
  endif()
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
  if(taglib_POPULATED)
    add_library(TagLib::TagLib ALIAS tag)
  else()
    add_library(TagLib::TagLib UNKNOWN IMPORTED)
    set_target_properties(TagLib::TagLib PROPERTIES
      IMPORTED_LOCATION             "${TagLib_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${TagLib_INCLUDE_DIR}"
    )
  endif()
endif()
