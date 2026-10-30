# Focused weakness remediation

This workflow runs finite experiments without resuming the ordinary learners or the paused heartbeat. It reuses the native reanalysis executable, ReplayStore, RecentReplay and the learning diagnostic. Original runs, checkpoints and trajectories stay immutable.

## Collect and resume

Build `min_reanalysis`, `weakness_replay_probe` and `min_learning_diagnostic` with the configured parallel build.

```sh
cmake --build build/replay-experiment --target min_reanalysis weakness_replay_probe min_learning_diagnostic --parallel 12
build/replay-experiment/bin/min_reanalysis --collect SOURCE_RUN NEW_ARCHIVE
build/replay-experiment/bin/min_reanalysis --validate NEW_ARCHIVE/train-endpoints.json cyclic
build/replay-experiment/bin/min_reanalysis NEW_ARCHIVE/validation-endpoints.json CHECKPOINT cyclic reanalyse NEW_VALIDATION_RUN
build/replay-experiment/bin/min_reanalysis NEW_ARCHIVE/train-endpoints.json CHECKPOINT cyclic reanalyse NEW_TRAIN_RUN
build/replay-experiment/bin/min_reanalysis --prepare NEW_ARCHIVE NEW_TRAIN_RUN NEW_VALIDATION_RUN NEW_DATASET
python3 benchmarks/verify_weakness_reanalysis.py build/replay-experiment/bin/min_reanalysis NEW_ARCHIVE NEW_TRAIN_RUN NEW_VALIDATION_RUN
```

Use `reverse` for a Reverse archive. New output paths must not exist. The source run must contain balanced native Min games with the goals-v2 encoder and an unchanged resolved configuration. All six turn orders are supported.

Collection reconstructs saved physical states and verifies them by re-encoding every feature. Repetitions after move 96 or a delayed first finish select an anchor 16 moves before the problem; delayed first finish uses move 128 as the marker. At most one pre-first anchor and one separately classified completion anchor are retained per original game. This is a heuristic for a pilot, not a learned weakness detector.

The split hashes the original game ID; every branch of that game stays in one split. Native board/actor/order duplicates are excluded across splits. Source hashes, teacher digests and native input fingerprints are recorded. Preparation checks that each continuation actually starts at its declared root. Completed native outcomes alone provide rank/value targets. Incomplete games remain unresolved; the CLI can resume independently audited cutoff states, but collection does not invent missing earlier trajectories for old aborted games.

Preparation keeps unique pre-first targets only, at most 16 rows per branch and 64 per original training family. If any training branch touches a held-out state, all branches of that source family are removed. Normal retention replay is rebuilt from other source games and excludes held-out continuation states. Post-first completion anchors remain a separate diagnostic dataset.

## Bounded learning

For each experimental arm, copy `NEW_DATASET/regular-replay` into a new arm directory. Set `training.recent_sample_fraction` to 0, 0.05 or 0.10 in a copied source config. The zero arm uses only regular replay. Other arms pass `NEW_DATASET/training.episodes` and the dataset manifest operation ID to `min_learning_diagnostic --episode-artifact ... --episode-operation-id ...`. Use the same source checkpoint, optimizer, seed, batch 512, 512 updates and learning rate 2e-5 for every arm. Point `--evaluation-replay` to the held-out replay and save each checkpoint to a new path. Do not ingest into the original production replay.

Held-out loss and KL are screening signals. Forks from one root are correlated; the first pilot has only four Cyclic and two Reverse validation families. Strength claims require unused normal-start games with balanced seats/order and uncertainty bounds. No production expert is replaced from training loss alone. Final Min still routes separate Cyclic and Reverse experts.

The finite resumed-game generator still has a batch tail barrier. Continuous ordinary self-play/learner decoupling is deferred while ordinary training is stopped.

## First pilot outcome

The six 512-update arms and a 1,728-game paired normal-start arena completed. Hard replay at 10% reduced held-out value MSE, but neither expert showed a verified first-place improvement against its equal-update control. The 95% whole-opening intervals crossed zero. All 48 held-out recovery continuations completed for both control and hard replay, so this small validation pool did not establish a baseline failure to repair. No production checkpoint was promoted. Results and exact commands are preserved under `artifacts/weakness-20261030/`.

The next dataset should first demonstrate actual failures or sustained pre-first stalls under the current selected experts and fixed search budget. Source-game length alone is insufficient evidence of a current weakness. More updates are not justified by this pilot.
