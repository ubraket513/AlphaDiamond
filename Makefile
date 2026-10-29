# CMake owns compiler flags and targets. Override worker counts for shared hosts.
.PHONY: help configure build
HOST_OS := $(shell uname -s)
PRESET ?= $(if $(filter Linux,$(HOST_OS)),native-ci-linux,native-ci)
NATIVE_TOOL := tools/native_training.sh
# Affinity/quota/memory-aware worker detection.
BUILD_JOBS ?= $(or $(CMAKE_BUILD_PARALLEL_LEVEL),$(shell tools/build_resources.sh build))
export OMP_NUM_THREADS ?= 1
export MKL_NUM_THREADS ?= 1

help:
	@echo "make build                 Debug native C++ build"
	@echo "make build PRESET=native-training-linux   Linux LibTorch training build"

configure:
	$(NATIVE_TOOL) cmake --preset $(PRESET)

build: configure
	@case "$(BUILD_JOBS)" in ''|*[!0-9]*|0) echo 'BUILD_JOBS must be positive' >&2; exit 2 ;; esac
	$(NATIVE_TOOL) cmake --build --preset $(PRESET) --parallel $(BUILD_JOBS)
