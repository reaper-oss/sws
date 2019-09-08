find_program(PHP_EXECUTABLE php)
mark_as_advanced(PHP_EXECUTABLE)

execute_process(
  COMMAND ${PHP_EXECUTABLE} -v
  OUTPUT_VARIABLE PHP_VERSION_OUTPUT
  ERROR_QUIET
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(PHP_VERSION_OUTPUT MATCHES "PHP ([^ ]+) ")
  set(PHP_VERSION "${CMAKE_MATCH_1}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PHP
  REQUIRED_VARS PHP_EXECUTABLE
  VERSION_VAR   PHP_VERSION
)
