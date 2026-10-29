# Engine and Desktop ownership

Engine/Training owns the shared C++26 implementation of board geometry, rules,
encoders, two- and three-player MCTS, self-play, inference, rating and model
validation. The separate AlphaDiamondDesktop repository owns Qt/QML rendering,
controller confirmation flow, model browsing, replay UI and packaging.

| Component | Owner and location |
|---|---|
| Core and search | Engine: `native/include/soo/`, `native/src/` |
| Training and pipeline | Engine: `native/include/diamond_training/`, `native/include/diamond_pipeline/` |
| Orchestration | Engine: `native/include/diamond_orchestration/` |
| Model and release | Engine: `native/include/diamond_model/`, `native/include/diamond_release/` |
| Application | AlphaDiamondDesktop: `app/`, consuming an installed Runtime SDK |

Runtime SDK exports core, search, support, artifact, rating and model libraries
without replay/training/orchestration dependencies. Two-player Soo uses scalar
values; three-player Min keeps one value per seat. Shared code preserves these
semantics. Python is absent from supported builds and runtime; offline analysis
summarizers are research tools only.

## Build and validation

CMake owns targets and flags. Make is a resource-aware command facade.

| Command | Product | Additional dependency |
|---|---|---|
| `make build` | Native core/search | Native toolchain |
| `make build PRESET=native-training-linux` | Training and benchmark executables on Linux | LibTorch |
| `make build PRESET=native-asan` | Core compilation with ASan/UBSan | Sanitizer toolchain |

Regression suites and existing GitHub CI workflows have been removed at the
user's request. Future tests and CI are a separate design task. Validate changes
with actual product builds and direct CLI checks; do not describe compilation
alone as runtime sanitizer coverage. No Qt kit is needed for Engine.
See [README](../../README.md) and [build cycle](../development/build-cycle.md).

## Runtime data and historical evidence

The five `data/topology/` tables remain Runtime SDK data embedded by
Desktop. Test-only golden vectors, synthetic SDK model fixtures, and their
generators have been removed. Runtime model/format validation and SDK provenance
remain enforced in product code. Dated documents and recorded experiments retain
the historical evidence for earlier contracts and measurements.

[Decisions](decisions.md), [retiring Python](retiring_the_python_engine.md) and
dated reports preserve historical evidence. [Model format v3](model_artifact_v3.md)
and [SDK upgrade policy](repository_split.md) define the current boundaries.
