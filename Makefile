# CMake owns compiler flags and targets. Override worker counts for shared hosts.
.PHONY: help configure build test test-native test-training test-native-asan test-qt package
PRESET ?= native-ci
NATIVE_TOOL := tools/native_training.sh
DESKTOP_TOOL := tools/desktop_toolchain.sh
# Engine targets must remain usable after Desktop tools are extracted.
BUILD_JOBS ?= $(or $(CMAKE_BUILD_PARALLEL_LEVEL),$(shell cpus=$$(getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo $${NUMBER_OF_PROCESSORS:-1}); jobs=$$cpus; if test -r /proc/meminfo; then mem=$$(awk '/MemAvailable:/ {print int(($$2/1024-1024)/2048)}' /proc/meminfo); test $$mem -gt 0 || mem=1; test $$jobs -le $$mem || jobs=$$mem; fi; echo $$jobs))
TEST_JOBS ?= $(or $(CTEST_PARALLEL_LEVEL),$(shell getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo $${NUMBER_OF_PROCESSORS:-1}))
SDK_PREFIX ?= $(DIAMOND_ENGINE_SDK_PREFIX)
MODELS_DIR ?=
export OMP_NUM_THREADS ?= 1
export MKL_NUM_THREADS ?= 1

help:
	@echo "make test-native       Debug native C++ contract tests"
	@echo "make test-training     Debug native LibTorch training tests"
	@echo "make test-qt SDK_PREFIX=/sdk    standalone Debug Desktop tests"
	@echo "make package SDK_PREFIX=/sdk [MODELS_DIR=/models]    explicit Release Windows package"

configure:
	$(NATIVE_TOOL) cmake --preset $(PRESET)

build: configure
	$(NATIVE_TOOL) cmake --build --preset $(PRESET) --parallel $(BUILD_JOBS)

test: test-native

test-native: build
	$(NATIVE_TOOL) ctest --preset $(PRESET) --parallel $(TEST_JOBS) --output-on-failure --no-tests=error

test-training:
	$(MAKE) test-native PRESET=native-training

test-native-asan:
	$(MAKE) test-native PRESET=native-asan

test-qt:
	tools/build_native_qt.sh --sdk-prefix "$(SDK_PREFIX)" --no-deploy
	$(DESKTOP_TOOL) ctest --test-dir build/desktop-dev --parallel $(TEST_JOBS) --output-on-failure --no-tests=error

package:
	tools/build_native_qt.sh --preset desktop-release --build-dir build/desktop-release --sdk-prefix "$(SDK_PREFIX)" --deploy $(if $(MODELS_DIR),--models-dir "$(MODELS_DIR)",)
