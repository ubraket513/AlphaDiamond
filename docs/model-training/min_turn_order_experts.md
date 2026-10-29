# Min turn-order specialists

Min can train separate networks for the two geometric turn-order groups. In the
standard board's physical player IDs, `cyclic` contains 123/231/312 and `reverse`
contains 132/213/321. The classifier uses home/goal geometry and the full Match
order, so rotating the board, relabeling players, changing the current actor, or
finishing a player does not change group membership.

Set `self_play.turn_order_group` to `cyclic` or `reverse` with
`balance_turn_orders: true` in a Min configuration. Games per iteration must be
divisible by three. The default `all` retains six-order training and historical
configuration serialization. Group selection is immutable on resume. Use a new
run directory for each expert; manually moving replay between runs is unsupported.

Each expert owns all weights, optimizer state, and replay. Start both with
`--warm-start` from the same deployment artifact to copy weights while resetting
AdamW and replay. Exact resume uses the expert's own run directory.

```sh
OMP_NUM_THREADS=2 MKL_NUM_THREADS=2 build/training/bin/alphadiamond-train train \
  --run-dir artifacts/min-experts-20261014/runs/min/cyclic \
  --config configs/current/min-expert-cyclic-local-pilot.json \
  --warm-start artifacts/min-experts-20261014/baseline/shared-step128
```

The reverse and shared-control pilot configurations use the same initialization,
learning rate, batch size and search budget. Each expert receives 768 requested
games and 256 updates; the shared control receives 1,536 games and 512 updates,
matching the experts' combined game and optimizer-example budgets. This does not
promise identical FLOPs: game lengths and search work can differ. Check actual
completions, aborted games and retained replay rows by order before interpreting
the results.

## Frozen-reference arena

`min_expert_arena` accepts goals-v2 checkpoint-v3 roots and validates their Min
architecture, compatibility and training-group provenance. It routes the
candidate's search through the appropriate expert. Both opposing players use
either one frozen shared reference or a frozen cyclic/reverse reference pair.
The outer arena router selects the network from
the actual root player; simulated leaf actors only determine feature/value
perspective.

```sh
OMP_NUM_THREADS=2 MKL_NUM_THREADS=2 build/training/bin/min_expert_arena \
  --cyclic-checkpoint artifacts/min-experts-20261014/runs/min/cyclic/iterations/1/candidate-checkpoint \
  --reverse-checkpoint artifacts/min-experts-20261014/runs/min/reverse/iterations/1/candidate-checkpoint \
  --shared-checkpoint artifacts/goal-aware-20261013/runs/min/pilot/iterations/0/candidate-checkpoint \
  --shared-control-checkpoint artifacts/min-experts-20261014/runs/min/shared-control/iterations/3/candidate-checkpoint \
  --config configs/experiments/min-expert-arena-local.json \
  --out artifacts/min-experts-20261014/comparison
```

To compare against specialists, replace `--shared-checkpoint` with both
`--reference-cyclic-checkpoint DIR` and `--reference-reverse-checkpoint DIR`.
The reference pair routes each opposing player's search by the same geometric
turn-order group as the candidate pair. Choose exactly one reference mode;
missing pair members, mixed modes, duplicate flags, incompatible architectures
and incorrect checkpoint groups fail before creating the output directory.
The optional shared-control arm works against either reference mode.

`invocation.json` records `reference_mode` as `shared` or `expert-pair`.
Its `checkpoints` object always contains `cyclic` and `reverse`; shared mode
also contains `shared`, while expert-pair mode contains `reference-cyclic` and
`reference-reverse`. An optional control adds `shared-control`. Each identity
retains the checkpoint and generation paths, model/state/optimizer/manifest and
resolved-configuration hashes, training step, source commit and exact group.

The output directory must be new. Raw outcomes and checkpoint/config identities
are preserved separately for each arm. Both arms use identical opening, seat,
turn-order and seed schedules. The pilot has 12 held-out opening blocks and 432
games per arm: 72 games per order, 216 per expert domain.

Report first/second/third placements and mean utility (+1/0/−1) for both groups
and all six orders. Bootstrap entire opening blocks, and compare experts to the
continued shared control using paired blocks. Report pending/aborted games and
exclude incomplete blocks from inferential estimates. An aggregate improvement
does not establish improvement in every order, and configuration coverage tests
do not enumerate every reachable board position. These experiments do not change
the shipped models or GUI model selection.

## Implementation verification

The native training build passed all 56 CTest tests, including four CUDA tests.
New tests exercise geometric classification, mixed-batch and root-owner routing,
isolated warm starts/replay/optimizers, immutable resume selection, and an actual
CPU arena with completed and aborted games. An independent review found no
correctness blocker. Arena output is saved after each order batch; if interrupted,
games from the active batch may still appear pending. Preserve the executable
hash and source commit alongside each experimental invocation.

## Pilot execution

The experiment started October 14, 2026 at 01:06 KST from implementation commit
`6a960ba850ec01e8e0c7afa9c6561945fcea6749`. Saved binaries, configs, launch hashes,
logs and results live under `artifacts/min-experts-20261014/`. Training and both
arena arms finished successfully at 02:27 KST. All models started with identical
weights and fresh optimizers. JAX verified finite losses and final weights,
changes in all three goal columns, and exact training-group isolation. Arena
runtime model digests match the exported final checkpoints and frozen baseline.

| Training arm | Updates | Completed / requested games | Aborted | Retained rows |
| --- | ---: | ---: | ---: | ---: |
| Cyclic | 256 | 767 / 768 | 1 | 83,562 |
| Reverse | 256 | 728 / 768 | 40 | 116,908 |
| Shared control | 512 | 1,495 / 1,536 | 41 | 188,132 |

## Pilot results

The specialists improved the aggregate result against the frozen reference and
outperformed the matched shared continuation. The clearest benefit is the cyclic
group: shared continuation regressed there. The reverse group's additional gain
is less certain when aborted games and all opening blocks are considered.
This pilot does not establish improvement in every individual turn order.

The arena recorded all 432 scheduled results per arm. Specialists completed 428
games (216 first, 164 second, 48 third), with four move-limit aborts. The shared
control completed 425 (152 first, 166 second, 107 third), with seven move-limit
aborts. No games remain pending. Completed-game descriptive utilities are +0.393
and +0.106 respectively; these are not paired inferential estimates.

The predeclared inference method excludes an entire opening block if any of its
36 games is incomplete. Nine blocks remain for experts, eight for the shared
control, and six are complete in both arms. The table below uses exactly those
same six paired blocks for both means and their difference. Utility is +1/0/−1
for first/second/third place against two copies of the frozen shared reference.
Intervals are pointwise 95% percentile intervals from 100,000 JAX opening-block
bootstrap replicates, seed 20261014.

| Group or order | Expert utility | Shared-control utility | Paired expert gain [95% CI] |
| --- | ---: | ---: | --- |
| All | +0.398 | +0.097 | +0.301 [+0.204, +0.431] |
| Cyclic | +0.130 | −0.241 | +0.370 [+0.167, +0.574] |
| Reverse | +0.667 | +0.435 | +0.231 [+0.056, +0.370] |
| 123 | −0.056 | −0.222 | +0.167 [−0.167, +0.556] |
| 132 | +0.500 | +0.361 | +0.139 [−0.111, +0.389] |
| 213 | +0.722 | +0.528 | +0.194 [−0.056, +0.444] |
| 231 | +0.333 | −0.222 | +0.556 [+0.333, +0.778] |
| 312 | +0.111 | −0.278 | +0.389 [+0.000, +0.778] |
| 321 | +0.778 | +0.417 | +0.361 [+0.028, +0.667] |

After correction across six order comparisons, only order 231 retains an interval
strictly above zero. Both group intervals remain positive after a separate
two-group correction, conditional on the retained six blocks. Using each arm's
own complete blocks, expert cyclic utility is +0.210 [+0.062, +0.358] and reverse
is +0.586 [+0.475, +0.698]; shared cyclic is −0.215 [−0.319, −0.125] and reverse
is +0.424 [+0.375, +0.486]. Different retained block sets explain why these means
differ from the paired table.

### Abort sensitivity and interpretation

Complete-block selection discards half the planned paired openings. As a
secondary sensitivity check, assign every aborted game any hypothetical utility
between −1 and +1 and retain all twelve openings. The overall expert gain is
bounded between +0.259 and +0.310, with a bootstrap sensitivity envelope of
[+0.185, +0.396]. Cyclic has no arena aborts: its all-opening gain is +0.486,
with interval [+0.324, +0.644]. Reverse's gain is bounded between +0.032 and
+0.134, but its sensitivity envelope spans [−0.134, +0.259]. Thus the overall and
cyclic benefit is more robust than a claim of additional reverse-group benefit.
The sensitivity envelope supplements, rather than replaces, the predeclared
complete-block analysis.

This is one training seed and twelve held-out opening clusters, not exhaustive
board coverage. Games and optimizer examples were matched in aggregate, but
retained data volume and per-group replay exposure differ: the specialist pair
retained 200,470 rows versus 188,132 for the control. Shared replay contains
85,138 cyclic rows (45.3%); specialists allocate half the combined optimizer
updates to cyclic training. The experiment supports specialization as a practical
improvement here, but does not isolate gradient interference as the sole cause.

Evidence is preserved locally in `artifacts/min-experts-20261014/`:

- `launch.json`: clean source revision, saved executable/config hashes and budgets.
- `verification/training-audit-jax.json`: initialization, updates and group audit.
- `comparison/{experts,shared-control}/matches.json`: every completed/aborted outcome.
- `comparison/paired-analysis-jax.json`: paired means, confidence intervals and source hashes.
- `comparison/abort-sensitivity-jax.json`: all-opening bounded-outcome sensitivity.

## Reboot handoff

The experiment is complete. Its systemd service exited with status zero and the
`finish-min-expert-comparison` heartbeat is paused. Checkpoints, replay, exports,
saved binaries and arena evidence are preserved. Final checkpoints are
`runs/min/{cyclic,reverse}/iterations/1/candidate-checkpoint` and
`runs/min/shared-control/iterations/3/candidate-checkpoint` under the artifact
directory; the frozen reference remains in the earlier goal-aware run. See the
artifact directory's `RESTART.md` and `SHA256SUMS` for post-reboot verification.
The original experiment runner is a completed, single-use launcher; subsequent
training should use the appropriate saved checkpoint/run rather than rerun it.
