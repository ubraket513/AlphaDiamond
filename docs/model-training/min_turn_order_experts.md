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
OMP_NUM_THREADS=2 MKL_NUM_THREADS=2 build/native-training/native/alphadiamond-train train \
  --run-dir artifacts/min-experts-20261014/runs/min/cyclic \
  --config configs/alphazero/min-expert-cyclic-local-pilot.json \
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
candidate's search through the appropriate expert, while both opposing players
use the frozen shared reference. The outer arena router selects the network from
the actual root player; simulated leaf actors only determine feature/value
perspective.

```sh
OMP_NUM_THREADS=2 MKL_NUM_THREADS=2 build/native-training/native/min_expert_arena \
  --cyclic-checkpoint artifacts/min-experts-20261014/runs/min/cyclic/iterations/1/candidate-checkpoint \
  --reverse-checkpoint artifacts/min-experts-20261014/runs/min/reverse/iterations/1/candidate-checkpoint \
  --shared-checkpoint artifacts/goal-aware-20261013/runs/min/pilot/iterations/0/candidate-checkpoint \
  --shared-control-checkpoint artifacts/min-experts-20261014/runs/min/shared-control/iterations/3/candidate-checkpoint \
  --config configs/alphazero/min-expert-arena-local.json \
  --out artifacts/min-experts-20261014/comparison
```

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
