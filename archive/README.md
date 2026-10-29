# Historical data archive

These are preserved experiment inputs and outputs. Moving them changes their
repository location while retaining their bytes, including recorded paths,
configuration identities, logs and checkpoints. Historical command transcripts
remain in the documentation archive and are not current launcher instructions.

| Former root location | Current location | Contents |
|---|---|---|
| `az-bench/` | [benchmark-runs/](benchmark-runs/) | Benchmark configs, logs, ledgers, samples and profiles |
| `runtime/` | [python-runtime/](python-runtime/) | Earlier Python runtime configs and the recorded step-80 checkpoint/run |
| `acceptance-baseline/` | [acceptance/](acceptance/) | Recorded self-play acceptance artifacts |

The old runtime folder is historical data, independent of the current C++ Runtime
SDK. Current source is in `src/` and `include/`, builds in `build/`, and local run
outputs in `artifacts/`. There are no compatibility symlinks at the old locations.

Documentation moved separately:

| Former location | Current location |
|---|---|
| `docs/superpowers/{plans,specs,reports,inventories}/` | `docs/archive/{plans,specs,reports,inventories}/` |
| `docs/native-selfplay/` | `docs/archive/selfplay/` |
| `docs/performance-profiling/` | `docs/archive/performance/` |
| Root `2026-09-02-min-a0-transition-roadmap.md` | `docs/archive/reports/2026-09-02-min-a0-transition-roadmap.md` |
| `TrainAlphaDiamond/` | [docs/archive/storage/](../docs/archive/storage/README.md) |

`TrainAlphaDiamond/` contained a README and two historical manifests, with no
actual bucket download content in this checkout. Its existing remote bucket
identity is unchanged. Optional local downloads can go in `artifacts/downloads/`;
this archive is not a new storage system. Use the [current documentation
index](../docs/README.md) for development and training navigation.
