set(LANGPACK_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/SWS_Template.ReaperLangPack)

add_executable(buildlangpack EXCLUDE_FROM_ALL BuildLangpack.cpp)

find_package(WDL REQUIRED)
target_include_directories(buildlangpack PRIVATE ${WDL_INCLUDE_DIR})

get_property(LANGPACK_SOURCES TARGET sws PROPERTY SOURCES)
list(REMOVE_ITEM LANGPACK_SOURCES reascript_vararg.h)
list(TRANSFORM LANGPACK_SOURCES PREPEND "${CMAKE_SOURCE_DIR}/" REGEX "^[^/][^:]")

add_custom_command(
  OUTPUT ${LANGPACK_OUTPUT}
  COMMAND $<TARGET_FILE:buildlangpack> --template ${LANGPACK_SOURCES}
    > ${LANGPACK_OUTPUT}
  DEPENDS ${LANGPACK_SOURCES}
  COMMAND_EXPAND_LISTS
)

add_custom_target(langpack DEPENDS ${LANGPACK_OUTPUT})
add_dependencies(langpack buildlangpack)

if(WIN32)
  add_executable(makewhatsnew EXCLUDE_FROM_ALL MakeWhatsNew.cpp)
  target_include_directories(makewhatsnew PRIVATE ${WDL_INCLUDE_DIR})

  add_executable(printversion EXCLUDE_FROM_ALL PrintVersion.cpp)
  target_include_directories(printversion PRIVATE ${WDL_INCLUDE_DIR})
endif()
