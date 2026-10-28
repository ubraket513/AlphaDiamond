# Native products, one core

The application, trainer, checkpoint tools, arena, and release tools are native
C++. They share one implementation of board geometry, rules, encoding, search,
and self-play. The Python trainer and pybind bridge were retired; the completed
migration brief and intermediate progress log are available in Git history.

| Component | Responsibility | Location |
|---|---|---|
| Core and search | Board, rules, encoders, two- and three-player MCTS, self-play | `native/include/soo/`, `native/src/` |
| Qt application | Controller confirmation flow, rendering, model inference | `native/qt/` |
| Training and pipeline | LibTorch training, replay, checkpoints, diagnostics | `native/include/diamond_training/`, `native/include/diamond_pipeline/` |
| Orchestration | Run stages, schedules, arena, ratings, reports | `native/include/diamond_orchestration/` |
| Model and release | Deployment validation, model loading, promotion | `native/include/diamond_model/`, `native/include/diamond_release/` |

QML renders state and forwards input. Native rules own legality; the controller
keeps proposals separate from confirmed commits. Two-player Soo uses scalar
values; three-player Min keeps a value for each seat. Shared search mechanics
must preserve those distinct value semantics.

Python is not part of the supported runtime, build, or test path. The two
offline analysis summarizers allowlisted by `cmake/VerifyPythonZero.cmake` are
research tooling only.

## Build and validation

CMake owns targets and dependencies. `CMakePresets.json` defines configurations;
the Makefile is a command facade. Use a compiler supporting the required C++26
dialect. See the [README](../../README.md) for setup and supported commands.

| Command | Coverage | Additional dependency |
|---|---|---|
| `make test-native` | Rules, encoders, search, self-play, replay, repository policies | None beyond the native toolchain |
| `make test-training` | Model, training, checkpoint, arena, rating, CLI contracts | LibTorch |
| `make test-qt` | Controller, analysis, rendering, model integration | Qt and LibTorch |
| `make test-native-asan` | Core contracts under ASan/UBSan | Supported sanitizer toolchain |

For explicit concurrency, use `cmake --build --preset <preset> --parallel <jobs>`
and `ctest --preset <preset> --parallel <workers> --output-on-failure`.
Choose counts for the host's available CPU and memory, including threads
started by the tests themselves.

CI defines `native-core`, `native-sanitizers`, `native-training`, `native-qt`,
and `native-format` in [ci.yml](../../.github/workflows/ci.yml). There are no
Python trainer, bridge, or lint lanes.

## Frozen contracts

`tests/golden/` and `tests/native/fixtures/` preserve the retired oracle's
answers. Do not regenerate them from the implementation under test. A deliberate
rules change requires a versioned contract change and an independently justified
corpus. `golden_manifest_test` checks the committed fixture provenance.

The golden search tests pin ordered actions, evaluator requests, root statistics,
and floating-point results. Stochastic and self-play tests cover distributions,
seed determinism, and lane scheduling. These independent expectations remain
valid when production code shares helpers.

Replay is the exception: `tests/golden/replay-v1/` is a historical record,
unreadable by the current schema-4/5 store. Replay tests construct their stores
natively. See [replay stateless sampling](replay_stateless_sampling.md).

## Historical evidence

- [Decisions](decisions.md) records the original migration decisions and the
  frozen-corpus contract; dated Python control-plane descriptions are historical.
- [Retiring the Python engine](retiring_the_python_engine.md) preserves deletion
  order, parity evidence, and why the oracle cannot be regenerated.
- [Python-zero final acceptance](../superpowers/reports/2026-08-25-python-zero-final-acceptance.md)
  records native training and Windows package validation.
- [Model artifact v3](model_artifact_v3.md) defines the deployment boundary.

Measured performance reports and experiment handoffs stay with their evidence.
Completed plans describe their original implementation, not current commands or
a new work queue. Use this page and the README for the current product layout.
