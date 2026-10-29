cmake_minimum_required(VERSION 3.21)

project(alphadiamond LANGUAGES CXX)

include(cmake/DevelopmentToolchain.cmake)

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
find_package(Threads REQUIRED)

add_subdirectory(native)

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
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Diamond engine, Runtime SDK and native training")
set(CPACK_PACKAGE_VERSION "0.1.0")
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_CHECKSUM SHA256)
set(CPACK_VERBATIM_VARIABLES ON)
include(CPack)
