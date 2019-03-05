find_path(TagLib_INCLUDE_DIR NAMES taglib.h PATH_SUFFIXES taglib)
find_library(TagLib_LIBRARY tag)
mark_as_advanced(TagLib_INCLUDE_DIR TagLib_LIBRARY)

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
  VERSION_VAR TagLib_VERSION
)

if(TagLib_FOUND AND NOT TARGET TagLib::TagLib)
  add_library(TagLib::TagLib UNKNOWN IMPORTED)
  set_target_properties(TagLib::TagLib PROPERTIES
    IMPORTED_LOCATION "${TagLib_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${TagLib_INCLUDE_DIR}"
  )
endif()
