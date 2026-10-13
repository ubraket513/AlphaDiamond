# Min Turn-Order Experts Implementation Plan

> **For agentic workers:** Execute assigned independent components with fresh contexts; root owns integration, build commands, and experimental runs. Use the existing native trainer rather than creating a second training stack.

**Goal:** Train two isolated Min specialists and measure each against a frozen shared Min checkpoint.

**Architecture:** Geometry-based static group classification, independently configured native training runs, and an expert evaluator inside the arena's root-owner router. A native arena harness provides reproducible frozen-reference comparisons.

**Tech Stack:** C++26, CMake/CTest, LibTorch/CUDA; JAX for numerical analysis.

**Spec:** `docs/superpowers/specs/2026-10-14-min-turn-order-experts-design.md`.

## Global constraints

- Preserve frozen fixtures, shipped models, and historical run directories.
- Baseline is the shared goals-v2 checkpoint at step 128, not a moving champion.
- Each specialist gets independent weights, AdamW state, and replay.
- Default config serialization and encoder contracts remain backward compatible.
- Route by full game geometry/order and actual root ownership; never leaf actor.
- All long experimental runs use saved binaries/configs and bounded budgets.

## Review focus

- Actor/finished-player changes must not switch a game's expert.
- Relabeling players or rotating the board must preserve geometric grouping.
- Mixed jobs and lane reuse must use each job's Match, with stable lifetime.
- Group changes on resume must be rejected before optimizer/replay mutation.
- Complete/aborted arena outcomes and fixed comparator identity must remain auditable.

## Tasks

- [x] Core routing: add `soo::MinTurnOrderGroup { cyclic, reverse }`,
  `min_turn_order_group(const Match&)`, names/parser in `soo/min_experts.hpp`;
  `MinExpertRouter` forwards prepare/evaluate. Append `BatchItem::match` and
  propagate in all four scheduler sites. Extend balancing with optional group.
  Test standard permutations, six rotations, relabeling, finish cases, mixed
  batches, real search ownership and lane reuse. Observe red before implementation.
- [x] Training selection: optional `SelfPlayConfig::turn_order_group = "all"`,
  omitted on default serialization. Validate model/balance/count combinations;
  wire the group into `iteration_job`. Test legacy bytes, three-order cycles,
  immutable transitions, isolated warm starts and wrong-group resume rejection.
- [x] Native comparison: add `min_expert_arena` with frozen shared, cyclic and
  reverse checkpoint arguments plus optional shared control. Validate checkpoint
  provenance/groups/shapes; use existing opening schedules and native model
  pools. Persist raw outcomes, source identities and per-arm/group summaries.
  Add a small CPU CLI contract test including negative inputs.
- [x] Integration: run the native training suite and CUDA smoke, obtain fresh
  review, save a clean source commit and executable copies, prepare pilot configs.
- [x] Experiment: stop old shared run after preserving durable work, train both
  specialists (and optional matched control), verify group isolation and updates,
  run the held-out arena and analyze all orders with opening-block statistics.
- [x] Record results and limitations; keep artifacts and checkpoints available.

## Execution ledger

- Implementation committed as `6a960ba850ec01e8e0c7afa9c6561945fcea6749`;
  all 56 tests passed, including four CUDA tests. Independent code review found
  no correctness blocker. Clean-revision binaries and config snapshots are saved.
- Experiment started October 14 at 01:06 KST. The cyclic arm completed 256
  updates with 767/768 games completed and one move-limit abort. JAX verified
  finite changed weights and updates in all three goal-input columns.
- Both specialists completed 256 updates and the matched control completed 512.
  Training totals and runtime model identities passed final verification.
- Both 432-game arena arms finished at 02:27 KST. Specialists had four aborts;
  the control had seven. The six complete paired opening blocks favor experts
  overall by +0.301 utility [95% CI +0.204, +0.431]. All-opening abort sensitivity
  supports the overall and cyclic benefit; extra reverse benefit is less certain.
  Full group/order tables and limitations are recorded in the training document.
- On the user's request, the machine-reboot handoff preserves all checkpoints
  and results. The `finish-min-expert-comparison` heartbeat is paused, and the
  completed experiment has no remaining training or arena process.
