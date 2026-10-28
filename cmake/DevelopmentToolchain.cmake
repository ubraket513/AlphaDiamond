include_guard(GLOBAL)

# Development builds should compile quickly; optimized binaries are selected by
# an explicit Release configuration. Keep platform/compiler defaults elsewhere.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build configuration" FORCE)
endif()
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "Enable link-time optimization explicitly")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compiler commands")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g1" CACHE STRING "GNU development compiler flags" FORCE)
    add_compile_options("$<$<COMPILE_LANG_AND_ID:CXX,GNU>:-pipe>")
endif()
