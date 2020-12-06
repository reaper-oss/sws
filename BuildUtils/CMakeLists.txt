set(LANGPACK_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/SWS_Template.ReaperLangPack)

get_property(LANGPACK_SOURCES TARGET sws PROPERTY SOURCES)
list(REMOVE_ITEM LANGPACK_SOURCES reascript_vararg.h)
list(TRANSFORM LANGPACK_SOURCES PREPEND "${CMAKE_SOURCE_DIR}/" REGEX "^[^/][^:]")

add_executable(buildlangpack EXCLUDE_FROM_ALL BuildLangpack.cpp)
find_package(WDL REQUIRED)
target_include_directories(buildlangpack PRIVATE ${WDL_INCLUDE_DIR})

add_custom_command(
  OUTPUT ${LANGPACK_OUTPUT}
  COMMAND $<TARGET_FILE:buildlangpack> --template ${LANGPACK_SOURCES}
    > ${LANGPACK_OUTPUT}
  DEPENDS $<TARGET_FILE:buildlangpack> ${LANGPACK_SOURCES}
  COMMAND_EXPAND_LISTS
)

add_custom_target(langpack DEPENDS ${LANGPACK_OUTPUT})
add_dependencies(langpack buildlangpack)

set(WHATSNEW_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/whatsnew.html)
set(WHATSNEW_SOURCE ${CMAKE_SOURCE_DIR}/whatsnew.txt)

add_executable(makewhatsnew EXCLUDE_FROM_ALL MakeWhatsNew.cpp)
target_compile_features(makewhatsnew PRIVATE cxx_std_11)
target_include_directories(makewhatsnew PRIVATE ${WDL_INCLUDE_DIR})

add_custom_command(
  OUTPUT ${WHATSNEW_OUTPUT}
  COMMAND $<TARGET_FILE:makewhatsnew> -h ${WHATSNEW_SOURCE} ${WHATSNEW_OUTPUT}
  DEPENDS $<TARGET_FILE:makewhatsnew> ${WHATSNEW_SOURCE}
  COMMAND_EXPAND_LISTS
)

add_custom_target(whatsnew DEPENDS ${WHATSNEW_OUTPUT})
add_dependencies(whatsnew makewhatsnew)
