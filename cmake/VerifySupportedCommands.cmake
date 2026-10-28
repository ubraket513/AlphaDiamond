if(NOT DEFINED SOURCE_DIR)
    message(FATAL_ERROR "SOURCE_DIR is required")
endif()

set(pattern "(^|[;&|[:space:]])(python([0-9.]+)?|pip[0-9.]*|pytest|ruff)([[:space:]]|$)|pybind11_add_module|find_package\\([[:space:]]*pybind11")
execute_process(
    COMMAND git -C "${SOURCE_DIR}" grep -n -E "${pattern}" --
        .github/workflows CMakeLists.txt CMakePresets.json Makefile "tools/*.sh"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE matches
    ERROR_VARIABLE error
    OUTPUT_STRIP_TRAILING_WHITESPACE)
if(result EQUAL 0)
    message(FATAL_ERROR "unsupported Python/pybind command remains:\n${matches}")
elseif(NOT result EQUAL 1)
    message(FATAL_ERROR "git grep failed: ${error}")
endif()

# Engine owns runtime/training only; Desktop has an independent source tree.
foreach(path native/qt desktop tools/build_native_qt.sh tools/deploy_native_qt.sh
        tools/preview_native_qt.sh tools/run_native_qt.sh tools/desktop_toolchain.sh
        .github/workflows/desktop.yml)
    if(EXISTS "${SOURCE_DIR}/${path}")
        message(FATAL_ERROR "Desktop-only path remains in Engine: ${path}")
    endif()
endforeach()
file(READ "${SOURCE_DIR}/CMakePresets.json" presets)
if(presets MATCHES "DIAMOND_BUILD_QT|native-qt|native-package")
    message(FATAL_ERROR "Desktop-only preset remains in Engine")
endif()
