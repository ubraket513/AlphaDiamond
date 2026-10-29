# alphadiamond

A 3-player Diamond (Chinese-checkers-family) engine, a Qt console, and an
AlphaZero training stack. The two-player configuration is called **Soo**; the
three-player one is **Min**.

**This is a C++ project.** There is no Python engine, trainer, bridge, or test
suite. Offline analysis summarizers remain as research tooling. Rules, encoding,
search, self-play, training,
checkpointing, arena and release are all native. Anything you read elsewhere
about a Python oracle, a `_diamond_native` pybind extension, `src/diamond/`, a
`selfplay_backend` setting or a `make test-python` / `make test-parity` /
`pytest` step describes a state this repository has left. See
[docs/architecture/decisions.md](docs/architecture/decisions.md) and
[docs/architecture/retiring_the_python_engine.md](docs/architecture/retiring_the_python_engine.md)
for how and why.

## Rules that are expensive to rediscover

- **Runtime topology remains authoritative SDK data.** The five binary tables in
  `data/topology/` are embedded by Desktop. The regression corpus and
  test suites have been removed; preserve historical experiment evidence.
- **Game geometry has exactly one authority.** Seats come from
  `soo::standard_soo_match()` and `soo::standard_min_match()` in
  `native/src/board.cpp`, never from triples written at a call site. Five
  hand-written copies drifted once: the trainer played a different game from the
  Qt application, and Soo self-play completed 77 % of games instead of 98 % for
  as long as it lasted. The former geometry regression and its findings are recorded in
  [docs/model-training/baseline_768_completion_regression.md](docs/model-training/baseline_768_completion_regression.md).
- **Do not micro-optimise before measuring.** A simple native representation and
  direct numerical/behavioral verification come first.
- **Measure at a scale that can answer the question.** Self-play completion
  varies enough run to run that sixteen games cannot distinguish two
  configurations; 768 games is the harness the findings in `docs/model-training/`
  use, and it costs about three minutes on one RTX 5090.

## Layout

| path | what |
|---|---|
| `native/include/soo/`, `native/src/` | the core: board, rules, encoder, MCTS, self-play, training, checkpoint, arena |
| sibling `AlphaDiamondDesktop/app/` | the Qt application |
| `native/benchmarks/` | benchmarks, kept out of production code |
| `data/topology/` | canonical runtime topology tables |
| `configs/alphazero/` | training configurations |
| `models/` | shipped deployment artifacts (format v3) |
| `docs/` | design records and measured findings |

## Build and direct validation

CMake owns product targets and flags. Regression suites and GitHub CI have been
removed at the user's request; use actual builds and direct CLI/product checks.

```bash
make build
# LibTorch training on Linux; select native-training on other platforms.
make build PRESET=native-training-linux
```

Point CMake at a compatible standalone LibTorch installation when required:

```bash
tools/native_training.sh cmake --preset native-training -G Ninja \
    -DCMAKE_PREFIX_PATH=<libtorch-root>
tools/native_training.sh cmake --build --preset native-training \
    --parallel "$(tools/build_resources.sh build)"
```

`tools/native_training.sh` initializes the Visual Studio environment for CMake
on Windows and passes through on Linux/macOS. Desktop is built separately.

## Commands

```bash
alphadiamond-train train  --run-dir <dir> --config <config.json> \
                          (--scratch | --checkpoint <dir> | --warm-start <artifact>)
alphadiamond-train resume --run-dir <dir>
alphadiamond-train report --run-dir <dir>
alphadiamond-checkpoint   inspect|validate|migrate <checkpoint-root>
alphadiamond-release      ...
```

A run directory is durable and idempotent: every stage is ledgered by operation
id, `resume` replays exactly the work that was not completed, and re-running a
finished stage is a no-op. `--run-dir` must sit under a `soo/` or `min/` parent.

Each iteration also writes two diagnostic sidecars that nothing reads back and
no gate depends on: `selfplay.metrics.json` (search budget actually in force,
boosted-move count, throughput, batch distribution, abort reasons,
completed-game move percentiles) and `aborted-games.json` (the final position of
every aborted game, with target-camp occupancy and blocker mobility).

## Shipping a model

```bash
cmake --build --preset native-release --target package --parallel 8
```

Artifact format v3 declares the model family and architecture and is validated
against the weights on both sides; see
[docs/architecture/model_artifact_v3.md](docs/architecture/model_artifact_v3.md).
A training checkpoint carries optimizer, scheduler and RNG state, is not a
deployment artifact, and is never bundled.

Native transactional v3 checkpoints can be exported with:

```bash
alphadiamond-checkpoint export <checkpoint-v3-root> --out <new-artifact> --version <test-version>
```

The exporter preserves the checkpoint, rejects noncanonical topology, and verifies
exact CPU inference parity before succeeding. Raw archival `.pt` checkpoints and
v2 checkpoints without architecture provenance are not supported by this exporter.

## Training data

Checkpoints, replay and logs live in the Hugging Face bucket, synchronised
through the ignored `TrainAlphaDiamond/` root; only its README and manifests are
tracked. See [TrainAlphaDiamond/README.md](TrainAlphaDiamond/README.md). Nothing
generated — build directories, checkpoints, run output — belongs in Git.
