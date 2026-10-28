# CMake owns compiler flags and targets. Override worker counts for shared hosts.
.PHONY: help configure build test test-native test-training test-native-asan
HOST_OS := $(shell uname -s)
PRESET ?= $(if $(filter Linux,$(HOST_OS)),native-ci-linux,native-ci)
TRAINING_PRESET ?= $(if $(filter Linux,$(HOST_OS)),native-training-linux,native-training)
NATIVE_TOOL := tools/native_training.sh
# Affinity/quota/memory-aware worker detection.
BUILD_JOBS ?= $(or $(CMAKE_BUILD_PARALLEL_LEVEL),$(shell tools/build_resources.sh build))
TEST_JOBS ?= $(or $(CTEST_PARALLEL_LEVEL),$(shell tools/build_resources.sh test))
export OMP_NUM_THREADS ?= 1
export MKL_NUM_THREADS ?= 1

help:
	@echo "make test-native       Debug native C++ contract tests"
	@echo "make test-training     Debug native LibTorch training tests"

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
