# CMake owns compiler flags and targets. Override worker counts for shared hosts.
.PHONY: help configure build test test-native test-training test-native-asan test-qt package
HOST_OS := $(shell uname -s)
PRESET ?= $(if $(filter Linux,$(HOST_OS)),native-ci-linux,native-ci)
TRAINING_PRESET ?= $(if $(filter Linux,$(HOST_OS)),native-training-linux,native-training)
NATIVE_TOOL := tools/native_training.sh
DESKTOP_TOOL := tools/desktop_toolchain.sh
# Engine and Desktop share affinity/quota/memory-aware detection.
BUILD_JOBS ?= $(or $(CMAKE_BUILD_PARALLEL_LEVEL),$(shell tools/build_resources.sh build))
TEST_JOBS ?= $(or $(CTEST_PARALLEL_LEVEL),$(shell tools/build_resources.sh test))
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
	@case "$(BUILD_JOBS)" in ''|*[!0-9]*|0) echo 'BUILD_JOBS must be positive' >&2; exit 2 ;; esac
	$(NATIVE_TOOL) cmake --build --preset $(PRESET) --parallel $(BUILD_JOBS)

test: test-native

test-native: build
	@case "$(TEST_JOBS)" in ''|*[!0-9]*|0) echo 'TEST_JOBS must be positive' >&2; exit 2 ;; esac
	$(NATIVE_TOOL) ctest --preset $(PRESET) --parallel $(TEST_JOBS) --output-on-failure --no-tests=error

test-training:
	$(MAKE) test-native PRESET=$(TRAINING_PRESET)

test-native-asan:
	$(MAKE) test-native PRESET=native-asan

test-qt:
	tools/build_native_qt.sh --sdk-prefix "$(SDK_PREFIX)" --no-deploy
	$(DESKTOP_TOOL) ctest --test-dir build/desktop-dev --parallel $(TEST_JOBS) --output-on-failure --no-tests=error

package:
	tools/build_native_qt.sh --preset desktop-release --build-dir build/desktop-release --sdk-prefix "$(SDK_PREFIX)" --deploy $(if $(MODELS_DIR),--models-dir "$(MODELS_DIR)",)
