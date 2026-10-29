# Torch's upstream config exposes some dependencies through variables. Wrap
# those values in an imported target on BOTH producer and consumer machines;
# only this target name is written into AlphaDiamondEngineTargets.cmake.
if(NOT TARGET Torch::Torch)
    if(NOT TARGET torch)
        message(FATAL_ERROR "AlphaDiamondEngine requires find_package(Torch) first")
    endif()
    add_library(Torch::Torch INTERFACE IMPORTED)
    set_property(TARGET Torch::Torch PROPERTY INTERFACE_LINK_LIBRARIES "${TORCH_LIBRARIES}")
    set_property(TARGET Torch::Torch PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${TORCH_INCLUDE_DIRS}")
    separate_arguments(_alphadiamond_torch_flags NATIVE_COMMAND "${TORCH_CXX_FLAGS}")
    set_property(TARGET Torch::Torch PROPERTY INTERFACE_COMPILE_OPTIONS "${_alphadiamond_torch_flags}")
endif()

# Distribution identity, distinct from whether a test runs on a CUDA device.
set(AlphaDiamondEngine_TORCH_VARIANT cpu)
if(CAFFE2_USE_CUDA OR TARGET torch_cuda OR TORCH_CUDA_LIBRARIES)
    set(AlphaDiamondEngine_TORCH_VARIANT cuda)
endif()
set(AlphaDiamondEngine_TORCH_ABI native)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(AlphaDiamondEngine_TORCH_ABI "cxx11-1")
    if("${TORCH_CXX_FLAGS}" MATCHES "_GLIBCXX_USE_CXX11_ABI=([01])")
        set(AlphaDiamondEngine_TORCH_ABI "cxx11-${CMAKE_MATCH_1}")
    elseif(DEFINED GLIBCXX_USE_CXX11_ABI)
        set(AlphaDiamondEngine_TORCH_ABI "cxx11-${GLIBCXX_USE_CXX11_ABI}")
    endif()
endif()
