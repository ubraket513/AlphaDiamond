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
    list(APPEND _ad_public_headers diamond_model/soo_model.hpp diamond_model/soo_evaluator.hpp)
endif()
foreach(_header IN LISTS _ad_public_headers)
    get_filename_component(_directory "${_header}" DIRECTORY)
    install(FILES "${PROJECT_SOURCE_DIR}/native/include/${_header}"
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
install(DIRECTORY "${PROJECT_SOURCE_DIR}/tests/golden/topology/"
    DESTINATION "${_ad_data_dir}/topology" COMPONENT Runtime)

if(TARGET alphadiamond-rating-sync)
    install(TARGETS alphadiamond-rating-sync
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT Runtime)
endif()
if(ALPHADIAMOND_ENGINE_HAS_MODEL)
    add_executable(alphadiamond-sdk-fixtures
        "${PROJECT_SOURCE_DIR}/native/tests/sdk_consumer/generate_fixtures.cpp")
    target_link_libraries(alphadiamond-sdk-fixtures PRIVATE alphadiamond::diamond_artifact)
    set(_ad_fixtures "${PROJECT_BINARY_DIR}/sdk-fixtures")
    add_custom_command(OUTPUT "${_ad_fixtures}/PROVENANCE.txt"
        COMMAND alphadiamond-sdk-fixtures "${PROJECT_SOURCE_DIR}/tests/golden/topology" "${_ad_fixtures}"
        DEPENDS alphadiamond-sdk-fixtures VERBATIM)
    add_custom_target(engine_sdk_data ALL DEPENDS "${_ad_fixtures}/PROVENANCE.txt")
    install(DIRECTORY "${_ad_fixtures}/" DESTINATION "${_ad_data_dir}/fixtures" COMPONENT Runtime)
endif()

execute_process(COMMAND git rev-parse HEAD WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    OUTPUT_VARIABLE ALPHADIAMOND_ENGINE_SOURCE_SHA OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _ad_git_status)
if(NOT _ad_git_status EQUAL 0)
    message(FATAL_ERROR "SDK provenance requires a Git source SHA")
endif()
execute_process(COMMAND git status --porcelain --untracked-files=normal
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}" OUTPUT_VARIABLE _ad_dirty)
set(ALPHADIAMOND_ENGINE_SOURCE_DIRTY false)
if(_ad_dirty)
    set(ALPHADIAMOND_ENGINE_SOURCE_DIRTY true)
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

if(BUILD_TESTING AND DIAMOND_BUILD_NATIVE_TESTS AND ALPHADIAMOND_ENGINE_HAS_MODEL)
    add_test(NAME engine_sdk_install COMMAND "${CMAKE_COMMAND}"
        "-DBUILD_DIR=${PROJECT_BINARY_DIR}" "-DSOURCE_DIR=${PROJECT_SOURCE_DIR}"
        "-DCONFIG=$<CONFIG>" "-DCOMPILER=${CMAKE_CXX_COMPILER}"
        "-DCXX_FLAGS=${CMAKE_CXX_FLAGS}" "-DEXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS}"
        "-DCOMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}"
        "-DTorch_DIR=${Torch_DIR}" "-DGENERATOR=${CMAKE_GENERATOR}"
        -P "${PROJECT_SOURCE_DIR}/native/tests/sdk_install_test.cmake")
    set_tests_properties(engine_sdk_install PROPERTIES LABELS "sdk;integration"
        ENVIRONMENT "OMP_NUM_THREADS=2;MKL_NUM_THREADS=2" PROCESSORS 8 TIMEOUT 300)
endif()
