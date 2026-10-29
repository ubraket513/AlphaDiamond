# Goal-aware training

The October 13 audit found two independent defects: arena requests selected a
network using the simulated leaf's player, and neural observations omitted
opponent goal ownership. The arena now keeps the actual mover's `root_player`
through every evaluation in its search. Simulated players still determine
canonical features and value perspective, not which competitor owns the search.

Set `encoder_version` to `diamond-camp-relative-goals-v2` in a new production
configuration. For P players, each board node contains P occupancy planes,
P finished flags, and P goal-membership planes, in that order. The goal planes
follow canonical player order and the actor's spatial rotation. Soo has six
inputs and one value output; Min has nine inputs and three placement values.

An omitted encoder selects legacy v1 and preserves old config serialization.
Old artifacts remain readable. New replay/checkpoints carry the new encoder in
their compatibility identity and cannot be exact-resumed using legacy features.
The legacy frozen fixture corpus is unchanged.

## Starting from an existing model

`--warm-start` can migrate a same-family legacy deployment artifact into a new
goal-aware run. Width and block count must match. It copies all parameters and
buffers, retains the old input-projection columns and zeroes the appended
columns. This preserves the original function up to floating-point reduction
rounding, while allowing gradients to learn the new goal inputs. AdamW and
replay start fresh; the checkpoint records the source artifact and migration
reason. Exact resume continues only within the new compatibility namespace.

```sh
OMP_NUM_THREADS=2 MKL_NUM_THREADS=2 build/native-training/native/alphadiamond-train train \
  --run-dir artifacts/goal-aware-20261013/runs/min/pilot \
  --config configs/alphazero/min-goals-v2-local-pilot.json \
  --warm-start models/min/2.0.1
```

This local pilot uses CUDA, all six turn orders, 384 games and 128 updates per
iteration, 512 simulations, batch 512 and learning rate 0.00002. Its bounded
budget is 12 iterations or two hours, checked at iteration boundaries. Arena
evaluation uses six opening blocks (216 games) and the corrected owner routing.
Internal champion promotion requires a 95% opening-block bootstrap lower bound
of at least +0.05 placement utility. It does not replace a shipped model.

Resume an interrupted run with `alphadiamond-train resume --run-dir <run>`.
Keep the resolved configuration and replay intact. Do not edit their encoder
field or reuse a legacy run directory for migration.

## Verification and limits

`encoder_version_test` exercises every supported standard order/actor case
(four Soo and eighteen Min), all six board rotations, finished-player variants,
action transforms and value mappings. Four legally reachable trajectory
fixtures show the former equal-input aliases; v2 distinguishes them while v1
retains historical behavior. `arena_root_routing_test` changes simulated actors
and verifies root ownership, including placed players. Migration tests cover
both families, prediction parity, new-column gradients and optimizer updates,
version rejection, checkpoint restore and deployment export/reload. The CLI
test runs migrated models through two full training iterations per family.
The native training suite passed all 54 tests, including four CUDA tests.
The Qt inference changes were reviewed but could not be compiled on this
machine because Qt development packages are unavailable.

The local Min pilot started October 14, 2026 at 00:09 KST using implementation
commit `626f8decea067f3f404548d64bbccfe2d0675e4e`. Its first checkpoint validates
at step 128: 382 of 384 games completed, all six turn orders contributed, and
58,410 rows entered replay. Two games reached the move limit. JAX verification
found finite weights and losses, exact preservation of the original weights
at initialization, and nonzero updates in all three appended goal columns.
The launch receipt and numerical checks are in the ignored
`artifacts/goal-aware-20261013/` directory. These first-update checks precede
arena results and do not establish playing-strength improvement.

These checks establish the representation/routing contracts for supported
configurations. They do not enumerate every reachable arrangement or establish
playing-strength improvement. Report strength separately for both canonical
turn-order groups. Independent per-group experts are a future controlled
comparison; they are not part of this run.
