# Weakness Remediation Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans to implement task-by-task. Preserve the current related working changes.

**Goal:** Replace ordinary training continuation with bounded, reproducible learning from difficult pre-first-finisher positions and independent verification.

**Architecture:** Extend the existing native reanalysis diagnostic to collect valid restart states from saved completed trajectories. Reuse the existing native continuation generator, ReplayStore, RecentReplay and learning diagnostic for focused training; keep complete original trajectories immutable and hold out entire source-game families. No new production service or priority-tree framework.

**Tech Stack:** C++26 / LibTorch / native benchmark probes; CPython for short-lived experiment orchestration and immutable manifests.

**Spec:** User-approved discussion in this chat: preserve long games, restart before the failure, bounded hard replay, refresh targets with completed continuations, validate on unseen families and normal play. Latest user explicitly requests fixing weaknesses before further ordinary training.

## Global Constraints

- General Cyclic/Reverse training remains stopped; heartbeat remains PAUSED. This request authorizes new bounded weakness experiments only.
- Preserve every old checkpoint, replay and user-stop record. Use new artifact directories and copied replay.
- Cyclic and Reverse remain separate experts; final Min uses routing, no distillation or weight averaging.
- Never invent terminal rank/value targets for incomplete games. Reanalysis remains a true resumed game with native rules.
- Compile with parallel12 (32 logical CPUs, about40GB available RAM); run independent CPU probes concurrently. GPU timing/experiments run sequentially to avoid resource interference.
- No architecture changes unless this experiment establishes a need.

## Review Focus

- Encoded states must reconstruct the physical board, actor, full turn order, turn number and finished order exactly; reject invalid features and noncanonical goals.
- A whole source game and all of its branches belong to one split; duplicate physical states cannot cross train/validation.
- Long repetitive games must not dominate the hard pool; cap positions per source game and remove duplicate board/actor/order states.
- Incomplete continuations remain diagnostic-only; completed placement targets must remain native and unchanged.
- Re-running collection/training cannot overwrite existing artifacts or mutate the original checkpoints/replays.

### Task 1: Native state recovery and bounded weakness selection

**Files:** Create benchmarks/weakness_positions.hpp; create benchmarks/weakness_replay_probe.cpp; modify src/CMakeLists.txt.
**Interfaces:** `recover_min_state(const TrainingSample&, const soo::Match&, uint16_t turn_number, const vector<int32_t>& final_order) -> soo::State`; `weakness_positions(const diamond_pipeline::Episode&, const soo::Match&) -> vector<size_t>` selects at most one pre-first anchor for a delayed first finish or repeated pre-first position, plus at most one separately classified completion anchor. Preserve original game identity in callers.

- [x] Write assertion-based probe covering all six turn orders, physical/canonical roundtrip, already-placed players, malformed finished/goal features and duplicate/length caps.
- [x] Compile probe parallel12 and observe missing implementation failure.
- [x] Implement minimal pure helper functions, validate re-encoding against all original features and canonical player IDs.
- [x] Run probe and existing recent_replay_probe concurrently; inspect both outputs.

### Task 2: Collect and reanalyse saved weakness families

**Files:** Modify benchmarks/min_reanalysis.cpp; add small CLI regression coverage alongside the probe/experiment verification.
**Interfaces:** `min_reanalysis --collect RUN_DIR NEW_OUTPUT_DIR` reads completed iteration episode artifacts with their exact resolved config and operation IDs. Emits a source manifest plus endpoints compatible with existing continuation generation. Extend endpoint validation from audited turn501 only to all native in-progress turn numbers with the same board/goal/order/fingerprint integrity. Preserve existing CLI behavior. Fix deterministic duplicate identities to include turn order.

- [x] Exercise currently rejected early restart state and collector CLI, observe failures before implementation.
- [x] Collect pre-first and post-first positions separately, at most one of each per source game; retain game/checkpoint/artifact provenance and native fingerprints.
- [x] Add deterministic original-game-family split and cross-split dynamics+order exclusion to collection output; no incomplete game labels.
- [x] Validate archive against exact source re-encoding, source hashes, no duplicate train/test positions, no source family leakage, and rejection of corrupted endpoint input.

### Task 3: Focused learning and independent validation

**Files:** Extend benchmarks/min_learning_diagnostic.cpp only if needed for bounded unique hard-row selection; reuse existing ReplayStore / RecentReplay. Save bounded experiment commands, hashes, split manifest, outputs and analysis under artifacts/weakness-20261030/.

- [x] Preserve strong group-specific starting checkpoints and isolated replay copies. Produce new completed continuations with fresh native search; keep unresolved results separate.
- [x] Cap duplicate states / per-family rows before hard ingestion. Existing RecentReplay supplies a bounded hard fraction; compare 0/5/10% at equal steps, batch, source weights and AdamW, with whole-family held-out data.
- [x] Run meaningful native roundtrip/target checks and short checkpoint reload training steps. Run independent CPU validations in parallel.
- [x] Measure held-out pre-first legal KL/value prediction and actual failure recovery. Promotion requires independent normal-start playing-strength evaluation; no claim from train loss or completion alone.
- [x] Leave ordinary training and automatic messages disabled. Record the next focused experiment and whether it is running or complete, with exact identities and source compatibility.

## Execution Ledger

Ruling: Reuse current codex/training-control-panel checkout because all existing dirty training changes are prerequisites already developed in this chat; do not reset or bundle unrelated changes into a commit. Cost: final diff review must distinguish inherited changes.
Ruling: Continuous normal self-play/learner decoupling is not needed while the user explicitly pauses ordinary training and prioritizes weakness repair. Use finite targeted restarts now; preserve this throughput improvement as a later ordinary-training task. Cost: ordinary production still retains its old tail barrier until that later task.
Ruling: The prior general-training stop does not authorize resuming old learners or heartbeat. This latest request authorizes newly named bounded weakness runs only.

Task 1 complete: missing-header RED observed; helper GREEN. Parallel12 build passed; weakness_replay_probe and recent_replay_probe executed concurrently and passed. Fixed an invalid already-placed test fixture whose goal camp originally contained another player; rebuilt and verified. No production learner changes.
Ruling: Starting checkpoints for focused comparison are preserved C4096/R13440, and newly generated deep-search source games form the dataset. These checkpoints predate all those source games, avoiding using validation families already trained by the starting checkpoint. Normal retention replay is assembled only from non-hard families outside held-out states; do not copy a replay that contains validation source families.

Task 2 collection GREEN on real data: Cyclic33train/4heldout/12completion roots, Reverse12train/2heldout/4completion roots; each regular retention pool100011rows. Entire original families excluded from regular pool, cross-split native dynamics+turn-order keys excluded. Collector CLI RED was observed on prior binary before adding implementation. Native endpoint validation still to run.
Ruling: Initial pilot has only4Cyclic/2Reverse held-out source families; branches are not independent samples. Treat replay-loss screen as exploratory and require independent normal-start arenas before any strength/default claim.

Task 2 validation complete:12concurrentCLIcases passed (8 valid archives including old500-ply inputs,4 corrupt turn/occupancy/order/fingerprint rejections), plus existing-output-preservation checks for both groups. Real archives contain33/4C and12/2Rpre-first roots.
Task 3: existing native reanalysis extended with --prepare to retain unique pre-first samples, cap16rows/branch64/family, drop whole training families touching held-out states, rebuild normal retention pool excluding held-out continuation states, and preserve original native outcomes. Bounded204focusedcontinuations launched; source checkpointsC4096/R13440 frozen; ordinarytraining/heartbeatremainoff. Active generation metadata in artifacts/weakness-20261030/generation-plan.json and active-generation.json.

Fresh reviewer found two Important provenance checks: saved-state SHA omitted turn/order integrity, and continuation report roots lacked split/first-sample binding. Added canonical SHA checks (explicit legacy spaced JSON schema1 retains turn501 restriction), source-family disjointness, split membership and recovered first-state equality. Corrupted turn/order RED on prior binary, GREEN after fix; both legacy groups remain valid. CLI regression script covers these and family/root corruption on completed native pilot fixtures.

Generation complete:204scheduled,203completed,1Cycliccontinuation unresolved at512moves (zero targets); all64Reverse continuations completed. PreparedCyclic873uniquehard/69heldout/100011normalrows; Reverse256/45/100011. No detected continuation-state overlap families, no normal-state overlap removed. CLI corruption regressions passed for both groups (turn, order, wrong root, cross-split root, source-family leak). Equal-budget six-arm learning pilot launched from frozen C4096/R13440; commands and input hashes recorded in learning-plan.json.

Six learning arms completed; identical initial metrics verified within each group. Actual hard draws0/12800/26112 (0/25/51per512batch); source AdamW restored. Hard10heldout valueMSE C0.063058vscontrol0.082024, R0.255967vs0.283222; legalKL C0.401566vs0.401153 (essentiallyflat), R0.296126vs0.302154. These are exploratory4C/2Rfamily screens, not strength proof. Both hard10checkpoints reloaded on CPU concurrently, restored LR2e-5, completed2realupdates and changed model digests at expected steps4610/13954.
Independent routed hard10vsroutedhard00 normal-start paired arena launched:24new actual-order-state-disjoint openings,1728totalgames,128MCTS,500move cap,seed918514368,bootstrapbywholeopening. First invocation failed preflight only (all-order group retained3weights); preserved failure record, set six equal weights and relaunched before any games. Heartbeat and production learners remain stopped.

Independent normal-start arena completed1728/1728,zeroaborts. Hard10minuscontrol paired first-place contrast: cyclic−0.023148(95%whole-openingCI[−0.106481,+0.069444]);reverse−0.041667([−0.115741,+0.032407]);all−0.032407([−0.081019,+0.020833]). No verified strength improvement; do not promote. A final finite same-root512MCTS held-out recovery check(48continuations,0vs10,4C/2Rfamilies) measures actual deadlock/finish behavior separately. No more learning updates launched.

Final recovery check complete:48/48completed,zeroaborts. Both control and hard10complete all held-out branches. Cmedianfirstfinish18.5→17plies but totalmedian20.5→23.5;R23.5→23and25→24. With only4C/2Rfamilies, no verified failure-recovery gain. Independent binary artifact reader verified original rank targets,873/256unique training states,33/12families,cap64/family and zero shared pre-first validation states. All finite processes exited; no production promotion, no ordinary learner/heartbeat resumed. Next experiment must establish an actual current-model failure before reanalysis rather than equating source-game length with weakness.
