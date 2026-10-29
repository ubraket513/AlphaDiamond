include_guard(GLOBAL)
function(alphadiamond_engine_track_provenance)
    cmake_parse_arguments(PROVENANCE "" "" "TARGETS;INPUTS;DATA_TARGETS;DATA_DIRS" ${ARGN})
    set(_root "${PROJECT_BINARY_DIR}/sdk-build-provenance")
    set(_script "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/AlphaDiamondEngineProvenance.cmake")
    set(_inputs ${PROVENANCE_INPUTS})
    set(_binaries)
    foreach(_target IN LISTS PROVENANCE_TARGETS)
        get_target_property(_sources ${_target} SOURCES)
        get_target_property(_source_dir ${_target} SOURCE_DIR)
        foreach(_source IN LISTS _sources)
            if(_source MATCHES "\\$<")
                message(FATAL_ERROR "SDK provenance needs an explicit source path: ${_target}: ${_source}")
            endif()
            get_filename_component(_absolute "${_source}" ABSOLUTE BASE_DIR "${_source_dir}")
            list(APPEND _inputs "${_absolute}")
            get_filename_component(_source_parent "${_absolute}" DIRECTORY)
            file(GLOB _private_headers CONFIGURE_DEPENDS
                "${_source_parent}/*.hpp" "${_source_parent}/*.h" "${_source_parent}/*.inl")
            list(APPEND _inputs ${_private_headers})
        endforeach()
        list(APPEND _binaries "$<TARGET_FILE:${_target}>")
    endforeach()
    list(REMOVE_DUPLICATES _inputs)
    list(SORT _inputs)
    file(GENERATE OUTPUT "${_root}/$<CONFIG>/inputs.cmake" CONTENT
        "set(AD_SOURCE_ROOT [==[${PROJECT_SOURCE_DIR}]==])\nset(AD_SOURCE_INPUTS [==[${_inputs}]==])\nset(AD_BINARY_INPUTS [==[${_binaries}]==])\nset(AD_DATA_DIRS [==[${PROVENANCE_DATA_DIRS}]==])\n")
    add_custom_target(engine_sdk_source_snapshot
        COMMAND "${CMAKE_COMMAND}" "-DAD_INPUT_FILE=${_root}/$<CONFIG>/inputs.cmake"
            -DAD_MODE=begin -P "${_script}" VERBATIM)
    foreach(_target IN LISTS PROVENANCE_TARGETS)
        add_dependencies(${_target} engine_sdk_source_snapshot)
    endforeach()
    add_custom_target(engine_sdk_provenance ALL
        COMMAND "${CMAKE_COMMAND}" "-DAD_INPUT_FILE=${_root}/$<CONFIG>/inputs.cmake"
            -DAD_MODE=stamp -P "${_script}"
        DEPENDS ${PROVENANCE_TARGETS} ${PROVENANCE_DATA_TARGETS} VERBATIM)
    file(GENERATE OUTPUT "${_root}/$<CONFIG>/validate.cmake" CONTENT
        "set(AD_INPUT_FILE [==[${_root}/$<CONFIG>/inputs.cmake]==])\nset(AD_MODE validate)\ninclude([==[${_script}]==])\n")
    # Register this before install(TARGETS/FILES): rejection leaves the prefix
    # untouched rather than installing binaries with a stale manifest.
    install(SCRIPT "${_root}/$<CONFIG>/validate.cmake" COMPONENT Runtime)
    set(ALPHADIAMOND_ENGINE_PROVENANCE_ROOT "${_root}" PARENT_SCOPE)
    set(ALPHADIAMOND_ENGINE_PROVENANCE_SCRIPT "${_script}" PARENT_SCOPE)
endfunction()
