include_guard(GLOBAL)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(ALPHADIAMOND_ENGINE_VERSION 0.1.0)
set(_ad_sdk_targets soo_core soo_search diamond_support diamond_artifact diamond_rating)
set(ALPHADIAMOND_ENGINE_HAS_MODEL OFF)
if(TARGET diamond_model)
    list(APPEND _ad_sdk_targets diamond_model)
    set(ALPHADIAMOND_ENGINE_HAS_MODEL ON)
endif()
if(ALPHADIAMOND_ENGINE_HAS_MODEL AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include("${CMAKE_CURRENT_LIST_DIR}/AlphaDiamondEngineAbi.cmake")
    alphadiamond_engine_apply_abi(TARGETS ${_ad_sdk_targets})
endif()
foreach(_target IN LISTS _ad_sdk_targets)
    if(MSVC)
        include(CheckCXXCompilerFlag)
        check_cxx_compiler_flag("/std:c++latest" _ad_has_latest)
        if(NOT _ad_has_latest)
            message(FATAL_ERROR "AlphaDiamondEngine requires MSVC /std:c++latest")
        endif()
        target_compile_features(${_target} PUBLIC cxx_std_23)
        target_compile_options(${_target} PUBLIC /std:c++latest)
        set_target_properties(${_target} PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
    elseif("cxx_std_26" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
        target_compile_features(${_target} PUBLIC cxx_std_26)
        set_target_properties(${_target} PROPERTIES CXX_STANDARD 26 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
    elseif(ALPHADIAMOND_ENGINE_HAS_MODEL)
        message(FATAL_ERROR "AlphaDiamondEngine Runtime SDK requires CMake >= 3.30 and a C++26 compiler")
    endif()
endforeach()

set(_ad_package_dir "${CMAKE_INSTALL_LIBDIR}/cmake/AlphaDiamondEngine")
set(_ad_data_dir "${CMAKE_INSTALL_DATADIR}/AlphaDiamondEngine")
include("${CMAKE_CURRENT_LIST_DIR}/AlphaDiamondEngineBuildProvenance.cmake")
file(GLOB_RECURSE _ad_provenance_inputs CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/include/*.hpp"
    "${PROJECT_SOURCE_DIR}/include/*.h"
    "${PROJECT_SOURCE_DIR}/src/*.hpp" "${PROJECT_SOURCE_DIR}/src/*.h"
    "${PROJECT_SOURCE_DIR}/cmake/*.cmake" "${PROJECT_SOURCE_DIR}/cmake/*.cmake.in"
    "${PROJECT_SOURCE_DIR}/data/topology/*")
list(APPEND _ad_provenance_inputs "${PROJECT_SOURCE_DIR}/CMakeLists.txt"
    "${PROJECT_SOURCE_DIR}/src/CMakeLists.txt")
set(_ad_provenance_targets ${_ad_sdk_targets})
if(TARGET alphadiamond-rating-sync)
    list(APPEND _ad_provenance_targets alphadiamond-rating-sync)
endif()
alphadiamond_engine_track_provenance(TARGETS ${_ad_provenance_targets}
    INPUTS ${_ad_provenance_inputs})
install(TARGETS ${_ad_sdk_targets} EXPORT AlphaDiamondEngineTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT Runtime
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT Runtime
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT Runtime)
install(EXPORT AlphaDiamondEngineTargets NAMESPACE alphadiamond::
    FILE AlphaDiamondEngineTargets.cmake DESTINATION "${_ad_package_dir}" COMPONENT Runtime)

# An explicit public surface: no replay/trainer/coordinator/checkpoint headers.
set(_ad_public_headers
    soo/action.hpp soo/arena_router.hpp soo/batcher.hpp soo/board.hpp soo/encoder.hpp
    soo/episode_search.hpp soo/evaluator.hpp soo/mcts.hpp soo/mcts3p.hpp
    soo/min_experts.hpp soo/prior.hpp soo/profile.hpp soo/random.hpp soo/rules.hpp
    soo/search_target_metrics.hpp soo/selfplay.hpp soo/state.hpp soo/tree.hpp
    diamond_support/json.hpp diamond_model/deployment_artifact.hpp diamond_model/model_index.hpp
    diamond_orchestration/rating.hpp diamond_orchestration/rating_store.hpp)
if(ALPHADIAMOND_ENGINE_HAS_MODEL)
    list(APPEND _ad_public_headers diamond_model/soo_model.hpp diamond_model/soo_evaluator.hpp
        diamond_model/min_evaluator.hpp)
endif()
foreach(_header IN LISTS _ad_public_headers)
    get_filename_component(_directory "${_header}" DIRECTORY)
    install(FILES "${PROJECT_SOURCE_DIR}/include/${_header}"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${_directory}" COMPONENT Runtime)
endforeach()

configure_package_config_file("${CMAKE_CURRENT_LIST_DIR}/AlphaDiamondEngineConfig.cmake.in"
    "${PROJECT_BINARY_DIR}/AlphaDiamondEngineConfig.cmake"
    INSTALL_DESTINATION "${_ad_package_dir}")
write_basic_package_version_file("${PROJECT_BINARY_DIR}/AlphaDiamondEngineConfigVersion.cmake"
    VERSION "${ALPHADIAMOND_ENGINE_VERSION}" COMPATIBILITY ExactVersion)
install(FILES "${PROJECT_BINARY_DIR}/AlphaDiamondEngineConfig.cmake"
    "${PROJECT_BINARY_DIR}/AlphaDiamondEngineConfigVersion.cmake"
    "${CMAKE_CURRENT_LIST_DIR}/AlphaDiamondEngineTorch.cmake"
    DESTINATION "${_ad_package_dir}" COMPONENT Runtime)
# These five normative tables are embedded by Desktop at runtime. They remain
# SDK runtime data, independent of the removed regression corpus.
install(DIRECTORY "${PROJECT_SOURCE_DIR}/data/topology/"
    DESTINATION "${_ad_data_dir}/topology" COMPONENT Runtime)

if(TARGET alphadiamond-rating-sync)
    install(TARGETS alphadiamond-rating-sync
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT Runtime)
endif()

set(ALPHADIAMOND_ENGINE_CRT "${CMAKE_MSVC_RUNTIME_LIBRARY}")
if(MSVC AND NOT ALPHADIAMOND_ENGINE_CRT)
    set(ALPHADIAMOND_ENGINE_CRT "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
elseif(NOT MSVC)
    set(ALPHADIAMOND_ENGINE_CRT "native")
endif()
configure_file("${CMAKE_CURRENT_LIST_DIR}/AlphaDiamondEngineManifest.cmake.in"
    "${PROJECT_BINARY_DIR}/AlphaDiamondEngineManifest.cmake" @ONLY)
install(SCRIPT "${PROJECT_BINARY_DIR}/AlphaDiamondEngineManifest.cmake" COMPONENT Runtime)
