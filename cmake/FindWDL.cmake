find_path(WDL_INCLUDE_DIR
  NAMES WDL/wdltypes.h
  PATHS vendor/WDL
)

set(WDL_DIR "${WDL_INCLUDE_DIR}/WDL")

add_library(WDL
  ${WDL_DIR}/projectcontext.cpp

  ${WDL_DIR}/wingui/virtwnd-iconbutton.cpp
  ${WDL_DIR}/wingui/virtwnd-slider.cpp
  ${WDL_DIR}/wingui/virtwnd.cpp
  ${WDL_DIR}/wingui/virtwnd.h
  ${WDL_DIR}/wingui/wndsize.cpp

  $<$<BOOL:${WIN32}>:${WDL_DIR}/win32_utf8.c>
)

include(CheckCCompilerFlag)
check_c_compiler_flag(-Wno-deprecated-register "-Wdeprecated-register")

if(Wdeprecated-register)
  target_compile_options(WDL PUBLIC
    # Disable annoying warnings from MersenneTwister.h
    -Wno-deprecated-register

    # Ignore deprecated stat64 from dirscan.h
    -Wno-deprecated-declarations
  )
endif()

target_compile_definitions(WDL INTERFACE WDL_NO_DEFINE_MINMAX)
target_include_directories(WDL INTERFACE ${WDL_INCLUDE_DIR})

find_package(SWELL)
if(TARGET SWELL)
  target_link_libraries(WDL SWELL)
endif()
