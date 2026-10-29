# Build the existing product boundaries directly: core/search and portable data
# support are always available; model inference requires LibTorch; learner and
# orchestration targets require training. Runtime SDK installation exports only
# the explicitly selected runtime targets below, never replay or Trainer headers.
# Regression targets/fixtures and the accepted-only pipeline CLI were removed;
# benchmarks remain explicit executables and do not register automated test runs.

option(DIAMOND_BUILD_BENCHMARKS "Build explicit native benchmark executables" OFF)

set(SOO_CORE_SOURCES
    src/board.cpp
    src/encoder.cpp
    src/min_experts.cpp
    src/prior.cpp
    src/rules.cpp
    src/topology_gen.cpp
    src/topology_io.cpp
)

set(SOO_SEARCH_SOURCES
    src/batcher.cpp
    src/evaluator.cpp
    src/mcts.cpp
    src/mcts3p.cpp
    src/profile.cpp
    src/search_target_metrics.cpp
    src/selfplay.cpp
)

add_library(soo_core STATIC ${SOO_CORE_SOURCES})
add_library(alphadiamond::soo_core ALIAS soo_core)

target_include_directories(soo_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(soo_core PUBLIC cxx_std_20)
target_compile_options(soo_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

add_library(soo_search STATIC ${SOO_SEARCH_SOURCES})
add_library(alphadiamond::soo_search ALIAS soo_search)

target_link_libraries(soo_search
    PUBLIC
        alphadiamond::soo_core
        Threads::Threads
)

target_include_directories(soo_search
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(soo_search PUBLIC cxx_std_20)
target_compile_options(soo_search PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# Shared runtime facilities keep their public headers and C++ namespaces.
add_library(diamond_support STATIC src/json.cpp)
add_library(alphadiamond::diamond_support ALIAS diamond_support)
add_library(diamond_artifact STATIC src/deployment_artifact.cpp)
add_library(alphadiamond::diamond_artifact ALIAS diamond_artifact)
add_library(diamond_rating STATIC src/rating.cpp src/rating_store.cpp)
add_library(alphadiamond::diamond_rating ALIAS diamond_rating)
target_link_libraries(diamond_artifact PUBLIC alphadiamond::soo_core alphadiamond::diamond_support)
target_link_libraries(diamond_rating PUBLIC alphadiamond::diamond_support)
foreach(runtime_target diamond_support diamond_artifact diamond_rating)
    target_include_directories(${runtime_target} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_compile_features(${runtime_target} PUBLIC cxx_std_20)
    target_compile_options(${runtime_target} PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>)
endforeach()

add_executable(alphadiamond-rating-sync src/rating_sync_main.cpp)
target_link_libraries(alphadiamond-rating-sync PRIVATE alphadiamond::diamond_rating)
target_compile_features(alphadiamond-rating-sync PRIVATE cxx_std_20)
install(TARGETS alphadiamond-rating-sync RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_library(diamond_pipeline STATIC
    src/policy_diagnostics.cpp
    src/replay_segment.cpp
    src/replay_store.cpp
    src/training_sample.cpp
)
add_library(alphadiamond::diamond_pipeline ALIAS diamond_pipeline)
target_link_libraries(diamond_pipeline PUBLIC alphadiamond::soo_core alphadiamond::diamond_support)
target_include_directories(diamond_pipeline PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(diamond_pipeline PUBLIC cxx_std_20)
target_compile_options(diamond_pipeline PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

if(DIAMOND_BUILD_MODEL)
    find_package(Torch REQUIRED)
    include(${PROJECT_SOURCE_DIR}/cmake/AlphaDiamondEngineTorch.cmake)
    separate_arguments(DIAMOND_TORCH_CXX_FLAGS NATIVE_COMMAND "${TORCH_CXX_FLAGS}")
    add_library(diamond_model STATIC
        src/soo_model.cpp
        src/soo_evaluator.cpp
    )
    add_library(alphadiamond::diamond_model ALIAS diamond_model)
    target_link_libraries(diamond_model PUBLIC alphadiamond::soo_core alphadiamond::diamond_artifact Torch::Torch)
    target_include_directories(diamond_model
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )
    target_compile_features(diamond_model PUBLIC cxx_std_20)
    target_compile_options(diamond_model PUBLIC ${DIAMOND_TORCH_CXX_FLAGS})
    target_compile_definitions(diamond_model PUBLIC _DISABLE_EXTENDED_ALIGNED_STORAGE)
    if(MSVC)
        target_compile_options(diamond_model PRIVATE /bigobj)
    endif()

    add_executable(soo_libtorch_probe src/soo_libtorch_probe.cpp)
    target_link_libraries(soo_libtorch_probe PRIVATE alphadiamond::diamond_model alphadiamond::soo_search ${TORCH_LIBRARIES})
    target_compile_features(soo_libtorch_probe PRIVATE cxx_std_20)
    target_include_directories(soo_libtorch_probe PRIVATE ${TORCH_INCLUDE_DIRS})
    target_compile_definitions(soo_libtorch_probe PRIVATE _DISABLE_EXTENDED_ALIGNED_STORAGE)
    if(MSVC)
        target_compile_options(soo_libtorch_probe PRIVATE /bigobj)
    endif()

    add_executable(soo_evaluator_probe src/soo_evaluator_probe.cpp)
    target_link_libraries(soo_evaluator_probe PRIVATE alphadiamond::diamond_model)
    target_compile_features(soo_evaluator_probe PRIVATE cxx_std_20)
    target_compile_definitions(soo_evaluator_probe PRIVATE _DISABLE_EXTENDED_ALIGNED_STORAGE)
    if(MSVC)
        target_compile_options(soo_evaluator_probe PRIVATE /bigobj)
    endif()

    add_executable(soo_mcts_probe src/soo_mcts_probe.cpp)
    target_link_libraries(soo_mcts_probe PRIVATE alphadiamond::diamond_model alphadiamond::soo_search)
    target_compile_features(soo_mcts_probe PRIVATE cxx_std_20)
    target_compile_definitions(soo_mcts_probe PRIVATE _DISABLE_EXTENDED_ALIGNED_STORAGE)
    if(MSVC)
        target_compile_options(soo_mcts_probe PRIVATE /bigobj)
    endif()

endif()

if(DIAMOND_BUILD_TRAINING)
    add_library(diamond_training STATIC
        src/checkpoint.cpp
        src/checkpoint_export.cpp
        src/device.cpp
        src/parameter_diagnostics.cpp
        src/training_sample.cpp
        src/trainer.cpp
    )
    add_library(alphadiamond::diamond_training ALIAS diamond_training)
    target_link_libraries(diamond_training PUBLIC alphadiamond::diamond_model ${TORCH_LIBRARIES})
    target_include_directories(diamond_training
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            ${TORCH_INCLUDE_DIRS}
    )
    target_compile_features(diamond_training PUBLIC cxx_std_20)
    target_compile_definitions(diamond_training PUBLIC _DISABLE_EXTENDED_ALIGNED_STORAGE)
    if(CAFFE2_USE_CUDA)
        target_compile_definitions(diamond_training PRIVATE DIAMOND_TORCH_WITH_CUDA=1)
    endif()
    if(MSVC)
        target_compile_options(diamond_training PRIVATE /bigobj)
    endif()

    add_library(diamond_release STATIC src/promotion.cpp)
    add_library(alphadiamond::diamond_release ALIAS diamond_release)
    target_link_libraries(diamond_release PUBLIC alphadiamond::diamond_training)
    target_include_directories(diamond_release PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    )
    target_compile_features(diamond_release PUBLIC cxx_std_20)

    add_executable(alphadiamond-checkpoint src/checkpoint_main.cpp)
    target_link_libraries(alphadiamond-checkpoint PRIVATE alphadiamond::diamond_training)
    target_compile_features(alphadiamond-checkpoint PRIVATE cxx_std_20)

    add_executable(alphadiamond-release src/release_main.cpp)
    target_link_libraries(alphadiamond-release PRIVATE alphadiamond::diamond_release)
    target_compile_features(alphadiamond-release PRIVATE cxx_std_20)

    install(TARGETS alphadiamond-checkpoint alphadiamond-release
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    # The direct native self-play -> replay -> training path is deliberately
    # separate from the schema-only replay library: it owns LibTorch models.
    add_library(diamond_native_pipeline STATIC
        src/learning_diagnostic.cpp
        src/model_pool.cpp
        src/pipeline.cpp
        src/vacancy_distillation.cpp
    )
    add_library(alphadiamond::diamond_native_pipeline ALIAS diamond_native_pipeline)
    target_link_libraries(diamond_native_pipeline PUBLIC
        alphadiamond::diamond_pipeline
        alphadiamond::diamond_training
        alphadiamond::soo_search
        ${TORCH_LIBRARIES}
    )
    target_include_directories(diamond_native_pipeline PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        ${TORCH_INCLUDE_DIRS}
    )
    target_compile_features(diamond_native_pipeline PUBLIC cxx_std_20)
    target_compile_definitions(diamond_native_pipeline PUBLIC _DISABLE_EXTENDED_ALIGNED_STORAGE)
    if(MSVC)
        target_compile_options(diamond_native_pipeline PRIVATE /bigobj)
    endif()

    add_library(diamond_orchestration STATIC
        src/arena.cpp
        src/config.cpp
        src/coordinator.cpp
        src/report.cpp
        src/run_state.cpp
        src/schedule.cpp
        src/training_resources.cpp
        src/training_wiring.cpp
    )
    add_library(alphadiamond::diamond_orchestration ALIAS diamond_orchestration)
    target_link_libraries(diamond_orchestration PUBLIC alphadiamond::diamond_pipeline alphadiamond::diamond_rating)
    target_include_directories(diamond_orchestration PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    )
    target_compile_features(diamond_orchestration PUBLIC cxx_std_20)

    if(DIAMOND_BUILD_BENCHMARKS)
        add_executable(training_step_benchmark benchmarks/training_step_benchmark.cpp)
        target_link_libraries(training_step_benchmark PRIVATE
            alphadiamond::diamond_training alphadiamond::diamond_orchestration
            alphadiamond::diamond_native_pipeline)

        add_executable(checkpoint_benchmark benchmarks/checkpoint_benchmark.cpp)
        target_link_libraries(checkpoint_benchmark PRIVATE alphadiamond::diamond_training)

        add_executable(replay_benchmark benchmarks/replay_benchmark.cpp)
        target_link_libraries(replay_benchmark PRIVATE alphadiamond::diamond_pipeline)

        add_executable(selfplay_benchmark benchmarks/selfplay_benchmark.cpp)
        target_link_libraries(selfplay_benchmark PRIVATE
            alphadiamond::diamond_native_pipeline
            alphadiamond::diamond_orchestration)

        add_executable(min_learning_diagnostic benchmarks/min_learning_diagnostic.cpp)
        target_link_libraries(min_learning_diagnostic PRIVATE
            alphadiamond::diamond_native_pipeline alphadiamond::diamond_orchestration)

        add_executable(min_vacancy_realizability benchmarks/min_vacancy_realizability.cpp)
        target_link_libraries(min_vacancy_realizability PRIVATE
            alphadiamond::diamond_native_pipeline alphadiamond::diamond_orchestration)

        add_executable(min_expert_arena benchmarks/min_expert_arena.cpp)
        target_link_libraries(min_expert_arena PRIVATE
            alphadiamond::diamond_native_pipeline alphadiamond::diamond_orchestration)

        add_executable(end_to_end_benchmark benchmarks/end_to_end_benchmark.cpp)
        target_link_libraries(end_to_end_benchmark PRIVATE
            alphadiamond::diamond_native_pipeline)

        foreach(benchmark IN ITEMS
                training_step_benchmark checkpoint_benchmark replay_benchmark
                selfplay_benchmark min_learning_diagnostic min_vacancy_realizability
                min_expert_arena end_to_end_benchmark)
            target_compile_features(${benchmark} PRIVATE cxx_std_20)
        endforeach()
    endif()

    add_executable(alphadiamond-train src/train_main.cpp)
    find_package(Git QUIET)
    set(DIAMOND_SOURCE_GIT_COMMIT "unavailable")
    set(DIAMOND_SOURCE_DIRTY 1)
    if(Git_FOUND)
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            OUTPUT_VARIABLE DIAMOND_SOURCE_GIT_COMMIT
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE DIAMOND_GIT_REV_PARSE_RESULT)
        if(NOT DIAMOND_GIT_REV_PARSE_RESULT EQUAL 0)
            set(DIAMOND_SOURCE_GIT_COMMIT "unavailable")
        endif()
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" status --porcelain --untracked-files=normal
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            OUTPUT_VARIABLE DIAMOND_GIT_STATUS
            RESULT_VARIABLE DIAMOND_GIT_STATUS_RESULT)
        if(DIAMOND_GIT_STATUS_RESULT EQUAL 0 AND DIAMOND_GIT_STATUS STREQUAL "")
            set(DIAMOND_SOURCE_DIRTY 0)
        else()
            set(DIAMOND_SOURCE_DIRTY 1)
        endif()
    endif()
    if(DIAMOND_BUILD_BENCHMARKS)
        foreach(benchmark IN ITEMS
                training_step_benchmark checkpoint_benchmark replay_benchmark
                selfplay_benchmark min_vacancy_realizability min_expert_arena end_to_end_benchmark)
            target_compile_definitions(${benchmark} PRIVATE
                DIAMOND_SOURCE_GIT_COMMIT="${DIAMOND_SOURCE_GIT_COMMIT}"
                DIAMOND_SOURCE_DIRTY=${DIAMOND_SOURCE_DIRTY})
        endforeach()
    endif()
    target_link_libraries(alphadiamond-train PRIVATE
        alphadiamond::diamond_orchestration
        alphadiamond::diamond_native_pipeline)
    target_compile_features(alphadiamond-train PRIVATE cxx_std_20)
    target_compile_definitions(alphadiamond-train PRIVATE
        DIAMOND_SOURCE_GIT_COMMIT="${DIAMOND_SOURCE_GIT_COMMIT}"
        DIAMOND_SOURCE_DIRTY=${DIAMOND_SOURCE_DIRTY})
    install(TARGETS alphadiamond-train
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    # Which prior carries Min out of a cold start: Soo's policy through a 3P
    # adapter, the vacancy heuristic, or nothing.  Self-play only -- it never
    # touches the arena or a run directory.
    add_executable(alphadiamond-min-probe src/min_bootstrap_probe.cpp)
    target_link_libraries(alphadiamond-min-probe
        PRIVATE alphadiamond::diamond_model alphadiamond::soo_search
                alphadiamond::diamond_training alphadiamond::diamond_native_pipeline
                ${TORCH_LIBRARIES})
    target_compile_features(alphadiamond-min-probe PRIVATE cxx_std_20)
    target_include_directories(alphadiamond-min-probe PRIVATE ${TORCH_INCLUDE_DIRS})
    target_compile_definitions(alphadiamond-min-probe PRIVATE _DISABLE_EXTENDED_ALIGNED_STORAGE)
    if(MSVC)
        target_compile_options(alphadiamond-min-probe PRIVATE /bigobj)
    endif()

endif()

include(${PROJECT_SOURCE_DIR}/cmake/InstallEngineSdk.cmake)
