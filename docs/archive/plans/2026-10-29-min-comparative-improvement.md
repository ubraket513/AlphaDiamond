# Min comparative improvement

## Goal

Demonstrate a measured improvement over the frozen pre-renewal Min specialist pair within the authorized 2026-10-29 16:20 KST deadline. Preserve shipped models and all earlier evidence.

## Architecture

Extend the existing native arena with a second existing MinExpertRouter for specialist references. Retain the shared-reference mode. Train at most two preselected cyclic candidates, screen against the current pair, then confirm one selected candidate on fresh openings. Reverse stays the current final specialist.

## Tech stack and specification

C++ native arena and existing router/checkpoint contracts; existing native trainer; experiment-local Python analysis. Detailed accepted specification: `.superpowers/sdd/2026-10-29-min-improvement/evaluation-design.md`. That document's 12-endpoint family, practical thresholds, simultaneous intervals, identity and abort rules are binding. No source changes to the old primary analyzer.

## Global constraints

User-authorized autonomous execution; no further design approval round. Work on local branches, no worktrees. Preserve user changes. GCC, mold, sccache, Ninja; development O0/g1/pipe/noLTO, optimized Release for real arenas/training. Resource-aware parallel builds and tests alongside one GPU job. Manual/API Stop is binding. Training decision cutoff 14:20 KST; reserve enough time for six confirmation arms plus verification before 16:20. Do not claim improvement from completion or loss.

## Review focus

Exact group routing on both sides; unchanged shared mode; malformed modes fail before output. Pin actual checkpoint and binary identities. Opening seed XOR behavior means adjacent seeds are not fresh. No pending outcomes converted to aborts, no checkpoint cherry-picking, no uncorrected selection on confirmation results.

## Task 1: native reference pair

- [ ] Extend `native/benchmarks/min_expert_arena.cpp`: exclusive shared checkpoint or complete reference cyclic/reverse pair, strict existing group validation, explicit invocation identities and reference mode.
- [ ] Extend focused existing CLI/router tests in the native test suite; inspect applicable files before editing. Validate malformed inputs and both groups, retain existing shared compatibility.
- [ ] Build and test in parallel, using available capacity without starving the active 24-thread pilot. Independent review; fix findings. Refresh Graphify after final supported edits.
- [ ] Build optimized arena and copy it into the NEW comparative experiment directory, recording source/compiler/binary hashes. Never overwrite the old immutable experiment binary.

## Task 2: fixed exploration and analysis

- [ ] Keep already launched search1024 pilot immutable: original teacher warmstart, two iterations, only iteration 1 eligible.
- [ ] Predeclare one additional candidate before any new arena outcomes: current final cyclic warmstart, unchanged 512 search, 512 updates per iteration, eight iterations, only iteration 7 eligible, 60-minute hard bound. Launch only after normal predecessor completion, when budget permits. No partial substitutes or automatic retries.
- [ ] Both candidates retain current final reverse. Use the same exploratory screen seed20261120/count12: RvsR once, each eligible CvsR once. Eligible selection requires positive pessimistic overall and cyclic paired gain, candidate abort rate <=2% overall/3% group, and abort increase <=1pp overall/2pp group. Choose largest pessimistic cyclic paired gain; exact tie selects learning-intensity candidate. If none qualifies, report no demonstrated gain and preserve evidence; no unplanned extra candidates.
- [ ] Freeze new recipe/selection/protocol before screen outcomes; freeze each selected generation before evaluation. Inspect all prior schedules for actual seed/depth collisions, preserving paired-arm reuse. Never consume confirmation for exploration.
- [ ] Implement an experiment-local analyzer validating exact identities/configuration/schedules and complete outcomes, pessimistic paired bounds, all orders/seats/aborts and the accepted simultaneous family. Independently review and exercise meaningful fixtures.

## Task 3: fresh confirmation and reporting

- [ ] Freeze one selected candidate before confirmation seed20261248/count24. Run six arms: RvsR/CvsR, RvsT/CvsT, RvsS/CvsS, identical paired schedules. Only one confirmatory family; no adaptive extensions or candidate replacement.
- [ ] Estimate wall time from screens; start only with sufficient time for all arms and 30-minute verification margin. Keep the final two hours reserved; label incomplete comparisons honestly if externally interrupted.
- [ ] Validate/export candidate checkpoints with existing native tools into experiment directories, record actual steps and hashes; no deployment.
- [ ] Report demonstrated comparisons and failed/inconclusive gates, test/build timings and limitations. At deadline stop only session-owned compute; retain panel and artifacts, pause heartbeat.
