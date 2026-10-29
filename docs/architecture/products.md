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
semantics. Python is absent from supported builds, tests and runtime; the two
allowlisted offline analysis summarizers are research tools only.

## Build and validation

CMake owns targets and flags. Make is a resource-aware command facade.

| Command | Coverage | Additional dependency |
|---|---|---|
| `make test-native` | Rules, encoders, search, self-play, replay and policies | Native toolchain |
| `make test-training` | Model, training, checkpoint, arena, rating and CLI contracts | LibTorch |
| `make test-native-asan` | Core contracts under ASan/UBSan | Sanitizer toolchain |

Engine CI has native-core, native-sanitizers, native-training and native-format
lanes, plus independent Runtime SDK and opt-in CUDA workflows. No Qt kit is
needed. Desktop validates its pinned SDK in its own repository.
See [README](../../README.md) and [build cycle](../development/build-cycle.md).

## Frozen contracts

`tests/golden/` and `tests/native/fixtures/` preserve the retired oracle's
answers and their provenance. Do not regenerate them from the implementation
under test. Deliberate rule changes require versioned contracts and independently
justified expectations. Ordered actions, evaluator requests, root statistics,
stochastic distributions and lane scheduling retain their existing gates.

`tests/golden/replay-v1/` is historical; current schema-4/5 replay tests construct
stores natively. See [replay stateless sampling](replay_stateless_sampling.md).
SDKs include only minimal runtime fixtures, not the full training corpus.

[Decisions](decisions.md), [retiring Python](retiring_the_python_engine.md) and
dated reports preserve historical evidence. [Model format v3](model_artifact_v3.md)
and [SDK upgrade policy](repository_split.md) define the current boundaries.
