# Min 512 by 12 local training

The current large Min is one shared dense network: width 512, 12 residual blocks,
22,834,179 parameters, nine goal-aware input features and three placement values.
Different game orders use the same parameters. Neural inference and learning run
on CUDA; game rules, search trees, episode scheduling and replay handling use CPU.

## Heuristic removal readiness on October 30 2026

Two consecutive saved actors were fixed before evaluation. Each played 384 games
without the target-vacancy-distance prior, covering all six orders equally.
Search used 128 simulations, repetition window eight and repetition temperature
one. Both had to complete at least 373 games and at least 61 of each order's 64
games. Mean revisit and repeat-within-eight fractions had to be at most 0.1,
mean maximum revisits at most five, median length at most 120 moves and the 90th
percentile at most 200 moves.
These criteria were unchanged from the preceding unsuccessful readiness check.

| Actor iteration | Saved updates | Completed | Median moves | 90th percentile moves |
|---|---:|---:|---:|---:|
| 78 | 20,224 | 382/384 | 91 | 108 |
| 77 | 19,968 | 382/384 | 93 | 145 |

Both actors passed every criterion. Combined order counts were 128, 127, 127,
127, 127 and 128 completed out of 128, in lexicographic order from `1-2-3` through
`3-2-1`. The combined result was 764/768, or 99.48%.

This establishes the declared unassisted completion readiness, rather than a
playing-strength improvement. The checkpoints share a training lineage, and the
before/after comparison also includes additional training and different seeds;
it does not isolate the effect of order weighting. Raw results and frozen inputs
are retained locally in
`artifacts/min-512x12-3h-20261029/heuristic-removal-readiness-v3/`.

## CPU inference measurements on October 30 2026

Hardware was a Ryzen 9 9950X3D with 16 physical cores and 32 logical CPUs, and an
RTX 5080. All measurements used actor 78 and the same frozen native benchmark.
The workload requested 96 games, 64 lanes, 24 search workers, eight simulations
and a one-move cap, with no heuristic prior. These short probes measure inference
capacity through the existing self-play path. Their move-limit aborts are
intentional; they provide no completed-game throughput or strength result.

The scan covered batch caps 8, 32 and 64 with 1, 4, 8, 16 and 32 Torch threads,
then batch caps 1, 2 and 4 with 8 and 16 threads. A warmup preceded two timed
passes per setting. The selected single-worker setting was repeated twice.

| Warm inference measurement | State evaluations per second |
|---|---:|
| GPU controls pooled, batch cap 64 and Torch threads 2 | 8,094.57 |
| Selected CPU repeats pooled, batch cap 8 and Torch threads 16 | 229.10 |

The CPU rate was 2.83% of the GPU rate on this proxy. GPU controls individually
measured 8,151.98 and 8,037.96 evaluations/s. The two selected CPU repeats measured
223.72 and 234.75 evaluations/s. Smaller batches and 32 threads did not improve
the selected single-worker result. Both devices used FP32 tensors; the GPU had
the TF32 cuBLAS permission request enabled. This does not identify individual
kernels or establish CPU/GPU numerical identity.

Independent CPU workers were also pinned across both chiplets. Each worker ran
one warmup and four timed passes. The following aggregate metric counts all
known evaluations, including warmup, over first-launch-to-last-exit wall time.
It includes model loading and unequal worker finish times, so it must not be
directly compared with the warmed GPU rate above.

| CPU workers by Torch threads each | Aggregate evaluations/s including load and warmup | Mean busy logical CPUs |
|---|---:|---:|
| 1 by 16 | 213.73 | 14.76 |
| 1 by 32 | 184.39 | 15.33 |
| 2 by 8 | 368.99 | 12.26 |
| 2 by 16 | 345.35 | 18.45 |
| 4 by 8 | 359.49 | 22.10 |

Two eight-thread workers improved this matched CPU metric by 72.64% over one
16-thread worker. Four eight-thread workers covered all 32 logical CPUs but used
more CPU time for slightly less throughput. Standalone rates do not establish
a simultaneous CPU+GPU speedup. CPU routing is therefore not enabled.

Protocols, execution records, all 21 batch/thread points, repeats and worker
measurements are retained locally under
`artifacts/min-512x12-3h-20261029/cpu-inference-capability-20261030/`.
`analysis.json` records the metric definitions; `cpu-grid.csv` contains the scan.

## Heuristic free continuation

Resume the existing run with `self_play.bootstrap_prior: "none"` and
`self_play.bootstrap_prior_weight: 0.0`. The existing native transition records
these two changes at a clean self-play boundary. Preserve the optimizer and
replay; older assisted targets remain until normal replay replacement removes
them. New self-play uses only the learned policy/value and retained repetition
sampling, without the distance prior.

Retain `reverse` order weights `[1,2,1]`: 24 games of `1-3-2`, 48 of `2-1-3`
and 24 of `3-2-1` per 96-game batch. Keep 128 simulations, 64 lanes, 24 search
threads, inference batch cap 64, 100 microseconds batch wait, learner batch 64,
256 updates per iteration and learning rate 0.0003. Inference remains GPU only.
An external process limit enforces the requested three-hour wall-clock budget;
completed checkpoints remain usable if the final stage is interrupted.
