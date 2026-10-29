cmake_minimum_required(VERSION 3.30)
project(AlphaDiamondSdkConsumer LANGUAGES CXX)

set(SDK_TEST_VERSION "0.1.0" CACHE STRING "Exact SDK version under test")
find_package(AlphaDiamondEngine ${SDK_TEST_VERSION} EXACT CONFIG REQUIRED COMPONENTS Runtime)
foreach(component soo_core soo_search diamond_support diamond_artifact diamond_model diamond_rating)
    if(NOT TARGET alphadiamond::${component})
        message(FATAL_ERROR "SDK is missing alphadiamond::${component}")
    endif()
endforeach()

add_executable(sdk_consumer main.cpp)
target_link_libraries(sdk_consumer PRIVATE
    alphadiamond::soo_core alphadiamond::soo_search
    alphadiamond::diamond_support alphadiamond::diamond_artifact
    alphadiamond::diamond_model alphadiamond::diamond_rating)
enable_testing()
add_test(NAME sdk_runtime COMMAND sdk_consumer "${AlphaDiamondEngine_DATA_DIR}")
set_tests_properties(sdk_runtime PROPERTIES ENVIRONMENT "OMP_NUM_THREADS=2;MKL_NUM_THREADS=2")
