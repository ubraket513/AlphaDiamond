# Build the existing product boundaries directly: core/search and portable data
# support are always available; model inference requires LibTorch; learner and
# orchestration targets require training. Runtime SDK installation exports only
# the explicitly selected runtime targets below, never replay or Trainer headers.
# Regression targets/fixtures and the accepted-only pipeline CLI were removed;
# benchmarks remain explicit executables and do not register automated test runs.

option(DIAMOND_BUILD_BENCHMARKS "Build explicit native benchmark executables" OFF)

set(SOO_CORE_SOURCES
    board.cpp
    encoder.cpp
    min_experts.cpp
    prior.cpp
    rules.cpp
    topology_gen.cpp
    topology_io.cpp
)

set(SOO_SEARCH_SOURCES
    batcher.cpp
    evaluator.cpp
    mcts.cpp
    mcts3p.cpp
    profile.cpp
    search_target_metrics.cpp
    selfplay.cpp
)

add_library(soo_core STATIC ${SOO_CORE_SOURCES})
add_library(alphadiamond::soo_core ALIAS soo_core)

target_include_directories(soo_core
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
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
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(soo_search PUBLIC cxx_std_20)
target_compile_options(soo_search PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# Shared runtime facilities keep their public headers and C++ namespaces.
add_library(diamond_support STATIC json.cpp)
add_library(alphadiamond::diamond_support ALIAS diamond_support)
add_library(diamond_artifact STATIC deployment_artifact.cpp)
add_library(alphadiamond::diamond_artifact ALIAS diamond_artifact)
add_library(diamond_rating STATIC rating.cpp rating_store.cpp)
add_library(alphadiamond::diamond_rating ALIAS diamond_rating)
target_link_libraries(diamond_artifact PUBLIC alphadiamond::soo_core alphadiamond::diamond_support)
target_link_libraries(diamond_rating PUBLIC alphadiamond::diamond_support)
foreach(runtime_target diamond_support diamond_artifact diamond_rating)
    target_include_directories(${runtime_target} PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_compile_features(${runtime_target} PUBLIC cxx_std_20)
    target_compile_options(${runtime_target} PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>)
endforeach()

add_executable(alphadiamond-rating-sync rating_sync_main.cpp)
target_link_libraries(alphadiamond-rating-sync PRIVATE alphadiamond::diamond_rating)
target_compile_features(alphadiamond-rating-sync PRIVATE cxx_std_20)
# Runtime SDK installation below owns this executable's single install rule.

if(DIAMOND_BUILD_MODEL)
    find_package(Torch REQUIRED)
    include(${PROJECT_SOURCE_DIR}/cmake/AlphaDiamondEngineTorch.cmake)
    separate_arguments(DIAMOND_TORCH_CXX_FLAGS NATIVE_COMMAND "${TORCH_CXX_FLAGS}")
    add_library(diamond_model STATIC
        soo_model.cpp
        soo_evaluator.cpp
        min_evaluator.cpp
    )
    add_library(alphadiamond::diamond_model ALIAS diamond_model)
    target_link_libraries(diamond_model PUBLIC alphadiamond::soo_core alphadiamond::diamond_artifact Torch::Torch)
    target_include_directories(diamond_model
        PUBLIC
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )
    target_compile_features(diamond_model PUBLIC cxx_std_20)
    target_compile_options(diamond_model PUBLIC ${DIAMOND_TORCH_CXX_FLAGS})
    target_compile_definitions(diamond_model PUBLIC _DISABLE_EXTENDED_ALIGNED_STORAGE)
    if(MSVC)
        target_compile_options(diamond_model PRIVATE /bigobj)
    endif()

    if(DIAMOND_BUILD_BENCHMARKS)
        add_executable(min_moe_probe "${PROJECT_SOURCE_DIR}/benchmarks/min_moe_probe.cpp")
        target_link_libraries(min_moe_probe PRIVATE alphadiamond::diamond_model alphadiamond::soo_search)
        add_executable(soo_libtorch_probe "${PROJECT_SOURCE_DIR}/benchmarks/soo_libtorch_probe.cpp")
        target_link_libraries(soo_libtorch_probe PRIVATE alphadiamond::diamond_model alphadiamond::soo_search ${TORCH_LIBRARIES})
        target_compile_features(soo_libtorch_probe PRIVATE cxx_std_20)
        target_include_directories(soo_libtorch_probe PRIVATE ${TORCH_INCLUDE_DIRS})
        target_compile_definitions(soo_libtorch_probe PRIVATE _DISABLE_EXTENDED_ALIGNED_STORAGE)
        if(MSVC)
            target_compile_options(soo_libtorch_probe PRIVATE /bigobj)
        endif()

        add_executable(soo_evaluator_probe "${PROJECT_SOURCE_DIR}/benchmarks/soo_evaluator_probe.cpp")
        target_link_libraries(soo_evaluator_probe PRIVATE alphadiamond::diamond_model)
        target_compile_features(soo_evaluator_probe PRIVATE cxx_std_20)
        target_compile_definitions(soo_evaluator_probe PRIVATE _DISABLE_EXTENDED_ALIGNED_STORAGE)
        if(MSVC)
            target_compile_options(soo_evaluator_probe PRIVATE /bigobj)
        endif()

        add_executable(soo_mcts_probe "${PROJECT_SOURCE_DIR}/benchmarks/soo_mcts_probe.cpp")
        target_link_libraries(soo_mcts_probe PRIVATE alphadiamond::diamond_model alphadiamond::soo_search)
        target_compile_features(soo_mcts_probe PRIVATE cxx_std_20)
        target_compile_definitions(soo_mcts_probe PRIVATE _DISABLE_EXTENDED_ALIGNED_STORAGE)
        if(MSVC)
            target_compile_options(soo_mcts_probe PRIVATE /bigobj)
        endif()
    endif()

endif()

if(DIAMOND_BUILD_TRAINING)
    # Portable replay/schema code is needed by training, not Runtime SDK builds.
    add_library(diamond_pipeline STATIC
        policy_diagnostics.cpp
        replay_segment.cpp
        replay_store.cpp
        training_sample.cpp
    )
    add_library(alphadiamond::diamond_pipeline ALIAS diamond_pipeline)
    target_link_libraries(diamond_pipeline PUBLIC alphadiamond::soo_core alphadiamond::diamond_support)
    target_include_directories(diamond_pipeline PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )
    target_compile_features(diamond_pipeline PUBLIC cxx_std_20)
    target_compile_options(diamond_pipeline PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
    )

    add_library(diamond_training STATIC
        checkpoint.cpp
        checkpoint_export.cpp
        device.cpp
        parameter_diagnostics.cpp
        trainer.cpp
    )
    add_library(alphadiamond::diamond_training ALIAS diamond_training)
    # Compatibility/sample schema has one compiled owner: diamond_pipeline.
    target_link_libraries(diamond_training PUBLIC alphadiamond::diamond_model alphadiamond::diamond_pipeline ${TORCH_LIBRARIES})
    target_include_directories(diamond_training
        PUBLIC
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
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

    add_library(diamond_release STATIC promotion.cpp)
    add_library(alphadiamond::diamond_release ALIAS diamond_release)
    target_link_libraries(diamond_release PUBLIC alphadiamond::diamond_training)
    target_include_directories(diamond_release PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    )
    target_compile_features(diamond_release PUBLIC cxx_std_20)

    add_executable(alphadiamond-checkpoint checkpoint_main.cpp)
    target_link_libraries(alphadiamond-checkpoint PRIVATE alphadiamond::diamond_training)
    target_compile_features(alphadiamond-checkpoint PRIVATE cxx_std_20)

    add_executable(alphadiamond-release release_main.cpp)
    target_link_libraries(alphadiamond-release PRIVATE alphadiamond::diamond_release)
    target_compile_features(alphadiamond-release PRIVATE cxx_std_20)

    install(TARGETS alphadiamond-checkpoint alphadiamond-release
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    # The direct native self-play -> replay -> training path is deliberately
    # separate from the schema-only replay library: it owns LibTorch models.
    add_library(diamond_native_pipeline STATIC
        model_pool.cpp
        pipeline.cpp
    )
    add_library(alphadiamond::diamond_native_pipeline ALIAS diamond_native_pipeline)
    target_link_libraries(diamond_native_pipeline PUBLIC
        alphadiamond::diamond_pipeline
        alphadiamond::diamond_training
        alphadiamond::soo_search
        ${TORCH_LIBRARIES}
    )
    target_include_directories(diamond_native_pipeline PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        ${TORCH_INCLUDE_DIRS}
    )
    target_compile_features(diamond_native_pipeline PUBLIC cxx_std_20)
    target_compile_definitions(diamond_native_pipeline PUBLIC _DISABLE_EXTENDED_ALIGNED_STORAGE)
    if(MSVC)
        target_compile_options(diamond_native_pipeline PRIVATE /bigobj)
    endif()

    add_library(diamond_orchestration STATIC
        config.cpp
        coordinator.cpp
        report.cpp
        run_state.cpp
        schedule.cpp
        training_resources.cpp
        training_wiring.cpp
    )
    add_library(alphadiamond::diamond_orchestration ALIAS diamond_orchestration)
    target_link_libraries(diamond_orchestration PUBLIC alphadiamond::diamond_pipeline alphadiamond::diamond_rating)
    target_include_directories(diamond_orchestration PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    )
    target_compile_features(diamond_orchestration PUBLIC cxx_std_20)

    if(DIAMOND_BUILD_BENCHMARKS)
        add_executable(training_step_benchmark "${PROJECT_SOURCE_DIR}/benchmarks/training_step_benchmark.cpp")
        target_link_libraries(training_step_benchmark PRIVATE
            alphadiamond::diamond_training alphadiamond::diamond_orchestration
            alphadiamond::diamond_native_pipeline)

        add_executable(checkpoint_benchmark "${PROJECT_SOURCE_DIR}/benchmarks/checkpoint_benchmark.cpp")
        target_link_libraries(checkpoint_benchmark PRIVATE alphadiamond::diamond_training)

        add_executable(replay_benchmark "${PROJECT_SOURCE_DIR}/benchmarks/replay_benchmark.cpp")
        target_link_libraries(replay_benchmark PRIVATE alphadiamond::diamond_pipeline)

        add_executable(recent_replay_probe "${PROJECT_SOURCE_DIR}/benchmarks/recent_replay_probe.cpp")
        target_link_libraries(recent_replay_probe PRIVATE alphadiamond::diamond_pipeline)

        add_executable(weakness_replay_probe "${PROJECT_SOURCE_DIR}/benchmarks/weakness_replay_probe.cpp")
        target_link_libraries(weakness_replay_probe PRIVATE alphadiamond::diamond_native_pipeline)

        add_executable(recent_replay_integration "${PROJECT_SOURCE_DIR}/benchmarks/recent_replay_integration.cpp")
        target_link_libraries(recent_replay_integration PRIVATE
            alphadiamond::diamond_native_pipeline alphadiamond::diamond_orchestration)

        add_executable(selfplay_benchmark "${PROJECT_SOURCE_DIR}/benchmarks/selfplay_benchmark.cpp")
        target_link_libraries(selfplay_benchmark PRIVATE
            alphadiamond::diamond_native_pipeline
            alphadiamond::diamond_orchestration)

        # These experiments own their helpers; ordinary training does not compile
        # diagnostic learning loops or vacancy distillation into its library.
        add_executable(min_learning_diagnostic
            "${PROJECT_SOURCE_DIR}/benchmarks/min_learning_diagnostic.cpp"
            "${PROJECT_SOURCE_DIR}/benchmarks/learning_diagnostic.cpp")
        target_link_libraries(min_learning_diagnostic PRIVATE
            alphadiamond::diamond_native_pipeline alphadiamond::diamond_orchestration)

        add_executable(min_vacancy_realizability
            "${PROJECT_SOURCE_DIR}/benchmarks/min_vacancy_realizability.cpp"
            "${PROJECT_SOURCE_DIR}/benchmarks/vacancy_distillation.cpp")
        target_link_libraries(min_vacancy_realizability PRIVATE
            alphadiamond::diamond_native_pipeline alphadiamond::diamond_orchestration)

        add_executable(min_expert_arena "${PROJECT_SOURCE_DIR}/benchmarks/min_expert_arena.cpp")
        target_link_libraries(min_expert_arena PRIVATE
            alphadiamond::diamond_native_pipeline alphadiamond::diamond_orchestration)

        add_executable(min_reanalysis "${PROJECT_SOURCE_DIR}/benchmarks/min_reanalysis.cpp")
        target_link_libraries(min_reanalysis PRIVATE
            alphadiamond::diamond_native_pipeline alphadiamond::diamond_orchestration)

        add_executable(end_to_end_benchmark "${PROJECT_SOURCE_DIR}/benchmarks/end_to_end_benchmark.cpp")
        target_link_libraries(end_to_end_benchmark PRIVATE
            alphadiamond::diamond_native_pipeline)

        foreach(benchmark IN ITEMS
                training_step_benchmark checkpoint_benchmark replay_benchmark
                selfplay_benchmark min_learning_diagnostic min_vacancy_realizability
                min_expert_arena end_to_end_benchmark)
            target_compile_features(${benchmark} PRIVATE cxx_std_20)
        endforeach()
    endif()

    add_executable(alphadiamond-train train_main.cpp)
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

    if(DIAMOND_BUILD_BENCHMARKS)
        # Which prior carries Min out of a cold start: Soo's policy through a 3P
        # adapter, the vacancy heuristic, or nothing.  Self-play only -- it never
        # touches the arena or a run directory.
        add_executable(alphadiamond-min-probe "${PROJECT_SOURCE_DIR}/benchmarks/min_bootstrap_probe.cpp")
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

endif()

include(${PROJECT_SOURCE_DIR}/cmake/InstallEngineSdk.cmake)
