# Child regression projects inherit the configured build, rather than choosing
# development-only executables such as Ninja or mold themselves.
if(NOT CONFIG)
    set(CONFIG Debug)
endif()
if(NOT JOBS)
    if(DEFINED ENV{CMAKE_BUILD_PARALLEL_LEVEL})
        set(JOBS "$ENV{CMAKE_BUILD_PARALLEL_LEVEL}")
    else()
        set(JOBS 8)
    endif()
endif()
set(_sdk_configure_options "-DCMAKE_BUILD_TYPE=${CONFIG}")
foreach(_pair "GENERATOR|-G" "GENERATOR_PLATFORM|-A" "GENERATOR_TOOLSET|-T")
    string(REPLACE "|" ";" _parts "${_pair}")
    list(GET _parts 0 _input)
    list(GET _parts 1 _option)
    if(DEFINED ${_input} AND NOT "${${_input}}" STREQUAL "")
        list(APPEND _sdk_configure_options "${_option}" "${${_input}}")
    endif()
endforeach()
foreach(_pair "COMPILER|CMAKE_CXX_COMPILER" "COMPILER_LAUNCHER|CMAKE_CXX_COMPILER_LAUNCHER"
    "GENERATOR_INSTANCE|CMAKE_GENERATOR_INSTANCE" "CXX_FLAGS|CMAKE_CXX_FLAGS"
    "EXE_LINKER_FLAGS|CMAKE_EXE_LINKER_FLAGS" "DEBUG_CXX_FLAGS|CMAKE_CXX_FLAGS_DEBUG"
    "RELEASE_CXX_FLAGS|CMAKE_CXX_FLAGS_RELEASE" "RELWITHDEBINFO_CXX_FLAGS|CMAKE_CXX_FLAGS_RELWITHDEBINFO"
    "MINSIZEREL_CXX_FLAGS|CMAKE_CXX_FLAGS_MINSIZEREL")
    string(REPLACE "|" ";" _parts "${_pair}")
    list(GET _parts 0 _input)
    list(GET _parts 1 _output)
    if(DEFINED ${_input})
        list(APPEND _sdk_configure_options "-D${_output}=${${_input}}")
    endif()
endforeach()
