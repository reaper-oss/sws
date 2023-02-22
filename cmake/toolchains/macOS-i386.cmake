set(NO_INSTALL_PREFIX YES)
set(CMAKE_OSX_ARCHITECTURES i386)
set(CMAKE_C_FLAGS -m32)
set(CMAKE_CXX_FLAGS -stdlib=libc++ -m32)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.7)
# ld: dynamic executables or dylibs must link with libSystem.dylib for architecture i386
# 32bit builds are deprecated in newer macOS releases
set(LINK_FLAGS "-L/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/lib -lSystem")
