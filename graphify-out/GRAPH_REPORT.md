# Graph Report - AlphaDiamond  (2026-10-29)

## Corpus Check
- cluster-only mode — file stats not available

## Summary
- 5213 nodes · 10486 edges · 389 communities (367 shown, 22 thin omitted)
- Extraction: 93% EXTRACTED · 7% INFERRED · 0% AMBIGUOUS · INFERRED: 714 edges (avg confidence: 0.87)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `070b84d5`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- train_main.cpp
- filesystem
- deployment_artifact.cpp
- RunState
- trainer.cpp / Trainer::train()
- check.hpp / board.hpp
- Match
- encoder_version_test.cpp
- Qt AlphaZero Analysis Console Implementation Plan
- CheckpointLineage
- Native Soo Self-Play — Phase 1 Progress / Environment and dependency status
- run_arm
- SearchSession3P / VectorNode
- load_checkpoint_v2
- AlphaDiamond Python-Zero Proposed PR Sequence
- EncodingVersion
- Diamond — Controller Console
- Native Soo Self-Play — Handoff / Where things stand right now
- Json
- ProductionConfig
- AlphaDiamondEngine
- State
- MaterializedOpening
- training_step_parity_test.cpp / check_cuda_path()
- SearchSession / supply()
- CheckpointInfo
- The promotion arena: what it cost, what was fixed, what is still open
- BatchEvaluator
- Variable / Gaussian
- rating.cpp / ParticipantIdentity
- rating_store.cpp / RatingError
- RTX 3060 GPU Profiling Findings / simulations adopted in the canonical config
- Compatibility / ModelPool::install_checkpoint()
- routes / -08-24-python-zero-migration-ledger.yaml
- replay_segment.cpp / Reader
- SooRatingEvent / MinRatingEvent
- AlphaDiamond Milestone 2 Implementation Plan / EloConfig
- ArenaConfig / arena.cpp
- Min training optimisation record — 2026-08-31
- Starting Min — what is settled, and what is not
- RatingRegistry / add_participant()
- run_state.cpp / RunStateError
- Soo from scratch — training record / Status
- golden_manifest_test.cpp
- JsonValue
- min_experts_test.cpp / ProbeModel
- AlphaDiamond Python-Zero Migration Design / training
- Totals / aborted
- EpisodeLane
- Native Soo Self-Play — Phase 0 Design / Measurements this design rests on
- Options / artifact
- EvalOutcome
- Min A0 Transition and Parallel MCTS Decision Implementation Plan / Codex Execution Order
- mcts3p.cpp
- ReplayStore::Impl
- DiamondModelImpl / DirectionalResidualBlockImpl
- AlphaDiamond Native GPU Training Design / Decision summary
- train() / training_step()
- SelfPlayResult
- replay_store.cpp
- Binary Replay Segments Implementation Plan
- TrainingMetrics / IterationResult
- selfplay.cpp
- Arena / Node
- integer() / non_negative_seed()
- RTX 3060 AlphaZero Training Implementation Plan / Baseline test state
- main
- SelfPlayMetrics / aborted_blocked_cells_total
- EpisodeSearch / Status
- checkpoint_v2_transaction_test.cpp / check_rejected_load_is_unchanged()
- Options / run_once()
- ModelPool / run_once()
- schedule.cpp
- TrainingSample / vacancy_distillation.cpp
- VectorNode
- report.cpp / dispatch_command()
- CommandRequest
- OpeningBlockSummary
- IterationRequest / run_self_play()
- Episode
- Options / arm
- The repetition trigger is implemented but unreachable / training_wiring.cpp
- AlphaDiamond Engine Training repository
- Options / parse()
- runtime-sdk
- OwnerEvaluator
- Rng / gammavariate()
- value() / Fnv
- compatibility / aborted/persistent-replay-v1/Soo/3f3372c174dba4b7bfa9288e2c7e0a33e284dfdc3313f1d073210de8e47df229/manifest.json
- compatibility / capacity-prune/persistent-replay-v1/Soo/3f3372c174dba4b7bfa9288e2c7e0a33e284dfdc3313f1d073210de8e47df229/manifest.json
- compatibility / completed/persistent-replay-v1/Soo/3f3372c174dba4b7bfa9288e2c7e0a33e284dfdc3313f1d073210de8e47df229/manifest.json
- compatibility / corrupt-digest/persistent-replay-v1/Soo/3f3372c174dba4b7bfa9288e2c7e0a33e284dfdc3313f1d073210de8e47df229/manifest.json
- compatibility / deterministic-sampling/persistent-replay-v1/Soo/3f3372c174dba4b7bfa9288e2c7e0a33e284dfdc3313f1d073210de8e47df229/manifest.json
- compatibility / duplicate-id/persistent-replay-v1/Soo/3f3372c174dba4b7bfa9288e2c7e0a33e284dfdc3313f1d073210de8e47df229/manifest.json
- compatibility / rollback/persistent-replay-v1/Soo/3f3372c174dba4b7bfa9288e2c7e0a33e284dfdc3313f1d073210de8e47df229/manifest.json
- training_vector_manifest_test.cpp
- CPU Profiling Findings / Batching wait experiment
- summarize_min_serial_search.py / metric()
- min_experts.cpp
- Repository split implementation plan
- StageTiming / profile_stages()
- 9. Gate D — the Python callback, on CPU
- BatchItem
- AlphaDiamond Python-Zero Migration Implementation Plan / Approval Checklist
- VacancyWithMinValueArm / SooPolicyArm
- config.cpp
- RTX 5060 Ti Parent-Bottleneck Investigation / Inference call / data flow
- steps / with
- selfplay.cpp
- Episode / ModelKey
- checkpoint.cpp
- generate_fixtures.cpp
- SchedulerConfig / scheduler_config()
- ReplayConfig
- pipeline.cpp
- JsonParser / parse_value()
- rating_sync_main.cpp / fail()
- 실행 단위와 순서
- Min A0 transition status — 2026-09-02 / Observability and legal-policy learning
- Options / arm
- diagnose_roots() / parse_options()
- Independent repository and SDK upgrade policy
- PromotionStatisticsConfig
- ResolvedDevice
- Trainer / TrainingConfig
- SearchResult / EvalRecord
- RTX 5060 Ti Worker-Scaling Findings / run_seed
- Tensor
- coordinator.cpp / CoordinatorError
- VisitTargetObservation / summarize_visit_targets()
- OwnerEvaluator
- Golden / GoldenCase
- checkpoint_benchmark.cpp
- training-v1/manifest.json
- Evaluator / MCTS2P
- RTX 3060 GPU Training Runbook / inference
- Batcher
- Min Turn-Order Experts Implementation Plan
- RunResult / aborted_games
- VacancyMetrics / VacancyDistillationResult
- ReplayStore
- CheckpointProvenance
- Min 10-hour autopilot log — 2026-08-31 / A0 transition gate after release
- AlphaDiamond Python-Zero Inventory / Verified baseline
- MinRatedMatch
- DeploymentArtifact / validate_soo_deployment_artifact()
- install() / activate()
- Phase
- EpisodeConfig
- EpisodeMetrics
- SchedulerMetrics / waiting
- main
- topology_gen.cpp / Cube
- Decisions
- 3. What is left
- RTX 3060 Benchmark Findings / Baseline: CPU
- Min Vacancy-Prior Realizability Implementation Plan / Min Vacancy Realizability Diagnostic Handoff — 2026-09-02
- SelfPlayConfig / operator==()
- release_main.cpp
- EvaluationStats / operator==()
- device_test.cpp
- Case / calls
- Native products, one core
- LoadedCheckpoint
- on
- conda_prefix / inputs
- cuda-acceptance / env
- Topology / camp_positions
- MCTSConfig / from_json()
- RunStateStore
- The repetition trigger is implemented but unreachable
- Lane
- BatchFixture
- min_value_head_init_test.cpp / samples()
- Min A0 전환 진단과 다음 실험
- Min: six-order datacenter experiment / run_min_balanced_datacenter.sh
- TrainingConfig / train_steps_per_iteration
- CLAUDE.md
- PolicyTotals
- OpeningBlockBootstrap / bootstrap_opening_blocks()
- Repository split dependency analysis
- Deployment artifact format v3 current contract
- IterationSource / checkpoint_lineage()
- Binary Replay Segments Design
- Case / calls
- Native Windows Qt runtime / Build and package
- steps / with
- Args / parse_args()
- OpeningSuiteConfig / from_json()
- MCTSConfig
- RatingEventOutbox / publish()
- ModelIndexEntry
- export_checkpoint
- ReplayIngestReport / ReplaySamplingStats
- ParameterGroup / parameter_group_name()
- CampDiagnostics / blocker_cells
- Min Vacancy Realizability Diagnostic Handoff — 2026-09-02
- Encoded
- family_case
- Retiring the Python engine
- files / min/after_step_parameters/trunk__output_norm__weight.f32
- Arena, Acceptance, CI, and Repository Tightening Implementation Plan / Task 1: Materialize versioned opening suites
- load_checkpoint
- native-sanitizers / env
- model_index_test.cpp
- diamond_rating
- InferenceConfig / max_batch_size
- WorkerConfig / from_json()
- MinRating / MinLeaderboardEntry
- VacancyDistillationConfig / DistillationArm
- EpisodeDiagnostics / camps
- copy_parameter
- vacancy_distillation_test.cpp / snapshot()
- payload_sha256 / mcts-v1.txt
- VacancyTarget
- Durable Iteration Loop and Replay Transactions Implementation Plan / Task 1: Replace full-pool sampling with bounded selection
- main
- cuda-acceptance.yml / concurrency
- HeldOutMetrics
- RunBudgetConfig
- LearningDiagnosticConfig / batch_size
- PolicyRowDiagnostics / full_cross_entropy
- parameter_diagnostics_test.cpp
- IterationResult
- Native products, one core
- fstream
- rating_sync_test.cpp / write()
- ModelKey
- Engine development build workflow
- training_vector_manifest_test.cpp
- native-core / strategy
- Development build cycle
- The replay store is a durable dataset with a stateless sampler
- NetworkConfig / from_json()
- main
- build_resources_test.sh
- LearningDrift / full_cross_entropy_delta
- runtime_sha256
- Goal-aware training
- Rating v2 Hugging Face synchronization
- Min turn-order specialists / Abort sensitivity and interpretation
- native_training.sh / native_training.sh script
- Min iteration-25 learning diagnostic — 2026-09-01 / Commands
- select_from_visits
- Min next intervention — measured decision / Authorized change
- ResidentModel
- string
- LearningStepDiagnostic / size_t
- CheckpointLoadIntent
- ReplayOpenMode
- Phase / AwaitLeaf
- Resolved: the 768-game completion regression was a match-construction bug
- CommandInterruptedError
- sync_ratings.sh / sync_ratings.sh script
- Goal-aware training implementation plan / Execution ledger
- SooLeaderboardEntry / display_name
- environment/source_commit.txt / byte_count
- environment/torch_version.txt / byte_count
- min/after_step_parameters/policy_head__destination__bias.f32 / byte_count
- min/after_step_parameters/policy_head__destination__weight.f32 / byte_count
- min/after_step_parameters/policy_head__source__bias.f32 / byte_count
- min/after_step_parameters/policy_head__source__weight.f32 / byte_count
- min/after_step_parameters/trunk__adjacency.f32 / byte_count
- min/after_step_parameters/trunk__blocks__0__direction_projections__0__weight.f32 / byte_count
- min/after_step_parameters/trunk__blocks__0__direction_projections__1__weight.f32 / byte_count
- min/after_step_parameters/trunk__blocks__0__direction_projections__2__weight.f32 / byte_count
- min/after_step_parameters/trunk__blocks__0__direction_projections__3__weight.f32 / byte_count
- min/after_step_parameters/trunk__blocks__0__direction_projections__4__weight.f32 / byte_count
- min/after_step_parameters/trunk__blocks__0__direction_projections__5__weight.f32 / byte_count
- min/after_step_parameters/trunk__blocks__0__norm__bias.f32 / byte_count
- min/after_step_parameters/trunk__blocks__0__norm__weight.f32 / byte_count
- min/after_step_parameters/trunk__blocks__0__self_projection__bias.f32 / byte_count
- min/after_step_parameters/trunk__blocks__0__self_projection__weight.f32 / byte_count
- min/after_step_parameters/trunk__input_projection__bias.f32 / byte_count
- min/after_step_parameters/trunk__input_projection__weight.f32 / byte_count
- min/after_step_parameters/trunk__output_norm__bias.f32 / byte_count
- min/after_step_parameters/value_head__0__bias.f32 / byte_count
- min/after_step_parameters/value_head__0__weight.f32 / byte_count
- min/after_step_parameters/value_head__2__bias.f32 / byte_count
- min/after_step_parameters/value_head__2__weight.f32 / byte_count
- min/checkpoint-v1.pt / byte_count
- min/gradients/policy_head__destination__bias.f32 / byte_count
- min/gradients/policy_head__destination__weight.f32 / byte_count
- min/gradients/policy_head__source__bias.f32 / byte_count
- min/gradients/policy_head__source__weight.f32 / byte_count
- min/gradients/trunk__blocks__0__direction_projections__0__weight.f32 / byte_count
- min/gradients/trunk__blocks__0__direction_projections__1__weight.f32 / byte_count
- min/gradients/trunk__blocks__0__direction_projections__2__weight.f32 / byte_count
- min/gradients/trunk__blocks__0__direction_projections__3__weight.f32 / byte_count
- min/gradients/trunk__blocks__0__direction_projections__4__weight.f32 / byte_count
- min/gradients/trunk__blocks__0__direction_projections__5__weight.f32 / byte_count
- min/gradients/trunk__blocks__0__norm__bias.f32 / byte_count
- min/gradients/trunk__blocks__0__norm__weight.f32 / byte_count
- min/gradients/trunk__blocks__0__self_projection__bias.f32 / byte_count
- min/gradients/trunk__blocks__0__self_projection__weight.f32 / byte_count
- min/gradients/trunk__input_projection__bias.f32 / byte_count
- min/gradients/trunk__input_projection__weight.f32 / byte_count
- min/gradients/trunk__output_norm__bias.f32 / byte_count
- min/gradients/trunk__output_norm__weight.f32 / byte_count
- min/gradients/value_head__0__bias.f32 / byte_count
- min/gradients/value_head__0__weight.f32 / byte_count
- min/gradients/value_head__2__bias.f32 / byte_count
- min/gradients/value_head__2__weight.f32 / byte_count
- min/initial_logits.f32 / byte_count
- min/initial_parameters/policy_head__destination__bias.f32 / byte_count
- min/initial_parameters/policy_head__destination__weight.f32 / byte_count
- min/initial_parameters/policy_head__source__bias.f32 / byte_count
- min/initial_parameters/policy_head__source__weight.f32 / byte_count
- min/initial_parameters/trunk__adjacency.f32 / byte_count
- min/initial_parameters/trunk__blocks__0__direction_projections__0__weight.f32 / byte_count
- min/initial_parameters/trunk__blocks__0__direction_projections__1__weight.f32 / byte_count
- min/initial_parameters/trunk__blocks__0__direction_projections__2__weight.f32 / byte_count
- min/initial_parameters/trunk__blocks__0__direction_projections__3__weight.f32 / byte_count
- min/initial_parameters/trunk__blocks__0__direction_projections__4__weight.f32 / byte_count
- min/initial_parameters/trunk__blocks__0__direction_projections__5__weight.f32 / byte_count
- min/initial_parameters/trunk__blocks__0__norm__bias.f32 / byte_count
- min/initial_parameters/trunk__blocks__0__norm__weight.f32 / byte_count
- min/initial_parameters/trunk__blocks__0__self_projection__bias.f32 / byte_count
- min/initial_parameters/trunk__blocks__0__self_projection__weight.f32 / byte_count
- min/initial_parameters/trunk__input_projection__bias.f32 / byte_count
- min/initial_parameters/trunk__input_projection__weight.f32 / byte_count
- min/initial_parameters/trunk__output_norm__bias.f32 / byte_count
- min/initial_parameters/trunk__output_norm__weight.f32 / byte_count
- min/initial_parameters/value_head__0__bias.f32 / byte_count
- min/initial_parameters/value_head__0__weight.f32 / byte_count
- min/initial_parameters/value_head__2__bias.f32 / byte_count
- min/initial_parameters/value_head__2__weight.f32 / byte_count
- min/initial_values.f32 / byte_count
- min/inputs.f32 / byte_count
- min/losses.f32 / byte_count
- min/optimizer/policy_head__destination__bias__exp_avg.f32 / byte_count
- min/optimizer/policy_head__destination__bias__exp_avg_sq.f32 / byte_count
- min/optimizer/policy_head__destination__bias__step.i64 / byte_count
- min/optimizer/policy_head__destination__weight__exp_avg.f32 / byte_count
- min/optimizer/policy_head__destination__weight__exp_avg_sq.f32 / byte_count
- min/optimizer/policy_head__destination__weight__step.i64 / byte_count
- min/optimizer/policy_head__source__bias__exp_avg.f32 / byte_count
- min/optimizer/policy_head__source__bias__exp_avg_sq.f32 / byte_count
- min/optimizer/policy_head__source__bias__step.i64 / byte_count
- min/optimizer/policy_head__source__weight__exp_avg.f32 / byte_count
- min/optimizer/policy_head__source__weight__exp_avg_sq.f32 / byte_count
- min/optimizer/policy_head__source__weight__step.i64 / byte_count
- min/optimizer/trunk__blocks__0__direction_projections__0__weight__exp_avg.f32 / byte_count
- min/optimizer/trunk__blocks__0__direction_projections__0__weight__exp_avg_sq.f32 / byte_count
- min/optimizer/trunk__blocks__0__direction_projections__0__weight__step.i64 / byte_count
- min/optimizer/trunk__blocks__0__direction_projections__1__weight__exp_avg.f32 / byte_count
- min/optimizer/trunk__blocks__0__direction_projections__1__weight__exp_avg_sq.f32 / byte_count
- min/optimizer/trunk__blocks__0__direction_projections__1__weight__step.i64 / byte_count
- min/optimizer/trunk__blocks__0__direction_projections__2__weight__exp_avg.f32 / byte_count
- min/optimizer/trunk__blocks__0__direction_projections__2__weight__exp_avg_sq.f32 / byte_count
- min/optimizer/trunk__blocks__0__direction_projections__2__weight__step.i64 / byte_count
- min/optimizer/trunk__blocks__0__direction_projections__3__weight__exp_avg.f32 / byte_count
- min/optimizer/trunk__blocks__0__direction_projections__3__weight__exp_avg_sq.f32 / byte_count
- cpu8h-soo-20260819 — a pinned fixture, not a live run / P5090-w16-g48/provenance.txt
- Lane
- The replay path: measured before porting anything / Still unmeasured
- InferencePrecision
- Golden
- Result / games
- run_arm() / run_min_serial_search_sweep.sh
- TrainAlphaDiamond
- build_resources.sh
- Python-zero benchmark disposition
- Min serial-search sweep — 2026-09-01
- Min topology repair and fresh restart
- push_checkpoints.sh / push_checkpoints.sh script
- Arguments
- probe-a-evaluator-batch-scaling
- provenance
- provenance
- Legal trajectory from the standard opening; generated by the 2026-10-13 representation audit.
- Legal trajectory from the standard opening; generated by the 2026-10-13 representation audit.
- alphadiamond mcts golden v1
- alphadiamond rules golden v1
- torch_version
- libtorch_root
- 0. Where things stand right now
- moves_from
- AGENTS.md
- 2026-09-10-min-training-wrap-up.md
- 2026-10-14-min-turn-order-experts-design.md
- cpu8h-soo-20260819/README.md
- iteration_job
- Status

## God Nodes (most connected - your core abstractions)
1. `string()` - 343 edges
2. `files` - 100 edges
3. `Match` - 78 edges
4. `State` - 76 edges
5. `Native Soo Self-Play — Phase 1 Progress` - 74 edges
6. `Compatibility` - 72 edges
7. `AlphaDiamond Python-Zero Proposed PR Sequence` - 70 edges
8. `ProductionConfig` - 66 edges
9. `SearchSession` - 56 edges
10. `TrainingSample` - 53 edges

## Surprising Connections (you probably didn't know these)
- `Gate record` --references--> `MCTS2P`  [INFERRED]
  docs/native-selfplay/native_windows_runtime.md → native/include/soo/mcts.hpp
- `1. Two defects, one untested function` --references--> `EpisodeConfig`  [INFERRED]
  docs/model-training/min_arena_throughput.md → native/include/soo/selfplay.hpp
- `Task 4: Add legal-set policy-fit diagnostics to `selfplay_benchmark`` --references--> `EpisodeDiagnostics`  [INFERRED]
  2026-09-02-min-a0-transition-roadmap.md → native/include/soo/selfplay.hpp
- `An observability gap this exposed` --references--> `SelfPlayResult`  [INFERRED]
  docs/model-training/repetition_trigger_config_gap.md → native/include/diamond_pipeline/pipeline.hpp
- `7. Self-play, batching, and abort semantics` --references--> `TrainingSample`  [INFERRED]
  docs/superpowers/specs/2026-08-25-native-gpu-training.md → native/include/diamond_training/training_sample.hpp

## Import Cycles
- None detected.

## Communities (389 total, 22 thin omitted)

### Community 0 - "train_main.cpp"
Cohesion: 0.16
Nodes (32): CommandArtifactError, actor_precision(), aggregate_result(), authoritative_request(), candidate_checkpoint(), deployment_training_step(), describe(), evaluate() (+24 more)

### Community 1 - "filesystem"
Cohesion: 0.14
Nodes (6): Json, main(), parse_args(), run(), sample(), SooModelImpl

### Community 2 - "deployment_artifact.cpp"
Cohesion: 0.18
Nodes (25): absolute_normal(), add_tensor(), deployment_runtime_sha256(), expected_weights(), integer_field(), is_hex_digest(), load_model_index(), read_bytes() (+17 more)

### Community 3 - "RunState"
Cohesion: 0.09
Nodes (19): RunStage, complete, initialize, persist, promote_or_reject, promotion_arena, rating_benchmark, replay_ingest (+11 more)

### Community 4 - "trainer.cpp / Trainer::train()"
Cohesion: 0.06
Nodes (46): ModelRole, actor, candidate, learner, AcceleratorStepEvents, backward_end, backward_start, forward_end (+38 more)

### Community 5 - "check.hpp / board.hpp"
Cohesion: 0.11
Nodes (10): MinTurnOrderGroup, Json, Clock, log_gamma(), lower_incomplete_gamma(), evaluate_vector(), mix(), mix_u32() (+2 more)

### Community 6 - "Match"
Cohesion: 0.11
Nodes (14): Match, count, players, MCTS2P::MCTS2P(), SearchSession::SearchSession(), check_match(), check_no_target_start_overlap(), check_opening() (+6 more)

### Community 7 - "encoder_version_test.cpp"
Cohesion: 0.17
Nodes (17): check_goal_planes(), check_reachable_alias(), check_rotations(), check_search(), check_supported_combinations(), check_version_contract(), expect_invalid_argument(), expected_goal_camp() (+9 more)

### Community 8 - "Qt AlphaZero Analysis Console Implementation Plan"
Cohesion: 0.31
Nodes (8): Global Constraints, Qt AlphaZero Analysis Console Implementation Plan, Task 1: Search telemetry semantics and timing, Task 2: Structured worker result and telemetry math, Task 3: Persistent Soo runtime and controller lifecycle, Task 4: Analysis console and relocated history, Task 5: Regression and release verification, root_mean_value

### Community 9 - "CheckpointLineage"
Cohesion: 0.12
Nodes (16): CheckpointInitializationMode, audited_legacy_import, resume, scratch, warm_start, CheckpointLineage, initialization_mode, iteration (+8 more)

### Community 10 - "Native Soo Self-Play — Phase 1 Progress / Environment and dependency status"
Cohesion: 0.08
Nodes (44): 10.0 The host, 10.1 The Python backend, re-measured on this host, 10.2 Risk 5 — does a real GPU forward behind the GIL recreate the ceiling?, 10.3 The A/B throughput table, 10.4 A caveat that had to be measured, not assumed, 10.5 What this means for `selfplay_backend = "native"`, 10.5 What this means for selfplaybackend = "native", 10.6 Verdict (+36 more)

### Community 11 - "run_arm"
Cohesion: 0.15
Nodes (18): Counts, aborted, completed, placements, scheduled, integers(), Json, ordered_match() (+10 more)

### Community 12 - "SearchSession3P / VectorNode"
Cohesion: 0.05
Nodes (26): EvalOutcome3P, priors, value, SearchSession3P, budget_seconds_, config_, edges_, node_ (+18 more)

### Community 13 - "load_checkpoint_v2"
Cohesion: 0.26
Nodes (13): collect_named_tensors(), fresh_model_like(), load_checkpoint_v2(), load_checkpoint_v2_weights(), load_checkpoint_v3(), load_model_archive(), load_optimizer_archive(), NamedTensors (+5 more)

### Community 14 - "AlphaDiamond Python-Zero Proposed PR Sequence"
Cohesion: 0.07
Nodes (23): AlphaDiamond Python-Zero Proposed PR Sequence, PR 00 — Baseline, acceptance, and branch-protection integrity, PR 01 — Close native contract coverage gaps, PR 02 — Retire duplicate Python action and canonical encoders, PR 03 — Productionize the native model and freeze training vectors, PR 04 — Native LibTorch training step, PR 05 — Native checkpoint, legacy import, and resume, PR 06 — Native replay schema and persistent store (+15 more)

### Community 15 - "EncodingVersion"
Cohesion: 0.20
Nodes (12): EncodingVersion, goals_v2, legacy_v1, encode(), encoded_feature_count(), encoding_version_from_name(), encoding_version_name(), Compatibility::family() (+4 more)

### Community 16 - "Diamond — Controller Console"
Cohesion: 0.05
Nodes (38): Architecture, Board geometry, Board rendering, Camps — the Diamond rule, Canonical path policy, Collapsing the panel, Colour, Controller workflow (+30 more)

### Community 17 - "Native Soo Self-Play — Handoff / Where things stand right now"
Cohesion: 0.09
Nodes (32): 1. What this project is, 2. Gate status, 4. Environment setup on a new host, 5. How to run the benchmarks, 6. Invariants that must not break, 7.10 A permissive RNG policy makes stream tests worthless (Gate E), 7.11 A training sample is the root, not the pending node (Gate F), 7.12 A deterministic parity test may be comparing nothing (Gate F) (+24 more)

### Community 18 - "Json"
Cohesion: 0.32
Nodes (17): ArenaConfig::to_json(), InferenceConfig::to_json(), Json, json_integer(), MCTSConfig::to_json(), NetworkConfig::to_json(), OpeningSuiteConfig::to_json(), optional_json() (+9 more)

### Community 19 - "ProductionConfig"
Cohesion: 0.09
Nodes (23): Hugging Face handoff, Task 4: 기존 저장소에서 Qt를 SDK 소비자로 전환, ProductionConfig, arena, encoder_version, mcts, model_name, model_version (+15 more)

### Community 20 - "AlphaDiamondEngine"
Cohesion: 0.25
Nodes (11): Threads dependency rediscovery, Torch exact version dependency rediscovery, AlphaDiamondEngine, SDK canonical topology and Soo Min fixtures, Runtime SDK product boundary, diamond_artifact, diamond_model, diamond_pipeline (+3 more)

### Community 21 - "State"
Cohesion: 0.09
Nodes (27): The order it happened in, and why that order, Resolution, 8. Phase 1 implementation plan, Mutation results, Steps 1–4 — prior, rules, state, encoder, Simulation breakdown, PlayerSpec, camp (+19 more)

### Community 22 - "MaterializedOpening"
Cohesion: 0.14
Nodes (10): opening(), MaterializedOpening, depth, opening_id, seed, serialized_state, state_sha256, SooOpeningBlock (+2 more)

### Community 23 - "training_step_parity_test.cpp / check_cuda_path()"
Cohesion: 0.16
Nodes (30): device_, model_, optimizer_, main(), check_close(), check_cuda_close(), check_cuda_path(), check_deep_storage() (+22 more)

### Community 24 - "SearchSession / supply()"
Cohesion: 0.05
Nodes (25): Native tree representation, SearchSession, arena_, budget_seconds_, config_, node_, noise_, path_ (+17 more)

### Community 25 - "CheckpointInfo"
Cohesion: 0.19
Nodes (14): CheckpointInfo, format_version, generation, lineage, model_digest, optimizer_digest, provenance, training_step (+6 more)

### Community 26 - "The promotion arena: what it cost, what was fixed, what is still open"
Cohesion: 0.20
Nodes (11): 1. Two defects, one untested function, 2. The fix, 3. Measured: a Min iteration on one RTX 4090, 4. The completion curve separates the two effects, 5. Where to pick up, 6. Setup notes for the next machine, Tested, and each assertion verified by breaking what it covers, The promotion arena: what it cost, what was fixed, what is still open (+3 more)

### Community 27 - "BatchEvaluator"
Cohesion: 0.19
Nodes (4): ArenaModelRouter, candidate_player_by_job_, MinExpertRouter, BatchEvaluator

### Community 28 - "Variable / Gaussian"
Cohesion: 0.09
Nodes (24): Gaussian, pi, tau, IdentityFactor, inverse_normal_cdf(), LikelihoodFactor, variance, normal_cdf() (+16 more)

### Community 29 - "rating.cpp / ParticipantIdentity"
Cohesion: 0.13
Nodes (31): ParticipantIdentity, display_name, full_identity, participant_id, canonical_participant_id(), event_digest(), expected_elo_score(), int_array() (+23 more)

### Community 30 - "rating_store.cpp / RatingError"
Cohesion: 0.12
Nodes (32): RatingError, RatingStoreFailpoint, before_event_commit, before_receipt_commit, EloConfig::validate(), atomic_write(), completed(), elo_config() (+24 more)

### Community 31 - "RTX 3060 GPU Profiling Findings / simulations adopted in the canonical config"
Cohesion: 0.09
Nodes (35): 64 simulations adopted in the canonical config, Adaptive wait: measured, and deliberately not merged, Bistability status: **eliminated**, Caveat: this host is not the host of the runs above, Coordinator batch-window fix validation, Correctness, CPU test suite, Effect on the recommended-work ranking (+27 more)

### Community 32 - "Compatibility / ModelPool::install_checkpoint()"
Cohesion: 0.07
Nodes (19): Compatibility, action_space_version, board_topology_version, encoder_version, model_name, model_version, network_config, player_count (+11 more)

### Community 33 - "routes / -08-24-python-zero-migration-ledger.yaml"
Cohesion: 0.06
Nodes (35): baseline_generated_from, baseline_tracked_python_file_count, category_definitions, A, B, C, D, current_tracked_python_file_count (+27 more)

### Community 34 - "replay_segment.cpp / Reader"
Cohesion: 0.17
Nodes (16): append_bytes(), append_count(), append_float(), append_string(), append_u32(), append_u64(), compatibility_digest(), decode_replay_segment() (+8 more)

### Community 35 - "SooRatingEvent / MinRatingEvent"
Cohesion: 0.06
Nodes (27): MinRatingEvent, completed, event_id, final_ranking, game_id, opening_id, participant_identities, participant_ids (+19 more)

### Community 36 - "AlphaDiamond Milestone 2 Implementation Plan / EloConfig"
Cohesion: 0.10
Nodes (30): AlphaDiamond Milestone 2 Implementation Plan, File Structure, Global Constraints, Plan Self-Review, Task 10: Atomic Training Run State, Task 11: Iterative Training Coordinator and Resume, Task 12: Headless CLI and Complete CPU Smoke, Task 13: FP32 Profiling and Controlled Numeric Modes (+22 more)

### Community 37 - "ArenaConfig / arena.cpp"
Cohesion: 0.08
Nodes (35): ArenaMatchup2, candidate_player, turn_order, ArenaMatchup3, candidate_player, turn_order, MinArenaExecutor, MinArenaResult (+27 more)

### Community 38 - "Min training optimisation record — 2026-08-31"
Cohesion: 0.06
Nodes (32): Adopted optimisations, Arena removal from the B0 production loop, Automatic capacity pruning and progressive JSON retirement, Binary replay segments, Current steady-state budget, Fixed training invariants, Measured replay result, Min training optimisation record — 2026-08-31 (+24 more)

### Community 39 - "Starting Min — what is settled, and what is not"
Cohesion: 0.22
Nodes (13): 1. Why Min could not start: a setting nothing read, 2. Which prior: `alphadiamond-min-probe`, 3. Soo as a Min teacher: rejected, 4. The scratch value head: zero, not random, 5. Not yet measured, 6. What the next machine needs, 7. Where to pick up, Build (+5 more)

### Community 40 - "RatingRegistry / add_participant()"
Cohesion: 0.10
Nodes (24): Task 3: Soo Elo and Replayable Registry, Family, min, soo, RatingRegistry, elo_, events_, family_ (+16 more)

### Community 41 - "run_state.cpp / RunStateError"
Cohesion: 0.19
Nodes (23): RunStateError, append_unique_event(), atomic_write(), Json, next_stage(), operation_id_is_valid(), optional_string_field(), RunState::derive_seed() (+15 more)

### Community 42 - "Soo from scratch — training record / Status"
Cohesion: 0.11
Nodes (32): 1. Status, 2. Why heuristics on, and what turns them off, 3. Configuration, and why, 4. Operating it, 5.10 B0 *does* improve A0 robustness — the corollary was wrong, 5.11 The replay store had to be bounded first, 5.1 First block, 384 games/iteration (superseded), 5.2 Second block, 768 games/iteration (+24 more)

### Community 43 - "golden_manifest_test.cpp"
Cohesion: 0.23
Nodes (6): Interrupted, integer_field(), is_lower_hex(), main(), read_bytes(), safe_relative_path()

### Community 44 - "JsonValue"
Cohesion: 0.23
Nodes (8): JsonValue, value, compatibility_from_json(), integer(), json_string_array(), legacy_sample(), number(), sample_json()

### Community 45 - "min_experts_test.cpp / ProbeModel"
Cohesion: 0.14
Nodes (21): check_geometric_groups(), check_lane_reuse(), check_mixed_batch_and_finish_stability(), check_scheduler_match(), check_search_ownership(), check_selected_balancing(), expect_invalid(), Group (+13 more)

### Community 46 - "AlphaDiamond Python-Zero Migration Design / training"
Cohesion: 0.09
Nodes (34): 10. Rollback strategy, 11. Approval decisions, 1. Decision summary, 2. Goals and non-goals, 3.1 Recommended: vertical-slice extinction, 3.2 Conservative alternative: bridge-first, 3.3 Rejected: aggressive big-bang, 3. Migration approaches (+26 more)

### Community 47 - "Totals / aborted"
Cohesion: 0.07
Nodes (29): Totals, aborted, attempted, batch_sizes, batches, boosted_moves, collation_seconds, completed (+21 more)

### Community 48 - "EpisodeLane"
Cohesion: 0.05
Nodes (31): balance_episode_turn_orders(), blend_legal_priors(), DummyBatchEvaluator::evaluate(), EpisodeLane, bootstrap_priors, camp_changed_ply, camp_snapshot, deadline (+23 more)

### Community 49 - "Native Soo Self-Play — Phase 0 Design / Measurements this design rests on"
Cohesion: 0.11
Nodes (29): 0.1 Where the system's CPU goes, 0.2 What the multiprocessing architecture costs, 0.3 The search trajectory is not distorted by a constant evaluator, 0.4 Value-only inference (B0 ABI question), 0.5 What is *not* yet known, 0. Measurements this design rests on, 10. Go / no-go summary, 1. Current Soo self-play hot path (+21 more)

### Community 50 - "Options / artifact"
Cohesion: 0.07
Nodes (28): Options, artifact, artifact_explicit, bootstrap_prior, checkpoint, config, device, diagnostic_batch (+20 more)

### Community 51 - "EvalOutcome"
Cohesion: 0.18
Nodes (8): EvalOutcome, priors, value, values, ModelPool::evaluate(), SooEvaluator::evaluate(), check_outcomes_unchanged(), same_outcome()

### Community 52 - "Min A0 Transition and Parallel MCTS Decision Implementation Plan / Codex Execution Order"
Cohesion: 0.13
Nodes (22): Codex Execution Order, Expected Decision Value, Global Constraints, Min A0 Transition and Parallel MCTS Decision Implementation Plan, Phase 0: Establish a Reproducible, Read-Only Baseline, Phase 2: Enable and Measure Stronger Serial Search, Phase 3A: Conditional Parallel MCTS Implementation, Phase 3B: Conditional Vacancy-Prior Annealing (+14 more)

### Community 53 - "mcts3p.cpp"
Cohesion: 0.08
Nodes (24): SearchResult3P, evaluator_calls, nodes_created, policy, q_vectors, root_actions, root_priors, selected_action (+16 more)

### Community 54 - "ReplayStore::Impl"
Cohesion: 0.07
Nodes (26): ReplayContents, full, metadata_only, EpisodeRecord, aborted_reason, completed, game_id, move_count (+18 more)

### Community 55 - "DiamondModelImpl / DirectionalResidualBlockImpl"
Cohesion: 0.11
Nodes (10): DiamondModelImpl, adjacency, blocks, input_projection, neighbour_index_, neighbour_missing_, neighbour_tables_version_, output_norm (+2 more)

### Community 56 - "AlphaDiamond Native GPU Training Design / Decision summary"
Cohesion: 0.10
Nodes (26): 10. Checkpoint and initialization manifest v3, 11. Durable orchestration, 13. Metrics and benchmarks, 14. Testing and CI, 15. Delivery boundaries and defect coverage, 16. Operating and verification policy, 17. Completion and hardware gate, 1. Decision summary (+18 more)

### Community 57 - "train() / training_step()"
Cohesion: 0.23
Nodes (9): check_adamw_equal(), check_cuda_roundtrips(), check_tensor_device(), check_tensor_equal(), check_trainer_device_state(), compatibility(), main(), rejects() (+1 more)

### Community 58 - "SelfPlayResult"
Cohesion: 0.10
Nodes (18): SelfPlayResult, aborted_diagnostics, aborted_games, completed_games, episodes, metrics, new_samples, operation_id (+10 more)

### Community 59 - "replay_store.cpp"
Cohesion: 0.11
Nodes (22): atomic_write(), chunk_body(), compatibility_json(), encoding_from_string(), encoding_name(), extended_windows_path(), failure_injected(), next_splitmix() (+14 more)

### Community 60 - "Binary Replay Segments Implementation Plan"
Cohesion: 0.15
Nodes (13): Binary Replay Segments Implementation Plan, Global Constraints, Self-review, Task 1: Deterministic binary-v1 codec, Task 2: Schema 5 mixed-format replay store, Task 3: Timed ingest and reopen comparison, Task 4: Boundary deployment and verification, chunk_file_count() (+5 more)

### Community 61 - "TrainingMetrics / IterationResult"
Cohesion: 0.13
Nodes (15): TrainingMetrics, backward_seconds, collation_seconds, forward_seconds, h2d_seconds, optimizer_seconds, peak_cuda_allocated_bytes, peak_cuda_memory_available (+7 more)

### Community 62 - "selfplay.cpp"
Cohesion: 0.13
Nodes (23): PromotionManifest, checkpoint_generation, checkpoint_sha256, model_family, payload, state, PromotionState, archival (+15 more)

### Community 63 - "Arena / Node"
Cohesion: 0.08
Nodes (19): Arena, edges_, nodes_, Edge, action, child, prior, value_sum (+11 more)

### Community 64 - "integer() / non_negative_seed()"
Cohesion: 0.19
Nodes (18): ArenaConfig::from_json(), boolean(), InferenceConfig::from_json(), integer(), MCTSConfig::from_json(), NetworkConfig::from_json(), non_negative_seed(), number() (+10 more)

### Community 65 - "RTX 3060 AlphaZero Training Implementation Plan / Baseline test state"
Cohesion: 0.13
Nodes (23): Baseline test state, Benchmark Procedure, File/Component Map, Findings that change the design (discovered, not in the prompt), Global Constraints, Implementation Tasks, Pinned source-checkpoint facts (do not re-derive; do not hard-code `72` in code), Rollback Criteria (+15 more)

### Community 66 - "main"
Cohesion: 0.22
Nodes (6): ProductionConfig::from_json(), main(), rejects(), rejects_with_message(), require(), runtime_json()

### Community 67 - "SelfPlayMetrics / aborted_blocked_cells_total"
Cohesion: 0.06
Nodes (41): Checkpoint validation, Effective iteration-25 config, Min A0 Baseline — 2026-09-01, PR 1 verification and scope, Release artifact verification, search_targets, SelfPlayMetrics, aborted_blocked_cells_total (+33 more)

### Community 68 - "EpisodeSearch / Status"
Cohesion: 0.09
Nodes (10): EpisodeSearch, root_, root_captured_, root_encoded_, simulations_, three_, two_, Status (+2 more)

### Community 69 - "checkpoint_v2_transaction_test.cpp / check_rejected_load_is_unchanged()"
Cohesion: 0.19
Nodes (17): check_adamw_equal(), check_rejected_load_is_unchanged(), check_snapshot(), check_tensor_equal(), clone_trainer(), compatibility(), main(), make_trained() (+9 more)

### Community 70 - "Options / run_once()"
Cohesion: 0.10
Nodes (21): checkpoint_provenance(), main(), make_match(), make_request(), one_move_finish(), Options, batch_size, device (+13 more)

### Community 71 - "ModelPool / run_once()"
Cohesion: 0.11
Nodes (11): run_once(), ModelPool, active_, capacity_, device_, models_, precision_, staging_features_ (+3 more)

### Community 72 - "schedule.cpp"
Cohesion: 0.11
Nodes (39): Task 8: Streaming inference metrics and summaries, Task 2: Use complete Soo pairs and Min triples without pseudo-replicates, 12. Opening suites, arena, and promotion, MaterializedOpeningSuite, config, openings, suite_sha256, ScheduleError (+31 more)

### Community 73 - "TrainingSample / vacancy_distillation.cpp"
Cohesion: 0.16
Nodes (14): TrainingSample, canonical_player_ids, compatibility, node_features, value_target, evaluate(), Evaluation, metrics (+6 more)

### Community 74 - "VectorNode"
Cohesion: 0.25
Nodes (8): VectorNode, edge_begin, edge_count, expanded, seat, state, terminal, total_visits

### Community 75 - "report.cpp / dispatch_command()"
Cohesion: 0.21
Nodes (13): CommandArgumentError, CommandService, dispatch_command(), emit_error(), is_verb(), Json, load_config(), make_command_report() (+5 more)

### Community 76 - "CommandRequest"
Cohesion: 0.09
Nodes (20): CommandInitialization, native_checkpoint, none, scratch, warm_start, CommandRequest, candidate_path, champion_path (+12 more)

### Community 77 - "OpeningBlockSummary"
Cohesion: 0.09
Nodes (22): MinOpeningBlock, matches, opening, MinOpeningBlockResult, opening_id, results, MinRatedResult, candidate_placement (+14 more)

### Community 78 - "IterationRequest / run_self_play()"
Cohesion: 0.11
Nodes (17): CancelledError, DeadlineExceededError, IterationRequest, checkpoint_root, compatibility, iteration, jobs, match (+9 more)

### Community 79 - "Episode"
Cohesion: 0.14
Nodes (14): Self-Review, Episode, completed, diagnostics, finish_order, max_game_seconds_exceeded, move_count, move_limit_exceeded (+6 more)

### Community 80 - "Options / arm"
Cohesion: 0.07
Nodes (27): JsonArray, JsonObject, main(), opening(), Options, arm, artifact, device (+19 more)

### Community 81 - "The repetition trigger is implemented but unreachable / training_wiring.cpp"
Cohesion: 0.21
Nodes (9): EvaluationPipelineWiring, activate_candidate, record_rating, run_arena, native_int(), validate_training_config_transition(), wire_arena_episode(), wire_evaluation_pipeline() (+1 more)

### Community 82 - "AlphaDiamond Engine Training repository"
Cohesion: 0.18
Nodes (11): Engine only CMake entry, Engine SDK and training package output, Separate inference and training build options, Frozen golden corpus, Historical decisions and current ownership, Independent Engine Desktop release decision, Python engine retirement, AlphaDiamond Engine Training repository (+3 more)

### Community 83 - "Options / parse()"
Cohesion: 0.14
Nodes (13): Options, batch, checkpoint, config, device, eval_batch, eval_samples, iteration (+5 more)

### Community 84 - "runtime-sdk"
Cohesion: 0.05
Nodes (37): run, LIBTORCH_VERSION, MKL_NUM_THREADS, OMP_NUM_THREADS, SCCACHE_GHA_ENABLED, config, os, jobs (+29 more)

### Community 85 - "OwnerEvaluator"
Cohesion: 0.25
Nodes (5): OwnerEvaluator, evaluated, features_, foreign_leaves, prepared

### Community 87 - "value() / Fnv"
Cohesion: 0.20
Nodes (13): Fnv, hash_, main(), hash_doubles(), hash_i32(), hash_trace(), hash_visits(), load() (+5 more)

### Community 88 - "compatibility / aborted/persistent-replay-v1/Soo/3f3372c174dba4b7bfa9288e2c7e0a33e284dfdc3313f1d073210de8e47df229/manifest.json"
Cohesion: 0.10
Nodes (20): aborted, capacity, chunks, compatibility, action_space_version, board_topology_version, encoder_version, model_name (+12 more)

### Community 89 - "compatibility / capacity-prune/persistent-replay-v1/Soo/3f3372c174dba4b7bfa9288e2c7e0a33e284dfdc3313f1d073210de8e47df229/manifest.json"
Cohesion: 0.10
Nodes (20): aborted, capacity, chunks, compatibility, action_space_version, board_topology_version, encoder_version, model_name (+12 more)

### Community 90 - "compatibility / completed/persistent-replay-v1/Soo/3f3372c174dba4b7bfa9288e2c7e0a33e284dfdc3313f1d073210de8e47df229/manifest.json"
Cohesion: 0.10
Nodes (20): aborted, capacity, chunks, compatibility, action_space_version, board_topology_version, encoder_version, model_name (+12 more)

### Community 91 - "compatibility / corrupt-digest/persistent-replay-v1/Soo/3f3372c174dba4b7bfa9288e2c7e0a33e284dfdc3313f1d073210de8e47df229/manifest.json"
Cohesion: 0.10
Nodes (20): aborted, capacity, chunks, compatibility, action_space_version, board_topology_version, encoder_version, model_name (+12 more)

### Community 92 - "compatibility / deterministic-sampling/persistent-replay-v1/Soo/3f3372c174dba4b7bfa9288e2c7e0a33e284dfdc3313f1d073210de8e47df229/manifest.json"
Cohesion: 0.10
Nodes (20): aborted, capacity, chunks, compatibility, action_space_version, board_topology_version, encoder_version, model_name (+12 more)

### Community 93 - "compatibility / duplicate-id/persistent-replay-v1/Soo/3f3372c174dba4b7bfa9288e2c7e0a33e284dfdc3313f1d073210de8e47df229/manifest.json"
Cohesion: 0.10
Nodes (20): aborted, capacity, chunks, compatibility, action_space_version, board_topology_version, encoder_version, model_name (+12 more)

### Community 94 - "compatibility / rollback/persistent-replay-v1/Soo/3f3372c174dba4b7bfa9288e2c7e0a33e284dfdc3313f1d073210de8e47df229/manifest.json"
Cohesion: 0.10
Nodes (20): aborted, capacity, chunks, compatibility, action_space_version, board_topology_version, encoder_version, model_name (+12 more)

### Community 95 - "training_vector_manifest_test.cpp"
Cohesion: 0.33
Nodes (5): bytes(), integer_field(), main(), require_keys(), sha256()

### Community 96 - "CPU Profiling Findings / Batching wait experiment"
Cohesion: 0.14
Nodes (20): Batching wait experiment, Before vs after, Correctness, Correctness, CPU concurrency comparison, CPU NN batch scaling, CPU Profiling Findings, CPU root cause (+12 more)

### Community 97 - "summarize_min_serial_search.py / metric()"
Cohesion: 0.21
Nodes (11): main(), adaptive_search_wins(), completion(), deeper_search_helps(), illegal_mass_dominated(), load_scale(), main(), metric() (+3 more)

### Community 98 - "min_experts.cpp"
Cohesion: 0.30
Nodes (8): canonical_geometry(), Geometry, min_turn_order_group(), min_turn_order_group_from_name(), min_turn_order_group_name(), MinExpertRouter::evaluate(), MinExpertRouter::prepare(), request_group()

### Community 100 - "StageTiming / profile_stages()"
Cohesion: 0.08
Nodes (25): decode_action(), encode_action(), PieceSet, StageTiming, apply_ns, encode_ns, evaluations, legal_ns (+17 more)

### Community 101 - "9. Gate D — the Python callback, on CPU"
Cohesion: 0.29
Nodes (7): 9.1 The callback ABI, 9.2 GIL discipline, 9.4 A bug only an end-to-end test could find, 9.5 What the boundary costs, 9.6 Moving the prior off the critical thread, 9.7 What Gate D still needs from the GPU host, 9. Gate D — the Python callback, on CPU

### Community 102 - "BatchItem"
Cohesion: 0.12
Nodes (16): BatchItem, actions, encoded, job, match, outcome, salt, state (+8 more)

### Community 103 - "AlphaDiamond Python-Zero Migration Implementation Plan / Approval Checklist"
Cohesion: 0.09
Nodes (32): AlphaDiamond Python-Zero Migration Implementation Plan, Approval Checklist, Execution Handoff, Global Constraints, Planned File Structure, Task 10: Native Arena, Ratings, Reports, and CLI Cutover (PR 09), Task 11: Native Checkpoint/Release Commands and Data Boundary (PR 10), Task 12: Native Benchmark Disposition and Final Smokes (PR 11) (+24 more)

### Community 104 - "VacancyWithMinValueArm / SooPolicyArm"
Cohesion: 0.14
Nodes (13): SooModel, SooPolicyArm, device_, folded_, model_, VacancyWithMinValueArm, device_, match_ (+5 more)

### Community 105 - "config.cpp"
Cohesion: 0.12
Nodes (17): ConfigError, ArenaConfig::validate(), bootstrap_prior_is_valid(), InferenceConfig::validate(), is_device_name(), MCTSConfig::validate(), NetworkConfig::validate(), non_blank() (+9 more)

### Community 106 - "RTX 5060 Ti Parent-Bottleneck Investigation / Inference call / data flow"
Cohesion: 0.15
Nodes (19): 1. Inference call / data flow, 2. Parent-side profile, 3. `max_wait_ms` sweep, 3. maxwaitms sweep, 4. Diagnosis of the dominant serialized cost, 5. The change that shipped, 6. Before / after benchmark, 7. Was parent saturation eliminated, or moved? (+11 more)

### Community 107 - "steps / with"
Cohesion: 0.24
Nodes (15): UBSAN_OPTIONS, steps, steps, steps, steps, env, if, name (+7 more)

### Community 108 - "selfplay.cpp"
Cohesion: 0.12
Nodes (12): MinTurnOrderGroup, cyclic, reverse, Batcher::collect(), Clock, configure_topology(), main(), percentile() (+4 more)

### Community 109 - "Episode / ModelKey"
Cohesion: 0.17
Nodes (11): Episode, aborted_reason, compatibility, completed, final_order, game_id, model_key, move_count (+3 more)

### Community 110 - "checkpoint.cpp"
Cohesion: 0.22
Nodes (27): CheckpointError, bool_field(), canonical_object_json(), compatibility_json(), exact_fields(), file_digest(), is_digest(), Json (+19 more)

### Community 111 - "generate_fixtures.cpp"
Cohesion: 0.18
Nodes (4): main(), print_info(), run(), unsupported_morph()

### Community 112 - "SchedulerConfig / scheduler_config()"
Cohesion: 0.11
Nodes (18): SchedulerConfig, dirichlet_alpha, dirichlet_epsilon, eval_latency_ms, games, max_batch, max_moves, max_wait_us (+10 more)

### Community 113 - "ReplayConfig"
Cohesion: 0.29
Nodes (4): ReplayConfig, capacity, seed, ReplayConfig::from_json()

### Community 114 - "pipeline.cpp"
Cohesion: 0.21
Nodes (13): PipelineError, ingest_self_play(), load_episode_artifact(), read_sample(), read_string(), read_u64(), read_value(), sample_from_move() (+5 more)

### Community 115 - "JsonParser / parse_value()"
Cohesion: 0.26
Nodes (5): JsonParser, position_, text_, parse_json(), sha256()

### Community 116 - "rating_sync_main.cpp / fail()"
Cohesion: 0.26
Nodes (14): EventSet, events, ids, fail(), is_event_file_name(), Json, load_events(), main() (+6 more)

### Community 117 - "실행 단위와 순서"
Cohesion: 0.08
Nodes (22): Engine/Training–Desktop 저장소 분리 분석, Rating 기능을 잃지 않는 분리, Runtime SDK 범위, SDK 버전과 배포, 권고와 대안, 독립 개발의 완료 조건, 모델 계약은 재사용, 정리해야 할 경계 (+14 more)

### Community 118 - "Min A0 transition status — 2026-09-02 / Observability and legal-policy learning"
Cohesion: 0.14
Nodes (17): 1. Observability and legal-policy learning, 2. Serial-search sweep, 3. Prior annealing, 4. Autopilot run and storage failure, 5. Latest transition probe, Current durable state and artifacts, Executive summary, Experiment sequence (+9 more)

### Community 119 - "Options / arm"
Cohesion: 0.09
Nodes (21): count(), main(), Options, arm, batch_size, checkpoint, checkpoint_out, config (+13 more)

### Community 120 - "diagnose_roots() / parse_options()"
Cohesion: 0.14
Nodes (12): accumulate(), diagnose_roots(), dominant_cycle_period(), main(), make_match(), opening(), parse_count(), parse_number() (+4 more)

### Community 121 - "Independent repository and SDK upgrade policy"
Cohesion: 0.24
Nodes (10): AlphaDiamondDesktop external repository, Explicit pinned source SDK fallback, Independent repository and SDK upgrade policy, Independent UI and trainer releases, Repository extraction provenance, Separate SDK and model rollback, Stable Engine model publishing identities, Desktop configuration matched installed SDK (+2 more)

### Community 122 - "PromotionStatisticsConfig"
Cohesion: 0.20
Nodes (6): PromotionStatisticsConfig, bootstrap_replicates, confidence_level, method, resampling_unit, seed

### Community 123 - "ResolvedDevice"
Cohesion: 0.10
Nodes (19): CUDA Device Runtime and Checkpoint Portability Implementation Plan, Task 1: Add the shared device resolver, Task 2: Establish deep model snapshots and weight identities, Task 3: Make ModelPool inference device-safe and batched, Task 4: Move learner, targets, loss, gradients, and AdamW to the device, Task 5: Make checkpoint archives device-portable, Task 6: CUDA-runtime boundary verification, DeviceRequest (+11 more)

### Community 124 - "Trainer / TrainingConfig"
Cohesion: 0.15
Nodes (10): PolicyLossDomain, full, legal, Trainer, compatibility_, config_, TrainingConfig, learning_rate (+2 more)

### Community 125 - "SearchResult / EvalRecord"
Cohesion: 0.12
Nodes (17): EvalRecord, legal_actions, request_hash, SearchResult, evaluator_calls, neural_evaluation_ms, nodes_created, policy (+9 more)

### Community 126 - "RTX 5060 Ti Worker-Scaling Findings / run_seed"
Cohesion: 0.10
Nodes (22): Also unresolved, Best worker count, Coordinator diagnosis, CPU diagnosis: **parent/GIL limited** (Case B), Environment, GPU diagnosis: **severely underfed**, Headline, `max_wait_ms` is unresolved, and the RTX 3060 answer may not transfer (+14 more)

### Community 127 - "Tensor"
Cohesion: 0.28
Nodes (6): apply_gelu(), DiamondModelImpl::DiamondModelImpl(), DiamondModelImpl::forward(), DiamondModelImpl::set_adjacency(), DirectionalResidualBlockImpl::forward(), topology_adjacency()

### Community 128 - "coordinator.cpp / CoordinatorError"
Cohesion: 0.17
Nodes (13): CoordinatorError, StageOutcome, progress, result, additional_iteration_limit(), Coordinator::durable_result(), Coordinator::operation_id(), Coordinator::run_bounded() (+5 more)

### Community 129 - "VisitTargetObservation / summarize_visit_targets()"
Cohesion: 0.15
Nodes (16): Task 3: Surface target statistics in self-play results and the durable sidecar, VisitTargetObservation, effective_actions, entropy, legal_actions, max_probability, normalized_entropy, top3_mass (+8 more)

### Community 130 - "OwnerEvaluator"
Cohesion: 0.16
Nodes (13): EpisodeJob, initial_state, match, seed, check_episode_owners(), check_router_validates_owner(), check_scheduler_owners(), check_search_uses_owners_model() (+5 more)

### Community 131 - "Golden / GoldenCase"
Cohesion: 0.17
Nodes (12): GoldenCase, action_count, canonical_fnv, encoded_fnv, physical_fnv, player_count, prior_count, prior_dot (+4 more)

### Community 132 - "checkpoint_benchmark.cpp"
Cohesion: 0.31
Nodes (6): Args, repetitions, scratch, main(), parse_args(), run()

### Community 133 - "training-v1/manifest.json"
Cohesion: 0.33
Nodes (5): device, dtype, families, fixture_version, game_contract

### Community 134 - "Evaluator / MCTS2P"
Cohesion: 0.12
Nodes (11): 9.3 End-to-end: the native backend plays the same game, What one Python callback per node costs, What this does and does not license, SooEvaluator, model_, DeterministicEvaluator, Evaluator, UniformPriorEvaluator (+3 more)

### Community 135 - "RTX 3060 GPU Training Runbook / inference"
Cohesion: 0.13
Nodes (16): 1. Provision, 2. Install a CUDA torch build, 3. Confirm the CPU allocation policy on the real box, 4. Get the run onto the box, 5. Prove CUDA correctness before believing any GPU result, 6. Benchmark from copies, before migrating, 7. Back up, then migrate the real run, 8. Train (+8 more)

### Community 136 - "Batcher"
Cohesion: 0.11
Nodes (14): Two attempted optimizations, both reverted, Finding 1 — the coordinator's batch window is bistable, Fix, Mechanism, The failing mechanism, Batcher, arrival_, max_batch_ (+6 more)

### Community 137 - "Min Turn-Order Experts Implementation Plan"
Cohesion: 0.33
Nodes (5): Execution ledger, Global constraints, Min Turn-Order Experts Implementation Plan, Review focus, Tasks

### Community 138 - "RunResult / aborted_games"
Cohesion: 0.13
Nodes (14): RunResult, aborted_games, completed_games, completed_training_steps, final_checkpoint_model_digest, final_step, first_checkpoint_model_digest, new_samples (+6 more)

### Community 139 - "VacancyMetrics / VacancyDistillationResult"
Cohesion: 0.13
Nodes (14): metrics(), VacancyDistillationResult, final, initial, policy_kl, trainable_gradient_l2, trainable_update_l2, VacancyMetrics (+6 more)

### Community 140 - "ReplayStore"
Cohesion: 0.09
Nodes (16): Args, batch_size, pool_size, reopen_only, repetitions, scratch, episode(), main() (+8 more)

### Community 141 - "CheckpointProvenance"
Cohesion: 0.22
Nodes (9): CheckpointProvenance, creation_timestamp, environment_json, protocol_ids_json, replay_manifest_sha256, resolved_config_bytes, rng_state_status, rng_state_version (+1 more)

### Community 142 - "Min 10-hour autopilot log — 2026-08-31 / A0 transition gate after release"
Cohesion: 0.13
Nodes (14): A0 transition gate after release, Active benchmark sweep, Additional disk cleanup (about 18:20 UTC), Config transition and real training resume, End-of-session learning assessment, Implemented before benchmark sweep, Iteration 24 completion and FP16 safety rollback, Iteration 25 incremental Arena (+6 more)

### Community 143 - "AlphaDiamond Python-Zero Inventory / Verified baseline"
Cohesion: 0.17
Nodes (14): 1. Verified baseline, 2. CI and branch-protection drift, 3.1 Product subsystems, 3.2 Dependency graph, 3. Tracked Python inventory, 4. Additional repository drift, 5. Native targets and current CTest coverage, 6. Pytest inventory and migration matrix (+6 more)

### Community 144 - "MinRatedMatch"
Cohesion: 0.12
Nodes (12): MinRatedMatch, match_id, opening_id, participant_ids, seat_assignment, turn_order, SooRatedMatch, match_id (+4 more)

### Community 145 - "DeploymentArtifact / validate_soo_deployment_artifact()"
Cohesion: 0.15
Nodes (13): DeploymentArtifact, encoder_version, input_features, model_family, model_sha256, model_version, residual_blocks, root (+5 more)

### Community 146 - "install() / activate()"
Cohesion: 0.38
Nodes (10): check_distribution(), expect_rejected_transactionally(), main(), min_compatibility(), soo_compatibility(), test_cuda_batches(), test_cuda_precision_parity(), test_ragged_min() (+2 more)

### Community 147 - "Phase"
Cohesion: 0.33
Nodes (6): Phase, AwaitLeaf, AwaitRoot, Descend, Done, Root

### Community 148 - "EpisodeConfig"
Cohesion: 0.10
Nodes (19): EpisodeConfig, bootstrap_prior, bootstrap_prior_weight, dirichlet_alpha, dirichlet_epsilon, encoding, lanes, late_move_threshold (+11 more)

### Community 149 - "EpisodeMetrics"
Cohesion: 0.20
Nodes (10): EpisodeMetrics, batch_sizes, batches, boosted_moves, evaluations, evaluator_seconds, moves, repetition_moves (+2 more)

### Community 150 - "SchedulerMetrics / waiting"
Cohesion: 0.14
Nodes (14): SchedulerMetrics, batch_sizes, batcher_wakeups, batches, evaluations, evaluator_seconds, games_finished, lane_moves (+6 more)

### Community 151 - "main"
Cohesion: 0.39
Nodes (4): main(), read_f32(), read_text(), require_metadata()

### Community 152 - "topology_gen.cpp / Cube"
Cohesion: 0.33
Nodes (11): camp_index(), camps_of(), canonical_cube(), Cube, x, y, z, generate_cubes() (+3 more)

### Community 153 - "Decisions"
Cohesion: 0.17
Nodes (12): alphadiamond, Build and test, Commands, Layout, Rules that are expensive to rediscover, Shipping a model, Training data, 1. Training game execution requires the native extension (+4 more)

### Community 154 - "3. What is left"
Cohesion: 0.22
Nodes (9): 3.0.1 The A0 investigation — read this before changing anything, 3.0.2 Architecture scaling — tried both axes, both rejected, 3.0.4 Rolling a replay store back, 3.0 Done on the GPU host (progress doc §10), 3.1.0 Configuring the native backend, 3.1.1 Worth doing when PolicyValue matters, 3.1 The actual remaining work, in order, 3.2 Known optimisation target, deliberately untouched (+1 more)

### Community 155 - "RTX 3060 Benchmark Findings / Baseline: CPU"
Cohesion: 0.12
Nodes (20): Baseline: CPU, Does concurrency convert idle GPU into throughput?, Environment, GPU runs, Headline, Is 64 simulations cheaper in wall-clock than 32?, `max_moves: 200` may be silently discarding training data, maxmoves: 200 may be silently discarding training data (+12 more)

### Community 156 - "Min Vacancy-Prior Realizability Implementation Plan / Min Vacancy Realizability Diagnostic Handoff — 2026-09-02"
Cohesion: 0.31
Nodes (9): Global Constraints, Min Vacancy-Prior Realizability Implementation Plan, Task 1: Reproducible operating-point config, Task 2: Vacancy teacher reconstruction, Task 3: Freeze-aware distillation and held-out metrics, Task 4: Offline diagnostic CLI and provenance gates, Task 5: Format, regression verification, and experiment handoff, value_linear1 (+1 more)

### Community 157 - "SelfPlayConfig / operator==()"
Cohesion: 0.14
Nodes (11): Task 2: `max_game_seconds` configuration surface, SelfPlayConfig, balance_turn_orders, bootstrap_prior, bootstrap_prior_weight, max_game_seconds, max_moves, seed (+3 more)

### Community 158 - "release_main.cpp"
Cohesion: 0.42
Nodes (7): initialize(), Json, main(), print_json(), promote(), run(), stage()

### Community 159 - "EvaluationStats / operator==()"
Cohesion: 0.15
Nodes (12): EvaluationStats, batch_size, collation_seconds, d2h_seconds, d2h_transfers, forward_calls, forward_seconds, h2d_seconds (+4 more)

### Community 160 - "device_test.cpp"
Cohesion: 0.39
Nodes (4): main(), quote(), rejects(), write_cuda_config()

### Community 161 - "Case / calls"
Cohesion: 0.15
Nodes (13): Case, calls, evaluator, policy_fnv, q_fnv, root_fnv, selected, simulations (+5 more)

### Community 162 - "Native products, one core"
Cohesion: 0.33
Nodes (5): CPU training, GUI and model connection, Python-zero final acceptance, Release build, Windows package

### Community 163 - "LoadedCheckpoint"
Cohesion: 0.50
Nodes (3): LoadedCheckpoint, identity, pool

### Community 164 - "on"
Cohesion: 0.11
Nodes (18): concurrency, cancel-in-progress, group, BASE_SHA, CLANG_FORMAT_MAJOR, jobs, native-format, name (+10 more)

### Community 165 - "conda_prefix / inputs"
Cohesion: 0.17
Nodes (12): default, description, required, type, conda_prefix, warm_start_root, on, workflow_dispatch (+4 more)

### Community 166 - "cuda-acceptance / env"
Cohesion: 0.17
Nodes (12): defaults, env, name, runs-on, timeout-minutes, run, ACCEPTANCE_CONFIG, CONDA_PREFIX_INPUT (+4 more)

### Community 167 - "Topology / camp_positions"
Cohesion: 0.17
Nodes (7): Topology, camp_positions, canonical_to_physical, configured, neighbour, pairwise, physical_to_canonical

### Community 168 - "MCTSConfig / from_json()"
Cohesion: 0.17
Nodes (8): MCTSConfig, c_puct, dirichlet_alpha, dirichlet_epsilon, repeat_window, seed, simulations, simulations_late

### Community 169 - "RunStateStore"
Cohesion: 0.10
Nodes (13): Coordinator, describe_stage_, execute_stage_, StageCallback, StagePayload, RunStateStore, root_, Coordinator::Coordinator() (+5 more)

### Community 170 - "The repetition trigger is implemented but unreachable"
Cohesion: 0.13
Nodes (13): An observability gap this exposed, Conclusion first, Correction to an earlier draft of this document, The repetition trigger is implemented but unreachable, What changed here, What is still open, What the first measurement showed, and why it is void, Why it went unnoticed (+5 more)

### Community 171 - "Lane"
Cohesion: 0.25
Nodes (8): Lane, game_seed, move_count, outcome, salt, session, state, submitted_at

### Community 172 - "BatchFixture"
Cohesion: 0.25
Nodes (5): BatchFixture, actions, encoded, items, outcomes

### Community 173 - "min_value_head_init_test.cpp / samples()"
Cohesion: 0.30
Nodes (7): main(), max_abs(), max_abs_difference(), min_compatibility(), min_model(), samples(), tensors_identical()

### Community 174 - "Min A0 전환 진단과 다음 실험"
Cohesion: 0.16
Nodes (16): Arm A: policy head만 학습, Arm B: trunk와 policy 전체 학습, Min A0 전환 진단과 다음 실험, teacher fit은 되지만 A0가 계속 실패하면, teacher fit이 성공하면, value 쪽도 한 번은 확인해야 합니다, 근거 1: simulations 부족이 아닙니다, 근거 2: MCTS teacher target 자체가 지나치게 평평합니다 (+8 more)

### Community 175 - "Min: six-order datacenter experiment / run_min_balanced_datacenter.sh"
Cohesion: 0.14
Nodes (13): Balanced self-play turn orders, Historical distinction, Local smoke run, Observed local result — 2026-09-11, Min: six-order datacenter experiment, Run the comparison, Start on a Linux GPU host, control (+5 more)

### Community 176 - "TrainingConfig / train_steps_per_iteration"
Cohesion: 0.18
Nodes (7): TrainingConfig, batch_size, learning_rate, policy_loss_domain, seed, train_steps_per_iteration, weight_decay

### Community 177 - "CLAUDE.md"
Cohesion: 0.22
Nodes (3): Independent releases, Models and rollback, Repository split and SDK upgrades

### Community 178 - "PolicyTotals"
Cohesion: 0.22
Nodes (9): PolicyTotals, full_cross_entropy, full_kl, legal_cross_entropy, legal_kl, legal_mass, rows, target_entropy (+1 more)

### Community 179 - "OpeningBlockBootstrap / bootstrap_opening_blocks()"
Cohesion: 0.25
Nodes (7): OpeningBlockBootstrap, complete_blocks, confidence_lower, confidence_upper, incomplete_blocks, point_estimate, promoted

### Community 180 - "Repository split dependency analysis"
Cohesion: 0.29
Nodes (3): Proposed runtime SDK surface, Repository split dependency analysis, SDK version ABI and packaging contract

### Community 181 - "Deployment artifact format v3 current contract"
Cohesion: 0.29
Nodes (7): Deployment artifact format v3 current contract, Model and runtime digest coverage, Native checkpoint export integrity, Runtime bundle and checkpoint distinction, Soo Min legacy and goals encoder layouts, Strict declared architecture and game contract, Coordinated SDK lock upgrade

### Community 182 - "IterationSource / checkpoint_lineage()"
Cohesion: 0.17
Nodes (10): checkpoint_lineage(), IterationSource, mode, model_digest, optimizer_reset, optimizer_reset_reason, optimizer_restored, source_training_step (+2 more)

### Community 183 - "Binary Replay Segments Design"
Cohesion: 0.29
Nodes (6): Acceptance, Binary Replay Segments Design, Chosen design, Goal, Rollout, Transactions and identity

### Community 184 - "Case / calls"
Cohesion: 0.15
Nodes (12): Case, calls, policy_fnv, q_fnv, root_fnv, selected, simulations, simulations_run (+4 more)

### Community 185 - "Native Windows Qt runtime / Build and package"
Cohesion: 0.20
Nodes (10): Build and package, Compatibility notes, Gate record, Native Windows Qt runtime, Platform boundary, QML backend compatibility manifest, Runtime architecture, Search configuration and measured latency (+2 more)

### Community 186 - "steps / with"
Cohesion: 0.20
Nodes (10): steps, if, name, run, uses, with, fetch-depth, if-no-files-found (+2 more)

### Community 187 - "Args / parse_args()"
Cohesion: 0.22
Nodes (7): Args, artifact, batch_size, device, repetitions, threads, warmups

### Community 188 - "OpeningSuiteConfig / from_json()"
Cohesion: 0.20
Nodes (6): OpeningSuiteConfig, count, id, max_depth, seed, version

### Community 189 - "MCTSConfig"
Cohesion: 0.29
Nodes (7): MCTSConfig, c_puct, dirichlet_alpha, dirichlet_epsilon, encoding, seed, simulations

### Community 190 - "RatingEventOutbox / publish()"
Cohesion: 0.24
Nodes (5): RatingEventOutbox, root_, set_rating_store_failpoint_for_testing(), main(), require()

### Community 191 - "ModelIndexEntry"
Cohesion: 0.15
Nodes (10): ModelIndex, defaults_, models, ModelIndexEntry, family, model_sha256, root, runtime_sha256 (+2 more)

### Community 192 - "export_checkpoint"
Cohesion: 0.48
Nodes (5): export_checkpoint(), parity(), read(), write(), write_i32()

### Community 193 - "ReplayIngestReport / ReplaySamplingStats"
Cohesion: 0.20
Nodes (9): ReplayIngestReport, accepted_games, accepted_samples, duplicate_games, duplicate_samples, ReplaySamplingStats, copied_samples, selection_slots (+1 more)

### Community 194 - "ParameterGroup / parameter_group_name()"
Cohesion: 0.20
Nodes (10): ParameterGroup, input_projection, last_residual_block, output_norm, policy_destination, policy_source, residual_trunk, value_hidden (+2 more)

### Community 195 - "CampDiagnostics / blocker_cells"
Cohesion: 0.20
Nodes (10): CampDiagnostics, blocker_cells, blocker_legal_moves, blocker_owners, empty_in_target, foreign_in_target, own_in_target, player_id (+2 more)

### Community 196 - "Min Vacancy Realizability Diagnostic Handoff — 2026-09-02"
Cohesion: 0.25
Nodes (7): Authoritative execution commands, Implemented contracts, Min Vacancy Realizability Diagnostic Handoff — 2026-09-02, Non-authoritative iteration-25 smoke, Status, Verification, sparse_policy

### Community 197 - "Encoded"
Cohesion: 0.27
Nodes (12): Encoded, canonical_player_ids, feature_count, node_features, DeterministicEvaluator::evaluate(), mix(), mix_u32(), mix_u64() (+4 more)

### Community 198 - "family_case"
Cohesion: 0.25
Nodes (6): family_case(), features(), initial(), main(), rejects(), sample()

### Community 199 - "Retiring the Python engine"
Cohesion: 0.33
Nodes (5): Retiring the Python engine, The evidence collected before deleting, What cannot be done any more, on purpose, What was deleted, and what replaced it, What was never going to be ported

### Community 200 - "files / min/after_step_parameters/trunk__output_norm__weight.f32"
Cohesion: 0.20
Nodes (10): files, min/after_step_parameters/trunk__output_norm__weight.f32, min/optimizer/trunk__blocks__0__direction_projections__3__weight__step.i64, byte_count, element_type, sha256, shape, byte_count (+2 more)

### Community 201 - "Arena, Acceptance, CI, and Repository Tightening Implementation Plan / Task 1: Materialize versioned opening suites"
Cohesion: 0.22
Nodes (8): Arena, Acceptance, CI, and Repository Tightening Implementation Plan, Task 1: Materialize versioned opening suites, Task 3: Bootstrap statistics by opening block and harden promotion, Task 4: Complete canonical metrics and native benchmarks, Task 5: Add honest CPU and NVIDIA CI lanes, Task 6: Pin changed-file formatting and update current documentation, Task 7: CUDA-host acceptance checklist, Task 8: Final repository boundary verification

### Community 202 - "load_checkpoint"
Cohesion: 0.21
Nodes (13): Expected CLI Examples, compatibility(), compatibility_json(), Config, integer(), load_checkpoint(), main(), Path (+5 more)

### Community 203 - "native-sanitizers / env"
Cohesion: 0.14
Nodes (16): run, CC, CXX, LIBTORCH_VERSION, native-sanitizers, native-training, defaults, defaults (+8 more)

### Community 204 - "model_index_test.cpp"
Cohesion: 0.60
Nodes (4): main(), rejects(), with(), write_index()

### Community 205 - "diamond_rating"
Cohesion: 0.40
Nodes (3): alphadiamond-rating-sync, diamond_orchestration, diamond_rating

### Community 206 - "InferenceConfig / max_batch_size"
Cohesion: 0.22
Nodes (5): InferenceConfig, max_batch_size, max_wait_us, request_queue_capacity, response_timeout_s

### Community 207 - "WorkerConfig / from_json()"
Cohesion: 0.22
Nodes (5): WorkerConfig, games_per_iteration, logical_lanes, retry_id, search_threads

### Community 208 - "MinRating / MinLeaderboardEntry"
Cohesion: 0.20
Nodes (10): MinLeaderboardEntry, display_name, participant_id, rating, MinRating, exposure, mu, rated_games (+2 more)

### Community 209 - "VacancyDistillationConfig / DistillationArm"
Cohesion: 0.22
Nodes (8): DistillationArm, policy_head, trunk_policy, VacancyDistillationConfig, arm, batch_size, evaluation_batch, steps

### Community 210 - "EpisodeDiagnostics / camps"
Cohesion: 0.13
Nodes (15): AbortedGameDiagnostics, abort_reason, game_id, move_count, seed, state, EpisodeDiagnostics, camps (+7 more)

### Community 211 - "copy_parameter"
Cohesion: 0.52
Nodes (4): copy_parameter(), DiamondModelImpl::load_weights(), read_tensor(), weight()

### Community 212 - "vacancy_distillation_test.cpp / snapshot()"
Cohesion: 0.39
Nodes (5): main(), sample(), snapshot(), throws_invalid(), trainer()

### Community 213 - "payload_sha256 / mcts-v1.txt"
Cohesion: 0.13
Nodes (14): corpus_sha256, game_contract_version, golden_format_version, note, oracle_commit, payload_sha256, mcts3p-v1.txt, mcts-v1.txt (+6 more)

### Community 214 - "VacancyTarget"
Cohesion: 0.40
Nodes (4): VacancyTarget, actions, probabilities, progress

### Community 215 - "Durable Iteration Loop and Replay Transactions Implementation Plan / Task 1: Replace full-pool sampling with bounded selection"
Cohesion: 0.22
Nodes (8): Durable Iteration Loop and Replay Transactions Implementation Plan, Task 1: Replace full-pool sampling with bounded selection, Task 2: Add replay sampling and ingest transactions, Task 3: Introduce checkpoint manifest v3 and initialization lineage, Task 4: Give every stage a durable idempotent operation result, Task 5: Split the monolithic iteration into real stage executors, Task 6: Implement bounded multi-iteration train/resume semantics, Task 7: Durability boundary verification

### Community 216 - "main"
Cohesion: 0.31
Nodes (7): count(), Json, main(), metrics(), parse(), read_config(), replay_root()

### Community 217 - "cuda-acceptance.yml / concurrency"
Cohesion: 0.25
Nodes (7): concurrency, cancel-in-progress, group, jobs, name, permissions, contents

### Community 218 - "HeldOutMetrics"
Cohesion: 0.14
Nodes (11): HeldOutMetrics, full_cross_entropy, full_kl, target_entropy, top1_agreement, value_mse, evaluate(), Evaluation (+3 more)

### Community 219 - "RunBudgetConfig"
Cohesion: 0.12
Nodes (9): Task 2: Implement exact schema v2 and migrate tracked configs, RunBudgetConfig, checkpoint_every_iterations, max_iterations, max_wall_clock_seconds, RuntimeConfig, device, precision (+1 more)

### Community 220 - "LearningDiagnosticConfig / batch_size"
Cohesion: 0.14
Nodes (14): Phase 1: Add Observability Without Changing Behavior, Task 2: Implement pure MCTS visit-target statistics, Task 4: Add legal-set policy-fit diagnostics to `selfplay_benchmark`, Task 5: Add an in-memory learner diagnostic executable, Task 6: Verify PR 1 and document exactly what it does not change, LearningDiagnosticConfig, batch_size, evaluation_batch (+6 more)

### Community 221 - "PolicyRowDiagnostics / full_cross_entropy"
Cohesion: 0.25
Nodes (8): PolicyRowDiagnostics, full_cross_entropy, full_kl, legal_cross_entropy, legal_kl, legal_probability_mass, target_entropy, top1_agrees

### Community 222 - "parameter_diagnostics_test.cpp"
Cohesion: 0.50
Nodes (3): classify_parameter(), main(), throws_invalid_argument()

### Community 223 - "IterationResult"
Cohesion: 0.10
Nodes (19): Global constraints, Native Training Contract and Batching Implementation Plan, Task 1: Freeze config-v2 failure contracts, Task 3: Make pipeline batch and step counts observable and honored, Task 4: Separate lanes, threads, and microsecond waits at the command boundary, Task 5: Implement monotonic per-game deadlines and zero-sample aborts, Task 6: Collate dense training targets once on the host, Task 7: PR-A acceptance and handoff (+11 more)

### Community 224 - "Native products, one core"
Cohesion: 0.50
Nodes (4): Build and validation, Frozen contracts, Historical evidence, Native products, one core

### Community 225 - "fstream"
Cohesion: 0.08
Nodes (14): checked_kl(), diagnose_policy_row(), main(), rejects(), write_file(), exempt(), main(), check_family() (+6 more)

### Community 226 - "rating_sync_test.cpp / write()"
Cohesion: 0.39
Nodes (5): Json, main(), require(), run(), write()

### Community 227 - "ModelKey"
Cohesion: 0.18
Nodes (9): IncompatibleCheckpointError, ModelKey, checkpoint_sha256, model_name, model_version, ModelPool::activate(), ModelPool::install(), ModelPool::install_checkpoint() (+1 more)

### Community 228 - "Engine development build workflow"
Cohesion: 0.50
Nodes (4): Debug GCC15 Ninja sccache mold workflow, Engine development build workflow, Explicit optimized Engine outputs, Resource aware parallel build and CTest

### Community 229 - "training_vector_manifest_test.cpp"
Cohesion: 0.22
Nodes (4): DirectionalResidualBlockImpl, direction_projections, norm, self_projection

### Community 230 - "native-core / strategy"
Cohesion: 0.29
Nodes (7): native-core, os, name, runs-on, strategy, fail-fast, matrix

### Community 231 - "Development build cycle"
Cohesion: 0.50
Nodes (4): Desktop and the SDK, Development build cycle, Native engine and inference, Optimized application and final training

### Community 232 - "The replay store is a durable dataset with a stateless sampler"
Cohesion: 0.33
Nodes (5): Consequence for tests, The manifest is contents identity only, The replay store is a durable dataset with a stateless sampler, What changed, Why this is safe for resume

### Community 233 - "NetworkConfig / from_json()"
Cohesion: 0.29
Nodes (3): NetworkConfig, residual_blocks, width

### Community 234 - "main"
Cohesion: 0.28
Nodes (8): correlation(), gamma_samples(), ks_statistic(), main(), Moments, mean, variance, threw_invalid_argument()

### Community 235 - "build_resources_test.sh"
Cohesion: 0.25
Nodes (8): DIAMOND_RESOURCE_CGROUP_ROOT, DIAMOND_RESOURCE_PROC_ROOT, expect(), OMP_NUM_THREADS, OMP_THREAD_LIMIT, PATH, build_resources_test.sh script, TEST_AFFINITY_CPUS

### Community 236 - "LearningDrift / full_cross_entropy_delta"
Cohesion: 0.15
Nodes (12): LearningDiagnosticResult, drift, final, initial, steps, LearningDrift, full_cross_entropy_delta, logit_rms_delta (+4 more)

### Community 237 - "runtime_sha256"
Cohesion: 0.29
Nodes (7): Deployment artifact format v3, Metadata, Min, Native checkpoint export, One representation in the package, Packaging, Training checkpoint is not a deployment artifact

### Community 238 - "Goal-aware training"
Cohesion: 0.50
Nodes (4): Goal-aware training, Starting from an existing model, Verification and limits, root_player

### Community 239 - "Rating v2 Hugging Face synchronization"
Cohesion: 0.50
Nodes (3): Bucket wrapper, Protocol input and output, Rating v2 Hugging Face synchronization

### Community 240 - "Min turn-order specialists / Abort sensitivity and interpretation"
Cohesion: 0.29
Nodes (7): Abort sensitivity and interpretation, Frozen-reference arena, Implementation verification, Min turn-order specialists, Pilot execution, Pilot results, Reboot handoff

### Community 241 - "native_training.sh / native_training.sh script"
Cohesion: 0.43
Nodes (5): Execution Notes, append_runtime_dir(), find_tool(), native_training.sh script, usage()

### Community 242 - "Min iteration-25 learning diagnostic — 2026-09-01 / Commands"
Cohesion: 0.29
Nodes (6): Commands, Held-out metrics, Inputs and immutability, Min iteration-25 learning diagnostic — 2026-09-01, Parameter-group summaries, Result

### Community 243 - "select_from_visits"
Cohesion: 0.50
Nodes (3): 3.0.3 Arena — what the old numbers were actually measuring, 9. Risks to preserving semantics, select_from_visits()

### Community 244 - "Min next intervention — measured decision / Authorized change"
Cohesion: 0.29
Nodes (6): Authorized change, Evidence and gate values, Expected improvement and rollback, Min next intervention — measured decision, Rejected branches, Selected branch

### Community 245 - "ResidentModel"
Cohesion: 0.50
Nodes (3): ResidentModel, actor, compatibility

### Community 246 - "string"
Cohesion: 0.13
Nodes (21): build_provenance_json(), append_json(), append_string(), canonical_json(), string(), load_topology_from_dir(), read_exact(), read_i32() (+13 more)

### Community 247 - "LearningStepDiagnostic / size_t"
Cohesion: 0.18
Nodes (9): LearningStepDiagnostic, groups, local_step, losses, GroupNorms, gradient_l2, parameter_l2, relative_update (+1 more)

### Community 248 - "CheckpointLoadIntent"
Cohesion: 0.50
Nodes (4): CheckpointLoadIntent, audited_legacy_import, exact_resume, warm_start

### Community 249 - "ReplayOpenMode"
Cohesion: 0.67
Nodes (3): ReplayOpenMode, create_if_missing, must_exist

### Community 250 - "Phase / AwaitLeaf"
Cohesion: 0.33
Nodes (6): Phase, AwaitLeaf, AwaitRoot, Descend, Done, Root

### Community 251 - "Resolved: the 768-game completion regression was a match-construction bug"
Cohesion: 0.24
Nodes (5): Consequences for other measurements, Prevention, Resolved: the 768-game completion regression was a match-construction bug, What survives from it, What was wrong in the earlier analysis

### Community 254 - "sync_ratings.sh / sync_ratings.sh script"
Cohesion: 0.67
Nodes (5): check_event_name(), list_remote(), remote_contains(), sync_ratings.sh script, usage()

### Community 255 - "Goal-aware training implementation plan / Execution ledger"
Cohesion: 0.33
Nodes (5): Execution ledger, Global constraints, Goal-aware training implementation plan, Review focus, Tasks and verification

### Community 257 - "SooLeaderboardEntry / display_name"
Cohesion: 0.33
Nodes (6): SooLeaderboardEntry, display_name, games, participant_id, rating, RatingRegistry::soo_leaderboard()

### Community 262 - "environment/source_commit.txt / byte_count"
Cohesion: 0.40
Nodes (5): byte_count, element_type, sha256, shape, environment/source_commit.txt

### Community 263 - "environment/torch_version.txt / byte_count"
Cohesion: 0.40
Nodes (5): byte_count, element_type, sha256, shape, environment/torch_version.txt

### Community 264 - "min/after_step_parameters/policy_head__destination__bias.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/after_step_parameters/policy_head__destination__bias.f32, byte_count, element_type, sha256, shape

### Community 265 - "min/after_step_parameters/policy_head__destination__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/after_step_parameters/policy_head__destination__weight.f32, byte_count, element_type, sha256, shape

### Community 266 - "min/after_step_parameters/policy_head__source__bias.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/after_step_parameters/policy_head__source__bias.f32, byte_count, element_type, sha256, shape

### Community 267 - "min/after_step_parameters/policy_head__source__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/after_step_parameters/policy_head__source__weight.f32, byte_count, element_type, sha256, shape

### Community 268 - "min/after_step_parameters/trunk__adjacency.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/after_step_parameters/trunk__adjacency.f32, byte_count, element_type, sha256, shape

### Community 269 - "min/after_step_parameters/trunk__blocks__0__direction_projections__0__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/after_step_parameters/trunk__blocks__0__direction_projections__0__weight.f32, byte_count, element_type, sha256, shape

### Community 270 - "min/after_step_parameters/trunk__blocks__0__direction_projections__1__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/after_step_parameters/trunk__blocks__0__direction_projections__1__weight.f32, byte_count, element_type, sha256, shape

### Community 271 - "min/after_step_parameters/trunk__blocks__0__direction_projections__2__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/after_step_parameters/trunk__blocks__0__direction_projections__2__weight.f32, byte_count, element_type, sha256, shape

### Community 272 - "min/after_step_parameters/trunk__blocks__0__direction_projections__3__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/after_step_parameters/trunk__blocks__0__direction_projections__3__weight.f32, byte_count, element_type, sha256, shape

### Community 273 - "min/after_step_parameters/trunk__blocks__0__direction_projections__4__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/after_step_parameters/trunk__blocks__0__direction_projections__4__weight.f32, byte_count, element_type, sha256, shape

### Community 274 - "min/after_step_parameters/trunk__blocks__0__direction_projections__5__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/after_step_parameters/trunk__blocks__0__direction_projections__5__weight.f32, byte_count, element_type, sha256, shape

### Community 275 - "min/after_step_parameters/trunk__blocks__0__norm__bias.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/after_step_parameters/trunk__blocks__0__norm__bias.f32, byte_count, element_type, sha256, shape

### Community 276 - "min/after_step_parameters/trunk__blocks__0__norm__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/after_step_parameters/trunk__blocks__0__norm__weight.f32, byte_count, element_type, sha256, shape

### Community 277 - "min/after_step_parameters/trunk__blocks__0__self_projection__bias.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/after_step_parameters/trunk__blocks__0__self_projection__bias.f32, byte_count, element_type, sha256, shape

### Community 278 - "min/after_step_parameters/trunk__blocks__0__self_projection__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/after_step_parameters/trunk__blocks__0__self_projection__weight.f32, byte_count, element_type, sha256, shape

### Community 279 - "min/after_step_parameters/trunk__input_projection__bias.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/after_step_parameters/trunk__input_projection__bias.f32, byte_count, element_type, sha256, shape

### Community 280 - "min/after_step_parameters/trunk__input_projection__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/after_step_parameters/trunk__input_projection__weight.f32, byte_count, element_type, sha256, shape

### Community 281 - "min/after_step_parameters/trunk__output_norm__bias.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/after_step_parameters/trunk__output_norm__bias.f32, byte_count, element_type, sha256, shape

### Community 282 - "min/after_step_parameters/value_head__0__bias.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/after_step_parameters/value_head__0__bias.f32, byte_count, element_type, sha256, shape

### Community 283 - "min/after_step_parameters/value_head__0__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/after_step_parameters/value_head__0__weight.f32, byte_count, element_type, sha256, shape

### Community 284 - "min/after_step_parameters/value_head__2__bias.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/after_step_parameters/value_head__2__bias.f32, byte_count, element_type, sha256, shape

### Community 285 - "min/after_step_parameters/value_head__2__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/after_step_parameters/value_head__2__weight.f32, byte_count, element_type, sha256, shape

### Community 286 - "min/checkpoint-v1.pt / byte_count"
Cohesion: 0.40
Nodes (5): min/checkpoint-v1.pt, byte_count, element_type, sha256, shape

### Community 287 - "min/gradients/policy_head__destination__bias.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/gradients/policy_head__destination__bias.f32, byte_count, element_type, sha256, shape

### Community 288 - "min/gradients/policy_head__destination__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/gradients/policy_head__destination__weight.f32, byte_count, element_type, sha256, shape

### Community 289 - "min/gradients/policy_head__source__bias.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/gradients/policy_head__source__bias.f32, byte_count, element_type, sha256, shape

### Community 290 - "min/gradients/policy_head__source__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/gradients/policy_head__source__weight.f32, byte_count, element_type, sha256, shape

### Community 291 - "min/gradients/trunk__blocks__0__direction_projections__0__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/gradients/trunk__blocks__0__direction_projections__0__weight.f32, byte_count, element_type, sha256, shape

### Community 292 - "min/gradients/trunk__blocks__0__direction_projections__1__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/gradients/trunk__blocks__0__direction_projections__1__weight.f32, byte_count, element_type, sha256, shape

### Community 293 - "min/gradients/trunk__blocks__0__direction_projections__2__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/gradients/trunk__blocks__0__direction_projections__2__weight.f32, byte_count, element_type, sha256, shape

### Community 294 - "min/gradients/trunk__blocks__0__direction_projections__3__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/gradients/trunk__blocks__0__direction_projections__3__weight.f32, byte_count, element_type, sha256, shape

### Community 295 - "min/gradients/trunk__blocks__0__direction_projections__4__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/gradients/trunk__blocks__0__direction_projections__4__weight.f32, byte_count, element_type, sha256, shape

### Community 296 - "min/gradients/trunk__blocks__0__direction_projections__5__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/gradients/trunk__blocks__0__direction_projections__5__weight.f32, byte_count, element_type, sha256, shape

### Community 297 - "min/gradients/trunk__blocks__0__norm__bias.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/gradients/trunk__blocks__0__norm__bias.f32, byte_count, element_type, sha256, shape

### Community 298 - "min/gradients/trunk__blocks__0__norm__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/gradients/trunk__blocks__0__norm__weight.f32, byte_count, element_type, sha256, shape

### Community 299 - "min/gradients/trunk__blocks__0__self_projection__bias.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/gradients/trunk__blocks__0__self_projection__bias.f32, byte_count, element_type, sha256, shape

### Community 300 - "min/gradients/trunk__blocks__0__self_projection__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/gradients/trunk__blocks__0__self_projection__weight.f32, byte_count, element_type, sha256, shape

### Community 301 - "min/gradients/trunk__input_projection__bias.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/gradients/trunk__input_projection__bias.f32, byte_count, element_type, sha256, shape

### Community 302 - "min/gradients/trunk__input_projection__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/gradients/trunk__input_projection__weight.f32, byte_count, element_type, sha256, shape

### Community 303 - "min/gradients/trunk__output_norm__bias.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/gradients/trunk__output_norm__bias.f32, byte_count, element_type, sha256, shape

### Community 304 - "min/gradients/trunk__output_norm__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/gradients/trunk__output_norm__weight.f32, byte_count, element_type, sha256, shape

### Community 305 - "min/gradients/value_head__0__bias.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/gradients/value_head__0__bias.f32, byte_count, element_type, sha256, shape

### Community 306 - "min/gradients/value_head__0__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/gradients/value_head__0__weight.f32, byte_count, element_type, sha256, shape

### Community 307 - "min/gradients/value_head__2__bias.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/gradients/value_head__2__bias.f32, byte_count, element_type, sha256, shape

### Community 308 - "min/gradients/value_head__2__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/gradients/value_head__2__weight.f32, byte_count, element_type, sha256, shape

### Community 309 - "min/initial_logits.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/initial_logits.f32, byte_count, element_type, sha256, shape

### Community 310 - "min/initial_parameters/policy_head__destination__bias.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/initial_parameters/policy_head__destination__bias.f32, byte_count, element_type, sha256, shape

### Community 311 - "min/initial_parameters/policy_head__destination__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/initial_parameters/policy_head__destination__weight.f32, byte_count, element_type, sha256, shape

### Community 312 - "min/initial_parameters/policy_head__source__bias.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/initial_parameters/policy_head__source__bias.f32, byte_count, element_type, sha256, shape

### Community 313 - "min/initial_parameters/policy_head__source__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/initial_parameters/policy_head__source__weight.f32, byte_count, element_type, sha256, shape

### Community 314 - "min/initial_parameters/trunk__adjacency.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/initial_parameters/trunk__adjacency.f32, byte_count, element_type, sha256, shape

### Community 315 - "min/initial_parameters/trunk__blocks__0__direction_projections__0__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/initial_parameters/trunk__blocks__0__direction_projections__0__weight.f32, byte_count, element_type, sha256, shape

### Community 316 - "min/initial_parameters/trunk__blocks__0__direction_projections__1__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/initial_parameters/trunk__blocks__0__direction_projections__1__weight.f32, byte_count, element_type, sha256, shape

### Community 317 - "min/initial_parameters/trunk__blocks__0__direction_projections__2__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/initial_parameters/trunk__blocks__0__direction_projections__2__weight.f32, byte_count, element_type, sha256, shape

### Community 318 - "min/initial_parameters/trunk__blocks__0__direction_projections__3__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/initial_parameters/trunk__blocks__0__direction_projections__3__weight.f32, byte_count, element_type, sha256, shape

### Community 319 - "min/initial_parameters/trunk__blocks__0__direction_projections__4__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/initial_parameters/trunk__blocks__0__direction_projections__4__weight.f32, byte_count, element_type, sha256, shape

### Community 320 - "min/initial_parameters/trunk__blocks__0__direction_projections__5__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/initial_parameters/trunk__blocks__0__direction_projections__5__weight.f32, byte_count, element_type, sha256, shape

### Community 321 - "min/initial_parameters/trunk__blocks__0__norm__bias.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/initial_parameters/trunk__blocks__0__norm__bias.f32, byte_count, element_type, sha256, shape

### Community 322 - "min/initial_parameters/trunk__blocks__0__norm__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/initial_parameters/trunk__blocks__0__norm__weight.f32, byte_count, element_type, sha256, shape

### Community 323 - "min/initial_parameters/trunk__blocks__0__self_projection__bias.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/initial_parameters/trunk__blocks__0__self_projection__bias.f32, byte_count, element_type, sha256, shape

### Community 324 - "min/initial_parameters/trunk__blocks__0__self_projection__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/initial_parameters/trunk__blocks__0__self_projection__weight.f32, byte_count, element_type, sha256, shape

### Community 325 - "min/initial_parameters/trunk__input_projection__bias.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/initial_parameters/trunk__input_projection__bias.f32, byte_count, element_type, sha256, shape

### Community 326 - "min/initial_parameters/trunk__input_projection__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/initial_parameters/trunk__input_projection__weight.f32, byte_count, element_type, sha256, shape

### Community 327 - "min/initial_parameters/trunk__output_norm__bias.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/initial_parameters/trunk__output_norm__bias.f32, byte_count, element_type, sha256, shape

### Community 328 - "min/initial_parameters/trunk__output_norm__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/initial_parameters/trunk__output_norm__weight.f32, byte_count, element_type, sha256, shape

### Community 329 - "min/initial_parameters/value_head__0__bias.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/initial_parameters/value_head__0__bias.f32, byte_count, element_type, sha256, shape

### Community 330 - "min/initial_parameters/value_head__0__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/initial_parameters/value_head__0__weight.f32, byte_count, element_type, sha256, shape

### Community 331 - "min/initial_parameters/value_head__2__bias.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/initial_parameters/value_head__2__bias.f32, byte_count, element_type, sha256, shape

### Community 332 - "min/initial_parameters/value_head__2__weight.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/initial_parameters/value_head__2__weight.f32, byte_count, element_type, sha256, shape

### Community 333 - "min/initial_values.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/initial_values.f32, byte_count, element_type, sha256, shape

### Community 334 - "min/inputs.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/inputs.f32, byte_count, element_type, sha256, shape

### Community 335 - "min/losses.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/losses.f32, byte_count, element_type, sha256, shape

### Community 336 - "min/optimizer/policy_head__destination__bias__exp_avg.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/optimizer/policy_head__destination__bias__exp_avg.f32, byte_count, element_type, sha256, shape

### Community 337 - "min/optimizer/policy_head__destination__bias__exp_avg_sq.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/optimizer/policy_head__destination__bias__exp_avg_sq.f32, byte_count, element_type, sha256, shape

### Community 338 - "min/optimizer/policy_head__destination__bias__step.i64 / byte_count"
Cohesion: 0.40
Nodes (5): min/optimizer/policy_head__destination__bias__step.i64, byte_count, element_type, sha256, shape

### Community 339 - "min/optimizer/policy_head__destination__weight__exp_avg.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/optimizer/policy_head__destination__weight__exp_avg.f32, byte_count, element_type, sha256, shape

### Community 340 - "min/optimizer/policy_head__destination__weight__exp_avg_sq.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/optimizer/policy_head__destination__weight__exp_avg_sq.f32, byte_count, element_type, sha256, shape

### Community 341 - "min/optimizer/policy_head__destination__weight__step.i64 / byte_count"
Cohesion: 0.40
Nodes (5): min/optimizer/policy_head__destination__weight__step.i64, byte_count, element_type, sha256, shape

### Community 342 - "min/optimizer/policy_head__source__bias__exp_avg.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/optimizer/policy_head__source__bias__exp_avg.f32, byte_count, element_type, sha256, shape

### Community 343 - "min/optimizer/policy_head__source__bias__exp_avg_sq.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/optimizer/policy_head__source__bias__exp_avg_sq.f32, byte_count, element_type, sha256, shape

### Community 344 - "min/optimizer/policy_head__source__bias__step.i64 / byte_count"
Cohesion: 0.40
Nodes (5): min/optimizer/policy_head__source__bias__step.i64, byte_count, element_type, sha256, shape

### Community 345 - "min/optimizer/policy_head__source__weight__exp_avg.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/optimizer/policy_head__source__weight__exp_avg.f32, byte_count, element_type, sha256, shape

### Community 346 - "min/optimizer/policy_head__source__weight__exp_avg_sq.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/optimizer/policy_head__source__weight__exp_avg_sq.f32, byte_count, element_type, sha256, shape

### Community 347 - "min/optimizer/policy_head__source__weight__step.i64 / byte_count"
Cohesion: 0.40
Nodes (5): min/optimizer/policy_head__source__weight__step.i64, byte_count, element_type, sha256, shape

### Community 348 - "min/optimizer/trunk__blocks__0__direction_projections__0__weight__exp_avg.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/optimizer/trunk__blocks__0__direction_projections__0__weight__exp_avg.f32, byte_count, element_type, sha256, shape

### Community 349 - "min/optimizer/trunk__blocks__0__direction_projections__0__weight__exp_avg_sq.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/optimizer/trunk__blocks__0__direction_projections__0__weight__exp_avg_sq.f32, byte_count, element_type, sha256, shape

### Community 350 - "min/optimizer/trunk__blocks__0__direction_projections__0__weight__step.i64 / byte_count"
Cohesion: 0.40
Nodes (5): min/optimizer/trunk__blocks__0__direction_projections__0__weight__step.i64, byte_count, element_type, sha256, shape

### Community 351 - "min/optimizer/trunk__blocks__0__direction_projections__1__weight__exp_avg.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/optimizer/trunk__blocks__0__direction_projections__1__weight__exp_avg.f32, byte_count, element_type, sha256, shape

### Community 352 - "min/optimizer/trunk__blocks__0__direction_projections__1__weight__exp_avg_sq.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/optimizer/trunk__blocks__0__direction_projections__1__weight__exp_avg_sq.f32, byte_count, element_type, sha256, shape

### Community 353 - "min/optimizer/trunk__blocks__0__direction_projections__1__weight__step.i64 / byte_count"
Cohesion: 0.40
Nodes (5): min/optimizer/trunk__blocks__0__direction_projections__1__weight__step.i64, byte_count, element_type, sha256, shape

### Community 354 - "min/optimizer/trunk__blocks__0__direction_projections__2__weight__exp_avg.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/optimizer/trunk__blocks__0__direction_projections__2__weight__exp_avg.f32, byte_count, element_type, sha256, shape

### Community 355 - "min/optimizer/trunk__blocks__0__direction_projections__2__weight__exp_avg_sq.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/optimizer/trunk__blocks__0__direction_projections__2__weight__exp_avg_sq.f32, byte_count, element_type, sha256, shape

### Community 356 - "min/optimizer/trunk__blocks__0__direction_projections__2__weight__step.i64 / byte_count"
Cohesion: 0.40
Nodes (5): min/optimizer/trunk__blocks__0__direction_projections__2__weight__step.i64, byte_count, element_type, sha256, shape

### Community 357 - "min/optimizer/trunk__blocks__0__direction_projections__3__weight__exp_avg.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/optimizer/trunk__blocks__0__direction_projections__3__weight__exp_avg.f32, byte_count, element_type, sha256, shape

### Community 358 - "min/optimizer/trunk__blocks__0__direction_projections__3__weight__exp_avg_sq.f32 / byte_count"
Cohesion: 0.40
Nodes (5): min/optimizer/trunk__blocks__0__direction_projections__3__weight__exp_avg_sq.f32, byte_count, element_type, sha256, shape

### Community 362 - "Lane"
Cohesion: 0.29
Nodes (6): TrainingIterationWiring, games_per_iteration, policy_loss_domain, selfplay, training_batch_size, training_steps

### Community 363 - "The replay path: measured before porting anything / Still unmeasured"
Cohesion: 0.40
Nodes (4): Still unmeasured, The replay path: measured before porting anything, What it costs, What the fix is -- and is not

### Community 364 - "InferencePrecision"
Cohesion: 0.29
Nodes (6): InferencePrecision, bf16, fp16, fp32, ModelPool::ModelPool(), ModelPool::resident_count()

### Community 366 - "Golden"
Cohesion: 0.22
Nodes (7): Golden, cases, matches, load_golden(), main(), quote(), read()

### Community 369 - "Result / games"
Cohesion: 0.50
Nodes (3): Result, games, step

### Community 370 - "run_arm() / run_min_serial_search_sweep.sh"
Cohesion: 0.83
Nodes (3): run_arm(), selected(), run_min_serial_search_sweep.sh script

### Community 371 - "TrainAlphaDiamond"
Cohesion: 0.29
Nodes (6): CI fixtures, Immutable paths, Layout, Promotion, Sync, TrainAlphaDiamond

### Community 372 - "build_resources.sh"
Cohesion: 0.52
Nodes (5): limit_cpu(), limit_memory(), positive_integer(), scan_limits(), build_resources.sh script

### Community 373 - "Python-zero benchmark disposition"
Cohesion: 0.50
Nodes (3): Comparison evidence, Python-zero benchmark disposition, Tool disposition

### Community 374 - "Min serial-search sweep — 2026-09-01"
Cohesion: 0.33
Nodes (6): Comparisons, Comparisons, Decision classification, full, Min serial-search sweep — 2026-09-01, smoke

### Community 375 - "Min topology repair and fresh restart"
Cohesion: 0.50
Nodes (3): Bounded parameter measurements, Min topology repair and fresh restart, Self-play selection and startup gate

### Community 382 - "Arguments"
Cohesion: 0.33
Nodes (6): Arguments, config, cyclic, out, reverse, shared

### Community 402 - "libtorch_root"
Cohesion: 0.50
Nodes (4): libtorch_root, description, required, type

### Community 404 - "0. Where things stand right now"
Cohesion: 0.50
Nodes (4): 0. Where things stand right now, The measurement that was wrong for longer, Training past step 44,250 has not bought anything, Two numbers that were being conflated, and are now separate

### Community 407 - "moves_from"
Cohesion: 0.50
Nodes (4): 4. Findings, Ordering parity is the real content of Gate A, The step-beats-jump rule can never actually fire, moves_from()

### Community 416 - "iteration_job"
Cohesion: 0.12
Nodes (13): Controlled evidence, Finding, Implications for the proposed changes, Min local CPU audit — 2026-09-11, Reproduction and files, Training evidence recovered, arena_seed(), game_match() (+5 more)

### Community 418 - "Status"
Cohesion: 0.67
Nodes (3): Status, NeedsEvaluation, Ready

## Knowledge Gaps
- **1149 isolated node(s):** `progress`, `result`, `apply_ns`, `encode_ns`, `evaluations` (+1144 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 2619 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **22 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `string()` connect `string` to `train_main.cpp`, `filesystem`, `deployment_artifact.cpp`, `RunState`, `trainer.cpp / Trainer::train()`, `check.hpp / board.hpp`, `Match`, `encoder_version_test.cpp`, `CheckpointLineage`, `run_arm`, `load_checkpoint_v2`, `AlphaDiamond Python-Zero Proposed PR Sequence`, `EncodingVersion`, `ProductionConfig`, `MaterializedOpening`, `training_step_parity_test.cpp / check_cuda_path()`, `CheckpointInfo`, `rating.cpp / ParticipantIdentity`, `rating_store.cpp / RatingError`, `Compatibility / ModelPool::install_checkpoint()`, `replay_segment.cpp / Reader`, `SooRatingEvent / MinRatingEvent`, `AlphaDiamond Milestone 2 Implementation Plan / EloConfig`, `Min training optimisation record — 2026-08-31`, `RatingRegistry / add_participant()`, `run_state.cpp / RunStateError`, `golden_manifest_test.cpp`, `JsonValue`, `Options / artifact`, `ReplayStore::Impl`, `train() / training_step()`, `SelfPlayResult`, `replay_store.cpp`, `Binary Replay Segments Implementation Plan`, `selfplay.cpp`, `main`, `checkpoint_v2_transaction_test.cpp / check_rejected_load_is_unchanged()`, `Options / run_once()`, `schedule.cpp`, `report.cpp / dispatch_command()`, `CommandRequest`, `OpeningBlockSummary`, `IterationRequest / run_self_play()`, `Options / arm`, `The repetition trigger is implemented but unreachable / training_wiring.cpp`, `Options / parse()`, `value() / Fnv`, `training_vector_manifest_test.cpp`, `AlphaDiamond Python-Zero Migration Implementation Plan / Approval Checklist`, `selfplay.cpp`, `Episode / ModelKey`, `checkpoint.cpp`, `generate_fixtures.cpp`, `pipeline.cpp`, `JsonParser / parse_value()`, `rating_sync_main.cpp / fail()`, `Options / arm`, `PromotionStatisticsConfig`, `ResolvedDevice`, `coordinator.cpp / CoordinatorError`, `Golden / GoldenCase`, `checkpoint_benchmark.cpp`, `RunResult / aborted_games`, `ReplayStore`, `CheckpointProvenance`, `MinRatedMatch`, `DeploymentArtifact / validate_soo_deployment_artifact()`, `main`, `SelfPlayConfig / operator==()`, `release_main.cpp`, `device_test.cpp`, `Case / calls`, `RunStateStore`, `min_value_head_init_test.cpp / samples()`, `TrainingConfig / train_steps_per_iteration`, `IterationSource / checkpoint_lineage()`, `Case / calls`, `Args / parse_args()`, `OpeningSuiteConfig / from_json()`, `ModelIndexEntry`, `export_checkpoint`, `load_checkpoint`, `model_index_test.cpp`, `WorkerConfig / from_json()`, `MinRating / MinLeaderboardEntry`, `EpisodeDiagnostics / camps`, `copy_parameter`, `vacancy_distillation_test.cpp / snapshot()`, `main`, `RunBudgetConfig`, `parameter_diagnostics_test.cpp`, `IterationResult`, `fstream`, `rating_sync_test.cpp / write()`, `ModelKey`, `SooLeaderboardEntry / display_name`, `Lane`, `Golden`, `iteration_job`?**
  _High betweenness centrality (0.297) - this node is a cross-community bridge._
- **What connects `progress`, `result`, `apply_ns` to the rest of the system?**
  _1149 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `filesystem` be split into smaller, more focused modules?**
  _Cohesion score 0.13530655391120508 - nodes in this community are weakly interconnected._
- **Why does `SearchSession` connect `SearchSession / supply()` to `Status`, `EpisodeSearch / Status`, `check.hpp / board.hpp`, `9. Gate D — the Python callback, on CPU`, `Retiring the Python engine`, `Evaluator / MCTS2P`, `Encoded`, `Native Soo Self-Play — Phase 1 Progress / Environment and dependency status`, `Match`, `Lane`, `EvalOutcome`, `State`, `Rng / gammavariate()`, `MCTSConfig`, `Phase / AwaitLeaf`, `SearchResult / EvalRecord`, `Arena / Node`?**
  _High betweenness centrality (0.045) - this node is a cross-community bridge._
- **Should `RunState` be split into smaller, more focused modules?**
  _Cohesion score 0.09333333333333334 - nodes in this community are weakly interconnected._
- **Why does `State` connect `State` to `train_main.cpp`, `OwnerEvaluator`, `Golden / GoldenCase`, `check.hpp / board.hpp`, `Match`, `encoder_version_test.cpp`, `SearchSession3P / VectorNode`, `EncodingVersion`, `ProductionConfig`, `MaterializedOpening`, `moves_from`, `SearchSession / supply()`, `The promotion arena: what it cost, what was fixed, what is still open`, `iteration_job`, `Case / calls`, `Lane`, `min_experts_test.cpp / ProbeModel`, `EpisodeLane`, `Native Soo Self-Play — Phase 0 Design / Measurements this design rests on`, `mcts3p.cpp`, `Case / calls`, `Arena / Node`, `EpisodeSearch / Status`, `Options / run_once()`, `ModelPool / run_once()`, `family_case`, `VectorNode`, `Options / arm`, `value() / Fnv`, `StageTiming / profile_stages()`, `BatchItem`, `diagnose_roots() / parse_options()`, `SearchResult / EvalRecord`?**
  _High betweenness centrality (0.044) - this node is a cross-community bridge._
- **Should `trainer.cpp / Trainer::train()` be split into smaller, more focused modules?**
  _Cohesion score 0.06346153846153846 - nodes in this community are weakly interconnected._
## Engine-only semantic refresh audit

Source checkpoint `070b84d5e03535956803cc507b61200c088dcf3f`: current ownership/SDK-upgrade/model/build documentation was fully re-extracted after Task7 source deletion. Graph has 5,213 nodes / 10,486 edges; zero Qt/Desktop implementation source nodes/edges or missing source paths remain, while historical plans remain document-sourced. External backend cost: $0, zero backend tokens; host-agent token usage unmeasured. Integrity warning: 19 AST self-loop call edges remain (not evidence of actual recursion); no missing/dangling endpoints or collapsed pairs. Detailed audit: `.superpowers/sdd/2026-10-29-repository-split/graph-refresh-report.md`.
