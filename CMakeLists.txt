cmake_minimum_required(VERSION 3.21)

project(alphadiamond LANGUAGES CXX)

option(DIAMOND_BUILD_NATIVE_TESTS "Build portable native smoke tests" ON)
option(DIAMOND_BUILD_LIBTORCH "Build native LibTorch model and training support" OFF)
option(DIAMOND_BUILD_LIBTORCH_PROBE "Deprecated alias for DIAMOND_BUILD_LIBTORCH" OFF)
if(DIAMOND_BUILD_LIBTORCH_PROBE)
    set(DIAMOND_BUILD_LIBTORCH ON CACHE BOOL "Build native LibTorch model and training support" FORCE)
endif()
# The legacy umbrella still enables both components. New presets select them
# explicitly so inference never implicitly enables training.
option(DIAMOND_BUILD_MODEL "Build native LibTorch inference models" OFF)
option(DIAMOND_BUILD_TRAINING "Build native training, replay orchestration and release tools" OFF)
if(DIAMOND_BUILD_LIBTORCH)
    set(DIAMOND_BUILD_MODEL ON)
    set(DIAMOND_BUILD_TRAINING ON)
endif()
if(DIAMOND_BUILD_TRAINING AND NOT DIAMOND_BUILD_MODEL)
    message(FATAL_ERROR "DIAMOND_BUILD_TRAINING requires DIAMOND_BUILD_MODEL=ON")
endif()
option(DIAMOND_DESKTOP_ENABLE_RATINGS "Enable Desktop local ratings and optional sync" ON)
option(DIAMOND_BUILD_QT "Build the native Qt/QML shell" OFF)
option(DIAMOND_BUILD_QT_SOO "Build Qt shell with native LibTorch Soo and Min search" OFF)

# CMake cannot yet model CXX26 for the installed MSVC toolset.  Keep its
# known-feature baseline at C++23, then require each compiler's next-standard
# dialect explicitly below.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/std:c++latest)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-std=c++2c)
else()
    message(FATAL_ERROR "AlphaDiamond requires a compiler with a C++26 dialect mode")
endif()

include(GNUInstallDirs)
include(CTest)
find_package(Threads REQUIRED)

add_subdirectory(native)

if(BUILD_TESTING)
    add_test(NAME python_zero_policy
        COMMAND ${CMAKE_COMMAND}
            -DSOURCE_DIR:PATH=${PROJECT_SOURCE_DIR}
            -P ${PROJECT_SOURCE_DIR}/cmake/VerifyPythonZero.cmake)
    add_test(NAME repository_hygiene
        COMMAND ${CMAKE_COMMAND}
            -DSOURCE_DIR:PATH=${PROJECT_SOURCE_DIR}
            -P ${PROJECT_SOURCE_DIR}/cmake/VerifyRepositoryHygiene.cmake)
    add_test(NAME supported_commands_policy
        COMMAND ${CMAKE_COMMAND}
            -DSOURCE_DIR:PATH=${PROJECT_SOURCE_DIR}
            -P ${PROJECT_SOURCE_DIR}/cmake/VerifySupportedCommands.cmake)
    set_tests_properties(
        python_zero_policy repository_hygiene supported_commands_policy
        PROPERTIES LABELS policy)
endif()

if(DIAMOND_BUILD_QT OR DIAMOND_BUILD_QT_SOO)
    add_subdirectory(native/qt)
endif()

if(BUILD_TESTING AND DIAMOND_BUILD_NATIVE_TESTS AND DIAMOND_BUILD_MODEL)
        add_test(NAME runtime_boundary_test
            COMMAND ${CMAKE_COMMAND}
                -DSOURCE_DIR:PATH=${PROJECT_SOURCE_DIR}
                -DBINARY_DIR:PATH=${CMAKE_BINARY_DIR}/native/runtime-boundary
                -DCXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
                -DGENERATOR:STRING=${CMAKE_GENERATOR}
                -DCXX_LAUNCHER:FILEPATH=${CMAKE_CXX_COMPILER_LAUNCHER}
                -DEXE_LINKER_FLAGS:STRING=${CMAKE_EXE_LINKER_FLAGS}
                -DTORCH_CUDA_ARCH_LIST:STRING=${TORCH_CUDA_ARCH_LIST}
                -DTorch_DIR:PATH=${Torch_DIR}
                -DQt6_DIR:PATH=${Qt6_DIR}
                -DENABLE_QT:BOOL=${DIAMOND_BUILD_QT_SOO}
                -P ${PROJECT_SOURCE_DIR}/native/tests/runtime_boundary_test.cmake)
        set_tests_properties(runtime_boundary_test PROPERTIES LABELS "runtime;contract")
endif()

# Packaging. Models ship *beside* the executable rather than compiled into it,
# so a release can carry a different default model, and a model can be replaced,
# without relinking. Build a validated runtime-only directory first:
#
#   alphadiamond-release stage dist/models \
#       --artifact artifacts/soo-spike --default soo
#
# then `cmake --build --preset native-release --target package`.
set(DIAMOND_MODELS_DIR "${CMAKE_SOURCE_DIR}/dist/models"
    CACHE PATH "models/ directory to bundle into the package")
if(EXISTS "${DIAMOND_MODELS_DIR}/index.json")
    install(DIRECTORY "${DIAMOND_MODELS_DIR}/" DESTINATION models)
endif()
install(FILES "${CMAKE_SOURCE_DIR}/README.md" DESTINATION .)

set(CPACK_PACKAGE_NAME "AlphaDiamond")
set(CPACK_PACKAGE_VENDOR "alphadiamond")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Diamond engine, Qt console and AlphaZero stack")
set(CPACK_PACKAGE_VERSION "0.1.0")
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_CHECKSUM SHA256)
set(CPACK_VERBATIM_VARIABLES ON)
include(CPack)
