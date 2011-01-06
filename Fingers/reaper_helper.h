#include <sstream>
#include "reaper_plugin_functions.h"
extern void (*ShowConsoleMsg)(const char* msg);
#define REAPERMSG(x) {  std::stringstream oss; oss << x << std::endl; ShowConsoleMsg(oss.str().c_str());}
#define __ARRAY_SIZE(x) sizeof(x) / sizeof(x[0])